//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <pthread.h>
#include <stdio.h>
#include <cassert>

#define LAPS 10000

TEST_CASE("Dispatch Semaphore") {
    static long total;

    {
        static auto dsema = DispatchSemaphore(1);

        auto queue = DispatchQueue::global();
        queue.apply(LAPS, ^(size_t i) {
            dsema.wait();
            total++;
            dsema.signal();
        });
    }

    CHECK_MESSAGE(total == LAPS, "count");
}
