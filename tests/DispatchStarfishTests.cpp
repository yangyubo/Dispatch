//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch++.test.starfish");

static void test_stop() {
    semaphore.signal();
}

#define COUNT	1000ul
#define LAPS	10ul

#if LENIENT_DEADLINES
#define ACCEPTABLE_LATENCY 10000
#elif TARGET_OS_EMBEDDED
#define ACCEPTABLE_LATENCY 3000
#else
#define ACCEPTABLE_LATENCY 1000
#endif

static DispatchQueue* queues[COUNT];
static size_t lap_count_down = LAPS;
static size_t count_down;
static uint64_t start;
static mach_timebase_info_data_t tbi;

static void do_test();

static void test_stop_after_delay(void *delay)
{
    if (delay != NULL) {
        sleep((unsigned int)(intptr_t)delay);
    }

    fflush(stdout);
    test_stop();
}

static void collect() {
    uint64_t delta;
    long double math;
    size_t i;

    if (--count_down) {
        return;
    }

    delta = mach_absolute_time() - start;
    delta *= tbi.numer;
    delta /= tbi.denom;
    math = delta;
    math /= COUNT * COUNT * 2ul + COUNT * 2ul;

    printf("lap: %zd\n", lap_count_down);
    printf("count: %lu\n", COUNT);
    printf("delta: %lu ns\n", (unsigned long)delta);
    printf("math: %Lf ns / lap\n", math);

    for (i = 0; i < COUNT; i++) {
        delete queues[i];
    }

    // our malloc could be a lot better,
    // this result is really a malloc torture test
    CHECK_MESSAGE(math < ACCEPTABLE_LATENCY, "Latency");

    if (--lap_count_down) {
        return do_test();
    }

    // give the threads some time to settle before test_stop() runs "leaks"
    // ...also note, this is a total cheat.   dispatch_after lets this
    // thread go idle, so dispatch cleans up the continuations cache.
    // Doign the "old style" sleep left that stuff around and leaks
    // took a LONG TIME to complete.   Long enough that the test harness
    // decided to kill us.
    dq.asyncAfter(DispatchTime::now() + DispatchTimeInterval::seconds(5), ^{
        test_stop_after_delay(nullptr);
    });
}

static void pong(DispatchQueue *this_q) {
    auto replies = this_q->getContext<unsigned long>();

    *replies -= 1;
//    dispatch_set_context(this_q, (void *)--replies);
    if (!(*replies)) {
        //printf("collect from: %s\n", dispatch_queue_get_label(dispatch_get_current_queue()));
        dq.async(^{
            collect();
        });
    }
}

static void ping(DispatchQueue *reply_q) {
    reply_q->async(^{
      pong(reply_q);
    });
}

static void start_node(DispatchQueue *this_q)
{
    size_t i;

    auto count = std::make_shared<unsigned long>(COUNT);
    this_q->setContext(count);

    for (i = 0; i < COUNT; i++) {
        queues[i]->async(^{
          ping(this_q);
        });
    }
}

void do_test() {
    auto soup = DispatchQueue::global(DispatchQoS::QoSClass::UTILITY);
    kern_return_t kr;
    char buf[1000];
    size_t i;

    count_down = COUNT;

    kr = mach_timebase_info(&tbi);
    assert(kr == 0);

    start = mach_absolute_time();

    for (i = 0; i < COUNT; i++) {
        snprintf(buf, sizeof(buf), "com.example.starfish-node#%zd", i);
        queues[i] = new DispatchQueue(buf);
        queues[i]->suspend();
        queues[i]->setTarget(soup);
    }

    for (i = 0; i < COUNT; i++) {
        queues[i]->async(^{
          start_node(queues[i]);
        });
    }

    for (i = 0; i < COUNT; i++) {
        queues[i]->resume();
    }
}

TEST_CASE("Dispatch++ Starfish") {
    do_test();
    semaphore.wait();
}
