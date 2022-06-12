//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

TEST_CASE("Dispatch After") {
    __block DispatchQueue queue = DispatchQueue{"dispatch.unittest.dispatch-after"};
    auto semaphore = std::make_shared<DispatchSemaphore>(0);

    queue.async(^{
        auto time_a_min = DispatchTime::now() +
                          DispatchTimeInterval::milliseconds(
                                  5500); // 5.5 seconds
        auto time_a = DispatchTime::now() +
                      DispatchTimeInterval::milliseconds(6000); // 6 seconds
        auto time_a_max = DispatchTime::now() +
                          DispatchTimeInterval::milliseconds(
                                  6800); // 6.8s would fail, 6.6s would pass
        queue.asyncAfter(time_a, ^{
            auto now_a = DispatchTime::now();
            CHECK_MESSAGE(now_a > time_a_min, "can't finish faster than 5.5s");
            CHECK_MESSAGE(now_a < time_a_max, "must finish faster than  6.8s");

            auto time_b_min = DispatchTime::now() +
                              DispatchTimeInterval::milliseconds(1500);
            auto time_b = DispatchTime::now() +
                          DispatchTimeInterval::milliseconds(2000);
            auto time_b_max = DispatchTime::now() +
                              DispatchTimeInterval::milliseconds(2500);
            queue.asyncAfter(time_b, ^{
                auto now_b = DispatchTime::now();
                CHECK_MESSAGE(now_b > time_b_min, "can't finish faster than 1.5s");
                CHECK_MESSAGE(now_b < time_b_max, "must finish faster than 2.5s");

                auto time_c_min = DispatchTime::now();
                auto time_c = DispatchTime::now();
                auto time_c_max = DispatchTime::now() +
                                  DispatchTimeInterval::milliseconds(500);
                queue.asyncAfter(time_c, ^{
                    auto now_c = DispatchTime::now();
                    CHECK_MESSAGE(now_c > time_c_min, "can't finish faster than 0s");
                    CHECK_MESSAGE(now_c < time_c_max, "must finish faster than 0.5s");

                    auto done = DispatchWorkItem(^{
                        std::this_thread::sleep_for(1s);
                        // unblock main queue we have finished
                        semaphore->signal();
                    });

                    queue.async(done);
                });
            });
        });
    });

    // wait until the test queue to finish
    semaphore->wait();
}
