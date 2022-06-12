//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------
#include "DispatchTests.h"

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#include <unistd.h>
#endif
#include <cstdio>

#define ACCEPTABLE_DRIFT 0.001

TEST_CASE("Dispatch Timer Drift") {
    __block uint32_t count = 0;
    __block double last_jitter = 0;
    __block double drift_sum = 0;
    __block auto semaphore = DispatchSemaphore(0);
    auto q = new DispatchQueue("tech.shifor.Dispatch.DispatchTimerDriftTests");

    // 100 times a second
    uint64_t interval = 1000000000 / 100;
    double interval_d = double(interval) / 1000000000.0;
    // for 25 seconds
    auto target = (unsigned int)(25.0 / interval_d);

    auto t = DispatchSource::makeTimerSource(q);
    auto intvl = DispatchTimeInterval::nanoseconds(int(interval));
    auto deadline = DispatchTime::now() + intvl;

    t->schedule(deadline, intvl);
    t->setEventHandler(^{
        struct timeval now_tv {};
        static double first = 0;
        gettimeofday(&now_tv, nullptr);
        double now = double(now_tv.tv_sec) + double(now_tv.tv_usec) / 1000000.0;

        if (count == 0) {
            // Because this is taken at 1st timer fire,
            // later jitter values may be negitave.
            // This doesn't effect the drift calculation.
            first = now;
        }
        double goal = first + interval_d * count;
        double jitter = goal - now;
        double drift = jitter - last_jitter;
        drift_sum += drift;

        printf("%4d: jitter %f, drift %f\n", count, jitter, drift);

        if (target <= ++count) {
            drift_sum /= count - 1;
            if (drift_sum < 0) {
                drift_sum = -drift_sum;
            }
            double acceptable_drift = ACCEPTABLE_DRIFT;
            auto sum = drift_sum;
            CHECK_LT(sum, acceptable_drift);
            semaphore.signal();
        }
        last_jitter = jitter;
    });

    t->resume();

    semaphore.wait();
    delete q;
}
