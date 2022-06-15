//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"
#include <stdatomic.h>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#ifdef __ANDROID__
#include <linux/sysctl.h>
#else
#if !defined(__linux__)
#include <sys/sysctl.h>
#endif
#endif /* __ANDROID__ */
#endif

static atomic_uint busy_threads_started, busy_threads_finished;

/*
 * Keep a thread busy, spinning on the CPU.
 */
static volatile int all_done = 0;

/* Fiddling with j in the middle and hitting this global will hopefully keep
 * the optimizer from cutting the whole thing out as dead code.
 */
static volatile unsigned int busythread_useless;

static void (^busythread)(void) = ^{
    /* prevent i and j been optimized out */
    volatile uint64_t i = 0, j = 0;

    atomic_fetch_add(&busy_threads_started, 1);

    while(!all_done)
    {
        if(i == 500000) { j = j - uint64_t(busythread_useless); }
        j = j + i;
        i = i + 1;
    }
    (void)j;

    atomic_fetch_add(&busy_threads_finished, 1);
};

TEST_SUITE("Dispatch++ Apply") {

DispatchQueue dq = DispatchQueue::global();

TEST_CASE("Concurrent Perform") {
    int final = 32;
    __block atomic_size_t count = 0;

    dq.apply(final, ^(size_t i) {
        atomic_fetch_add(&count, 1);
    });

    int added = atomic_load(&count);
    CHECK_EQ(added, final);

    count = 0; // rdar://problem/9294578
    dq.apply(final, ^(size_t i) {
        dq.apply(final, ^(size_t j) {
            dq.apply(final, ^(size_t k) {
                atomic_fetch_add(&count, 1);
            });
        });
    });

    added = atomic_load(&count);
    CHECK_EQ(added, final * final * final);
}

/*
 * Test that dispatch_apply can make progress and finish, even if there are
 * so many other running and unblocked workqueue threads that the apply's
 * helper threads never get a chance to come up.
 *
 * <rdar://problem/10718199> dispatch_apply should not block waiting on other
 * threads while calling thread is available
 */
TEST_CASE("Apply Contended") {
    uint32_t activecpu;
#if defined(__linux__) || defined(__OpenBSD__)
    activecpu = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_WIN32)
    SYSTEM_INFO si;
	GetSystemInfo(&si);
	activecpu = si.dwNumberOfProcessors;
#else
    size_t s = sizeof(activecpu);
    sysctlbyname("hw.activecpu", &activecpu, &s, nullptr, 0);
#endif

    uint32_t tIndex, n_threads = (int)activecpu;
    DispatchGroup grp {};

    for(tIndex = 0; tIndex < n_threads; tIndex++) {
        dq.async(grp, busythread);
    }

    // Spin until all the threads have actually started
    while(atomic_load(&busy_threads_started) < n_threads) {
        usleep(1);
    }

    __block atomic_int count = 0;
    const int32_t final = 32;
    auto before = int32_t(busy_threads_started);

    dq.apply(final, ^(size_t i) {
        atomic_fetch_add(&count, 1);
    });

    auto after = atomic_load(&busy_threads_finished);

    CHECK_MESSAGE(before == n_threads, "contended: threads started before apply");
    auto c = atomic_load(&count);
    CHECK_MESSAGE(c == final, "contended: count");
    CHECK_MESSAGE(after == 0, "contended: threads finished before apply");

    /* Release busy threads by setting all_done to 1 */
    all_done = 1;

    grp.wait();
}

}