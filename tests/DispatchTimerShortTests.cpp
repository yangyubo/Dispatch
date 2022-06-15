//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#ifdef __APPLE__
#include <mach/mach_time.h>
#include <libkern/OSAtomic.h>
#endif

static auto semaphore = DispatchSemaphore(0);

static void test_stop() {
    semaphore.signal();
}

#define delay (1ull * NSEC_PER_SEC)
#define interval (5ull * NSEC_PER_USEC)

#define N 25000

static std::shared_ptr<DispatchSourceTimer> t[N];
static DispatchQueue* q;
static DispatchGroup* g;

static volatile int32_t count;
static mach_timebase_info_data_t tbi;
static uint64_t start, last2;

#define elapsed_ms(x) (((now-(x))*tbi.numer/tbi.denom)/(1000ull*NSEC_PER_USEC))

static void test_fin() {
    auto finalCount = (uint32_t)count;
    fprintf(stderr, "Called back every %llu us on average\n",
            (delay/finalCount)/NSEC_PER_USEC);
    CHECK_MESSAGE(1 < (long)ceil((double)delay/(double)(finalCount*interval)), "Frequency");
    int i;
    for (i = 0; i < N; i++) {
        t[i]->cancel();
        t[i].reset();
    }
    q->resume();
    delete q;
    delete g;
    test_stop();
}

static void test_short_timer() {
    // Add a large number of timers with suspended target queue in front of
    // the timer being measured <rdar://problem/7401353>
    g = new DispatchGroup();
    q = new DispatchQueue("q");
    int i;
    for (i = 0; i < N; i++) {
        t[i] = DispatchSource::makeTimerSource(q);
        t[i]->schedule(DispatchTime::now(), DispatchTimeInterval(interval));

        g->enter();
        t[i]->setRegistrationHandler(^{
          t[i]->suspend();
          g->leave();
        });
        t[i]->resume();
    }
    // Wait for registration & configuration of all timers
    g->wait();
    q->suspend();
    for (i = 0; i < N; i++) {
        t[i]->resume();
    }

    auto global_q = DispatchQueue::global();
    auto __block s = DispatchSource::makeTimerSource(&global_q);

    s->schedule(DispatchTime::now(), DispatchTimeInterval(interval));
    s->setEventHandler(^{
      uint64_t now = mach_absolute_time();
      if (!count) {
          global_q.asyncAfter(DispatchTime::now() + DispatchTimeInterval(delay), ^{
                s->cancel();
                s.reset();
          });
          fprintf(stderr, "First timer callback  (after %4llu ms)\n",
                  elapsed_ms(start));
      }
      OSAtomicIncrement32(&count);
      if (elapsed_ms(last2) >= 100) {
          fprintf(stderr, "%5d timer callbacks (after %4llu ms)\n", count,
                  elapsed_ms(start));
          last2 = now;
      }
    });

    s->setCancelHandler(^{
      s.reset();
      test_fin();
    });

    fprintf(stderr, "Scheduling %llu us timer\n", interval/NSEC_PER_USEC);
    start = last2 = mach_absolute_time();
    s->resume();
}

TEST_CASE("Dispatch Short Timer") { // <rdar://problem/7765184>
    mach_timebase_info(&tbi);
    test_short_timer();
    semaphore.wait();
}
