//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cstdlib>
#include <cstdio>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif
#include <CoreFoundation/CoreFoundation.h>

static auto semaphore = DispatchSemaphore(0);

static void test_stop() {
    semaphore.signal();
}

const int32_t final = 10;
int global_count = 0;

static void work() {
    if (global_count == INT_MAX) {
        CFRunLoopStop(CFRunLoopGetCurrent());
        test_stop();
        return;
    }
    printf("Firing timer on main %d\n", ++global_count);
    DispatchQueue::main().asyncAfter(DispatchTime::now() + DispatchTimeInterval::microseconds(100000), ^{
      work();
    });
}

TEST_CASE("Dispatch Sync on main") {
     // <rdar://problem/7181849>
    auto dq = new DispatchQueue("foo.bar");
    dq->async(^{
      DispatchQueue::main().async(^{
        work();
      });

      int i;
      for (i=0; i<final; ++i) {
          DispatchQueue::main().sync(^{
            printf("Calling sync %d\n", i);
            CHECK_MESSAGE(pthread_main_np() == 1, "sync on main");
            if (i == final-1) {
                global_count = INT_MAX;
            }
          });
          const struct timespec t = {.tv_nsec = 50000*NSEC_PER_USEC};
          nanosleep(&t, nullptr);
      }
    });
    delete dq;

    CFRunLoopRun();
    semaphore.wait();
}
