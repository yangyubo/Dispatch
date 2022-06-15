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
static auto dq = DispatchQueue("Dispatch.test.timer-bit63");

static void test_stop() {
    semaphore.signal();
}

static void test_timer() {
    uint64_t interval = 0x8000000000000001ull;

    __block int i = 0;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    static auto ds = DispatchSource::makeTimerSource(&dq);
    CHECK_MESSAGE(ds != nullptr, "DispatchSource::makeTimerSource");

    ds->setEventHandler(^{
      auto tmp_i = i;
      CHECK_LT(tmp_i, 1);
      printf("%d\n", i++);
    });

    ds->schedule(DispatchTime::now(), DispatchTimeInterval(interval));
    ds->resume();

    dq.asyncAfter(DispatchTime::now() + DispatchTimeInterval::seconds(1), ^{
      test_stop();
    });
}

TEST_CASE("Dispatch Source Timer, bit 63") {
    test_timer();
    semaphore.wait();
}
