//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"
#include <vector>

static DispatchQueue semaphoreQueue = DispatchQueue("dispatch.unittest.dispatch-cascade-semaphore");
static auto semaphore = DispatchSemaphore(0);

static int done = 0;
static constexpr int QUEUES = 80;
static std::vector<DispatchQueue> queues {};

static constexpr int BLOCKS = 10000;
union {
    size_t index;
    char padding[64];
} indices[BLOCKS];
static size_t iterations = (QUEUES * BLOCKS) / 4;


static void histogram() {
    size_t counts[QUEUES] = {};
    size_t maxcount = 0;

    size_t q;
    for (q = 0; q < QUEUES; ++q) {
        size_t i;
        for (i = 0; i < BLOCKS; ++i) {
            if (indices[i].index == q) {
                ++counts[q];
            }
        }
    }

    for (q = 0; q < QUEUES; ++q) {
        if (counts[q] > maxcount) {
            maxcount = counts[q];
        }
    }

    printf("maxcount = %zd\n", maxcount);

    size_t x,y;
    for (y = 20; y > 0; --y) {
        for (x = 0; x < QUEUES; ++x) {
            double fraction = (double)counts[x] / (double)maxcount;
            double value = fraction * (double)20;
            printf("%s", (value > (double)y) ? "*" : " ");
        }
        printf("\n");
    }
}

static void cascade(void *context) {
    size_t idx, *idxptr = (size_t *)context;

    if (done) return;

    idx = *idxptr + 1;

    if (idx < QUEUES) {
        *idxptr = idx;
        queues[idx].async(^{
            cascade(context);
        });
    }

    if (__sync_sub_and_fetch(&iterations, 1) == 0) {
        done = 1;
        histogram();

        semaphoreQueue.async(^{
            for (int q = 0; q < QUEUES; ++q) {
                queues[q].sync(^{
                    return;
                });
            }
            semaphore.signal();
        });
    }
}

TEST_SUITE("Dispatch Cascade") {

TEST_CASE("cascade") {
    for (int i=0; i < QUEUES; ++i) {
        queues.emplace_back(DispatchQueue("dispatch.unittest.dispatch-cascade: " + std::to_string(i)));
    }
    for (auto & indice : indices) {
        cascade(&indice.index);
    }

    semaphore.wait();
}

}
