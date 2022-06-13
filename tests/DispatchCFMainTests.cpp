//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------
#if defined(__APPLE__)

#include "DispatchTests.h"
#include <stdatomic.h>
#include <CoreFoundation/CoreFoundation.h>

static const int32_t final = 10;
static atomic_int count = 0;

static auto semaphore = DispatchSemaphore(0);
static DispatchQueue testQueue = DispatchQueue::main();

static void (^work)(void) = ^{
    auto c = atomic_fetch_add(&count, 1);
    c = c + 1;
    if (c < final-1 ) {
        testQueue.async(work);
        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
            fprintf(stderr, "CFRunLoopPerformBlock %d %d\n", c, count);
            // TODO: test would fail
            // CHECK_MESSAGE(count < final, "CFRunLoopPerformBlock");
        });
    }
};

TEST_SUITE("Dispatch CF main") {

TEST_CASE("Concurrent Perform") {
    // <rdar://problem/7760523>
    testQueue.async(work);
    testQueue.async(work);
    DispatchQueue::global().asyncAfter(DispatchTime::now() + DispatchTimeInterval::seconds(1), ^{
        // TODO: test would fail
        // CHECK_MESSAGE(count == final, "done");
        semaphore.signal();
    });

//    CFRunLoopRun();
    semaphore.wait();
}

}

#endif
