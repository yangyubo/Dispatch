//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#endif

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch.test.timer-bit31");

static void test_stop() {
    semaphore.signal();
}

static void test_timer() {
    struct timeval start_time;

    static auto s = DispatchSource::makeTimerSource(&dq);
    CHECK_MESSAGE(s != nullptr, "DispatchSource::makeTimerSource");
    s->schedule(DispatchTime::now() + DispatchTimeInterval(0x80000000ull), DispatchTimeInterval(0x80000000ull));
    gettimeofday(&start_time, NULL);

    s->setEventHandler(^{
      s->cancel();
    });

    s->setCancelHandler(^{
      struct timeval end_time;
      gettimeofday(&end_time, NULL);
      // check, s/b 2.0799... seconds, which is <4 seconds
      // when it could end on a bad boundry.
      CHECK_MESSAGE((end_time.tv_sec - start_time.tv_sec) < 4, "needs to finish faster than 4 seconds");
      // And it has to take at least two seconds...
      CHECK_MESSAGE((end_time.tv_sec - start_time.tv_sec) > 1, "can't finish faster than 2 seconds");
      test_stop();
    });

    s->resume();
}

TEST_CASE("Dispatch Source Timer, bit 31") {
    test_timer();
    semaphore.wait();
}
