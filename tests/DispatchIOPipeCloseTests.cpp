//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <cassert>
#include <fcntl.h>
#include <stdatomic.h>
#include <cstdio>
#include <cstdlib>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <fts.h>
#include <sys/param.h>
#include <unistd.h>
#endif
#include <cerrno>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <libkern/OSAtomic.h>
#include <TargetConditionals.h>
#endif
#ifdef __linux__
#include <sys/resource.h>
#endif

static auto semaphore = DispatchSemaphore(0);

static void test_stop() {
    semaphore.signal();
}

TEST_CASE("Dispatch++ IO Pipe Close") {
    int pipe_fds[2] = { -1, -1 };
    int pipe_err = pipe(pipe_fds);
    int readFD = pipe_fds[0];
    int writeFD = pipe_fds[1];
    if (pipe_err) {
        CHECK_MESSAGE(errno == 0, "pipe");
        test_stop();
    }

    printf("readFD=%lld, writeFD=%lld\n", (long long)readFD, (long long)writeFD);
    auto q = DispatchQueue("q");
    auto __block io = DispatchIO(DispatchIO::StreamType::STREAM, readFD, q, ^(int err) {
        printf("cleanup, err=%d\n", err);
        close(readFD);
        printf("all done\n");
        test_stop();
    });
    io.setLowWater(0);
    io.read(0, UINT_MAX, q, ^(bool done, const std::shared_ptr<DispatchData> data, int err) {
      printf("read: %d, %zu, %d\n", done, data == nullptr ? 0 : data->count(), err);
      if (data != nullptr && data->count() > 0) {
          // will only happen once
          printf("closing writeFD\n");
          close(writeFD);
          q.asyncAfter(DispatchTime::now() + DispatchTimeInterval::nanoseconds(1), ^{
              io.close();
          });
      }
    });
    io.resume();

    printf("writing\n");
    write(writeFD, "x", 1);
    printf("wrtten\n");

    semaphore.wait();
}
