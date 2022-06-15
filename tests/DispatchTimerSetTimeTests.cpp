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
static auto dq = DispatchQueue("Dispatch.test.timer-settime");

static void test_stop() {
    semaphore.signal();
}

static void test_timer() {
    __block int i = 0;
    struct timeval start_time;

    gettimeofday(&start_time, NULL);

    auto __block s = DispatchSource::makeTimerSource(&dq);

    s->schedule(DispatchTime::now(), DispatchTimeInterval::seconds(1), DispatchTimeInterval::seconds(1));

    s->setCancelHandler(^{
      struct timeval end_time;
      gettimeofday(&end_time, NULL);
      // Make sure we actually managed to adjust the interval
      // duration.  Fifteen one second ticks would blow past
      // this.
      CHECK_MESSAGE((end_time.tv_sec - start_time.tv_sec) < 10, "total duration");
      test_stop();

      s.reset();
    });

    s->setEventHandler(^{
      fprintf(stderr, "%d\n", ++i);
      if (i >= 15) {
          s->cancel();
      } else if (i == 1) {
          s->schedule(DispatchTime::now(), DispatchTimeInterval::milliseconds(100));
      }
    });

    s->resume();

}

TEST_CASE("Dispatch Update Timer") {
    test_timer();
    semaphore.wait();
}
