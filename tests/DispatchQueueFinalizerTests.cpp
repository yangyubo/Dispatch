//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#endif
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cassert>

static auto semaphore = DispatchSemaphore(0);
static auto dq = DispatchQueue("Dispatch.test.queue-finalizer");

static void test_stop() {
    semaphore.signal();
}

std::shared_ptr<long> ctxt_magic;
std::shared_ptr<long> ctxt_null;

static void never_call(void *ctxt) {
    CHECK_MESSAGE(false, "never_call should not run");
    CHECK_MESSAGE(ctxt == nullptr, "correct context");
}

TEST_CASE("Dispatch Queue Finalizer") {
    auto number = random();
    ctxt_magic = std::make_shared<long>(number);

    // we need a non-NULL value for the tests to work properly
    CHECK(ctxt_magic != nullptr);

    {
        auto __block q = DispatchQueue("Dispatch.testing.finalizer");

        {
            auto q_null = DispatchQueue("Dispatch.testing.finalizer.context_null");

            q_null.setContext(ctxt_null, ^(std::shared_ptr<long> &value) {
              CHECK_MESSAGE(false, "never_call should not run");
              CHECK_MESSAGE(value == nullptr, "correct context");
            });
        }

        // Don't test k
        dq.asyncAfter(DispatchTime::now() + DispatchTimeInterval::seconds(1), ^{
          // Usign async to set the context helps test that blocks are
          // run before the release as opposed to just thrown away.
          q.async(^{
            q.setContext(ctxt_magic, ^(std::shared_ptr<long> &value) {
              CHECK_MESSAGE(value == ctxt_magic, "correct context");
              test_stop();
            });
          });
        });
    }

    semaphore.wait();
}
