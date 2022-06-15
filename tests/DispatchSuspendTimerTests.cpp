//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#include <cstdio>

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch.test.suspend-timer");

static void test_stop() {
    semaphore.signal();
}

std::shared_ptr<DispatchSourceTimer> tweedledee;
std::shared_ptr<DispatchSourceTimer> tweedledum;

static void test_timer() {
    __block int i = 0, i_prime = 0;
    __block int j = 0;

    tweedledee = DispatchSource::makeTimerSource(&dq);
    CHECK_MESSAGE(tweedledee != nullptr, "DispatchSource::makeTimerSource");

    tweedledee->schedule(DispatchTime::now() + DispatchTimeInterval::seconds(1), DispatchTimeInterval::seconds(1));
    tweedledee->setCancelHandler(^{
      tweedledee.reset();
      test_stop();
    });

    tweedledee->setEventHandler(^{
      i_prime += (int)tweedledee->getData();
      fprintf(stderr, "tweedledee %d (%d)\n", ++i, i_prime);
      if (i == 10) {
        tweedledee->cancel();
      }
    });

    tweedledee->resume();

    tweedledum = DispatchSource::makeTimerSource(&dq);
    CHECK_MESSAGE(tweedledum != nullptr, "DispatchSource::makeTimerSource");

    tweedledum->schedule(DispatchTime::now() + DispatchTimeInterval::seconds(3) + DispatchTimeInterval::milliseconds(500), \
                        DispatchTimeInterval::seconds(3));
    tweedledum->setCancelHandler(^{
        tweedledum.reset();
        test_stop();
    });

    tweedledum->setEventHandler(^{
      auto tmp_i = i;
      auto tmp_i_prime = i_prime;

      switch(++j) {
      case 1:
          fprintf(stderr, "suspending timer for 3 seconds\n");
          tweedledee->suspend();
          break;
      case 2:
          fprintf(stderr, "resuming timer\n");
          CHECK_MESSAGE(tmp_i == 3, "tweedleded tick count");
          CHECK_MESSAGE(tmp_i_prime == 3, "tweedledee virtual tick count");
          tweedledee->resume();
          break;
      default:
          CHECK_MESSAGE(tmp_i == 7, "tweedleded tick count");
          CHECK_MESSAGE(tmp_i_prime == 9, "tweedledee virtual tick count");
          tweedledee->cancel();
          break;
      }
    });

    tweedledum->resume();
}

TEST_CASE("Dispatch Suspend Timer") {
    test_timer();
    semaphore.wait();
}
