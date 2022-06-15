//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cstdlib>
#include <assert.h>
#include <cstdio>
#include <cstring>

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch++.test.timer");

static void test_stop() {
    semaphore.signal();
}

static bool finalized = false;

static void test_timer() {
    const int stop_at = 3;
    int64_t j;

    // create timers in two classes:
    //  * ones that should trigger before the test ends
    //  * ones that shouldn't trigger before the test ends
    for (j = 1; j <= 5; ++j) {
        auto gq = DispatchQueue::global();
        auto __block s = DispatchSource::makeTimerSource(&gq);

        int64_t delta = (int64_t)((uint64_t)j * NSEC_PER_SEC + NSEC_PER_SEC / 10);
        s->schedule(DispatchTime::now() + DispatchTimeInterval::nanoseconds(delta));

        s->setEventHandler(^{
          if (!finalized) {
              CHECK_MESSAGE(j <= stop_at, "timer number");
              fprintf(stderr, "timer[%lu]\n", (unsigned long)j);
          }
          s.reset();
        });
        s->resume();
    }

    __block int i = 0;

    auto __block s = DispatchSource::makeTimerSource(&dq);
    s->schedule(DispatchTime::now(), DispatchTimeInterval::seconds(1));

    s->setCancelHandler(^{
      s.reset();
      finalized = true;
      test_stop();
    });

    s->setEventHandler(^{
      fprintf(stderr, "%d\n", ++i);
      if (i >= stop_at) {
          auto tmp_i = i;
          CHECK_MESSAGE(tmp_i == stop_at, "i");
          s->schedule(DispatchTime::now());
          s->cancel();
      }
    });

    s->resume();
}

TEST_CASE("Dispatch++ Source Timer") {
    test_timer();
    semaphore.wait();
}
