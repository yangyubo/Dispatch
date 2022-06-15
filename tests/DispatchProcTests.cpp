//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cstdio>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif
#include <cstdlib>
#include <cassert>
#include <spawn.h>
#include <csignal>
#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#endif

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch++.test.proc");

static void test_stop() {
    semaphore.signal();
}

#define PID_CNT 5

static long event_cnt;

static void test_proc(pid_t bad_pid)
{
    dispatch_source_t proc_s[PID_CNT], proc;
    int res;
    pid_t pid, monitor_pid;

    event_cnt = 0;
    // Creates a process and register multiple observers.  Send a signal,
    // exit the process, etc., and verify all observers were notified.

    posix_spawnattr_t attr;
    res = posix_spawnattr_init(&attr);
    assert(res == 0);
    res = posix_spawnattr_setflags(&attr, POSIX_SPAWN_START_SUSPENDED);
    assert(res == 0);

    char* args[] = {
        "/bin/sleep", "2", nullptr
    };

    res = posix_spawnp(&pid, args[0], NULL, &attr, args, NULL);
    if (res < 0) {
        perror(args[0]);
        exit(127);
    }

    res = posix_spawnattr_destroy(&attr);
    assert(res == 0);

    auto group = DispatchGroup();

    assert(pid > 0);
    monitor_pid = bad_pid ? bad_pid : pid; // rdar://problem/8090801

    int i;
    for (i = 0; i < PID_CNT; ++i) {
        group.enter();
        proc = proc_s[i] = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC,
                                                  monitor_pid, DISPATCH_PROC_EXIT, dispatch_get_global_queue(0, 0));
        CHECK_MESSAGE(proc != nullptr, "dispatch_source_proc_create");
        dispatch_source_set_event_handler(proc, ^{
          long flags = dispatch_source_get_data(proc);
          CHECK_MESSAGE(flags == DISPATCH_PROC_EXIT, "DISPATCH_PROC_EXIT");
          event_cnt++;
          dispatch_source_cancel(proc);
        });
        dispatch_source_set_cancel_handler(proc, ^{
            group.leave();
        });
        dispatch_resume(proc);
    }
    kill(pid, SIGCONT);
    auto waitRet = group.wait(DispatchTime::now() + DispatchTimeInterval::seconds(10));
    if (waitRet == DispatchTimeoutResult::TIMED_OUT) {
        for (i = 0; i < PID_CNT; ++i) {
            dispatch_source_cancel(proc_s[i]);
        }
        group.wait();
    }
    for (i = 0; i < PID_CNT; ++i) {
        dispatch_release(proc_s[i]);
    }

    // delay 5 seconds to give a chance for any bugs that
    // result in too many events to be noticed
    dq.asyncAfter(DispatchTime::now() + DispatchTimeInterval::seconds(5), ^{
      int status;
      int res2 = waitpid(pid, &status, 0);
      assert(res2 != -1);
      //int passed = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
      CHECK_MESSAGE((WEXITSTATUS(status) | WTERMSIG(status)) == 0, "Sub-process exited");
      CHECK_MESSAGE(event_cnt == PID_CNT, "Event count");
      if (bad_pid) {
          test_stop();
      } else {
          dq.async(^{
            test_proc(pid);
          });
      }
    });
}

TEST_CASE("Dispatch++ Proc") {
    test_proc(0);
    semaphore.wait();
}
