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

static void test_stop() {
    semaphore.signal();
}

TEST_CASE("Dispatch Source Timeout") {
    // <rdar://problem/8015967>
    uint64_t mini_interval = 100ull; // 100 ns
    uint64_t long_interval = 2000000000ull; // 2 secs

    auto timer_queue = new DispatchQueue("timer_queue");

    __block int value_to_be_changed = 5;
    __block int fired = 0;

    auto mini_timer = DispatchSource::makeTimerSource(timer_queue);
    mini_timer->setEventHandler(^{
      printf("Firing mini-timer %d\n", ++fired);
      printf("Suspending mini-timer queue\n");
      timer_queue->suspend();
    });

    mini_timer->schedule(DispatchTime::now(), DispatchTimeInterval(mini_interval));

    auto global_q = DispatchQueue::global();
    auto long_timer = DispatchSource::makeTimerSource(&global_q);
    long_timer->setEventHandler(^{
      printf("Firing long timer\n");
      value_to_be_changed = 10;
      long_timer->cancel();
      test_stop();
    });

    long_timer->schedule(DispatchTime::now() + DispatchTimeInterval::seconds(1), DispatchTimeInterval(long_interval));

    mini_timer->resume();
    long_timer->resume();

    semaphore.wait();

    auto tmp_value_to_be_changed = value_to_be_changed;
    CHECK_MESSAGE(tmp_value_to_be_changed == 10, "Checking final value");
    auto tmp_fired = fired;
    CHECK_MESSAGE(tmp_fired == 1, "Mini-timer fired");

    mini_timer->cancel();
    mini_timer.reset();

    timer_queue->resume();
    delete timer_queue;

    long_timer.reset();
}
