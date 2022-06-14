//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------
#include "DispatchTests.h"

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cassert>
#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#endif
#include <cmath>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

#if TARGET_OS_EMBEDDED
#define LOOP_COUNT 50000
#else
#define LOOP_COUNT 200000
#endif

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch.test.group");

static DispatchGroup create_group(size_t count, unsigned int delay) {
    size_t i;
    auto group = DispatchGroup();

    for (i = 0; i < count; ++i) {
        auto queue = DispatchQueue("Dispatch.test.create_group");

        queue.async(group, ^{
            if (delay) {
                fprintf(stderr, "sleeping...\n");
                sleep(delay);
                fprintf(stderr, "done.\n");
            }
        });
  }
  return group;
}

static long completed;
static auto rq = DispatchQueue {"release"};
static auto nq = DispatchQueue {"notify"};

static void test_group_notify2(long cycle, const DispatchGroup& tested) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        rq.suspend();
    });
    rq.resume();

    // n=4 works great for a 4CPU Mac Pro, this might work for a wider range of
    // systems.
#if HAVE_ARC4RANDOM
    const int n = 1 + arc4random() % 8;
#else
    const int n = 1 + random() % 8;
#endif

    auto group = DispatchGroup();
    DispatchQueue* qa[n];

    group.enter();
    for (int i = 0; i < n; i++) {
        char buf[48];
        sprintf(buf, "T%ld-%d", cycle, i);
        qa[i] = new DispatchQueue(buf);
    }

    __block float eh = 0;
    for (int i = 0; i < n; i++) {
        DispatchQueue* q = qa[i];
        q->async(group, ^{
          // Seems to trigger a little more reliably with some work being
          // done in this block
          assert(cycle && "cycle must be non-zero");
          eh = (float)sin(M_1_PI / cycle);
        });
    }
    group.leave();

    group.notify(nq, ^{
      completed = cycle;
      tested.leave();
    });

    // Releasing qa's queues here seems to avoid the race, so we are arranging
    // for the current iteration's queues to be released on the next iteration.
    rq.sync(^{});
    rq.suspend();
    for (int i = 0; i < n; i++) {
        auto q = qa[i];
        rq.async(^{
            delete q;
        });
    }
}

static void (^test_group_notify)(void) = ^{
    // <rdar://problem/11445820>
    auto tested = DispatchGroup();
    long i;
    for (i = 1; i <= LOOP_COUNT; i++) {
        if (!(i % (LOOP_COUNT/10))) {
            fprintf(stderr, "#%ld\n", i);
        }
        tested.enter();
        test_group_notify2(i, tested);

        auto res = tested.wait(DispatchTime::now() + DispatchTimeInterval::seconds(1));
        if (res != DispatchTimeoutResult::SUCCESS) {
            break;
        }
    }
    CHECK_MESSAGE(i - 1 == LOOP_COUNT, "dispatch_group_notify");
    semaphore.signal();
};

static void (^test_group)(void) = ^ {
    DispatchGroup group;

    group = create_group(100, 0);
    group.wait();

    // should be OK to re-use a group
    DispatchQueue::global().async(group, ^{});
    group.wait();

    group = create_group(3, 7);
    auto res = group.wait(DispatchTime::now() + DispatchTimeInterval::seconds(5));
    CHECK_EQ(res, DispatchTimeoutResult::TIMED_OUT);

    // retry after timeout (this time succeed)
    res = group.wait(DispatchTime::now() + DispatchTimeInterval::seconds(5));
    CHECK_EQ(res, DispatchTimeoutResult::SUCCESS);

    group = create_group(100, 0);
    group.notify(dq, ^{
        dq.dispatchPrecondition(DispatchPredicate::ON_QUEUE);
        dq.async(test_group_notify);
    });
};

TEST_CASE("Dispatch Group") {
  dq.async(test_group);
  semaphore.wait();
  // Dispatch object could not be released with suspended state, so resume it before exit.
  rq.resume();
}
