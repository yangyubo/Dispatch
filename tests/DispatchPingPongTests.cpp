//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch++.test.io-muxed");

static void test_stop() {
    semaphore.signal();
}

static uint32_t count = 0;
static const uint32_t final = 1000000; // 10M

static void pingpongloop(const DispatchGroup& group, const DispatchQueue& ping, const DispatchQueue& pong, size_t counter)
{
    if (counter < final) {
        pong.async(group, ^{ pingpongloop(group, pong, ping, counter+1); });
    } else {
        count = (uint32_t)counter;
    }
}

TEST_CASE("Dispatch++ Ping Pong") {
    auto ping = DispatchQueue("ping");
    auto pong = DispatchQueue("pong");

    auto group = DispatchGroup();

    pingpongloop(group, ping, pong, 0);
    group.wait();

    CHECK_MESSAGE(count == final, "count");
}
