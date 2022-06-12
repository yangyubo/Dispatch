//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"
#include <vector>

static const char *ctxts[] = {"ctxt for app", "ctxt for key 1",
                              "ctxt for key 2", "ctxt for key 1 bis", "ctxt for key 4"};

static std::vector<DispatchSpecificKey<const std::string>> ctxtKeys;
static std::vector<std::shared_ptr<const std::string>> ctxtValues;

volatile long ctxts_destroyed;
static DispatchGroup* g;

static void (^cleanup)(std::shared_ptr<const std::string> &) = ^ (std::shared_ptr<const std::string> &value) {
    fprintf(stderr, "destructor of %s\n", (*value).c_str());
    (void)__sync_add_and_fetch(&ctxts_destroyed, 1);
    g->leave();
};

static void test_context_for_key() {
    __block auto q = DispatchQueue("q");
    auto tq = DispatchQueue {"tq", DispatchQueue::Attributes::CONCURRENT};

    for (auto ctxt : ctxts) {
        ctxtKeys.emplace_back(DispatchSpecificKey<const std::string>{});

        auto value = std::make_shared<const std::string>(ctxt);
        ctxtValues.emplace_back(value);
    }

    DispatchQueue ttq = DispatchQueue::global();
    g->enter();
    CHECK_EQ(ctxtValues[4].use_count(), 1);
    tq.setSpecific(ctxtKeys[4], ctxtValues[4], cleanup);

    tq.setTarget(ttq);
    g->enter();

    q.setContext(ctxtValues[0], cleanup);
    q.setTarget(tq);

    q.async(^{
        g->enter();
        q.setSpecific(ctxtKeys[1], ctxtValues[1], cleanup);
    });

    DispatchQueue::global().async(^{
        g->enter();
        q.setSpecific(ctxtKeys[2], ctxtValues[2], cleanup);
        DispatchQueue::global().async(^{
            auto ctxt = q.getSpecific(ctxtKeys[2]);
            CHECK_MESSAGE(ctxt == ctxtValues[2], "get context for key 2");
        });
    });

    q.async(^{
        auto ctxt = DispatchQueue::getCurrentSpecific(ctxtKeys[1]);
        CHECK_MESSAGE(ctxt == ctxtValues[1], "get current context for key 1");

        ctxt = DispatchQueue::getCurrentSpecific(ctxtKeys[4]);
        CHECK_MESSAGE(ctxt == ctxtValues[4], "get current context for key 4 (on target queue");

        ctxt = q.getSpecific(ctxtKeys[1]);
        CHECK_MESSAGE(ctxt == ctxtValues[1], "get context for key 1");
    });

    q.async(^{
        g->enter();
        q.setSpecific(ctxtKeys[1], ctxtValues[3], cleanup);
        auto ctxt = q.getSpecific(ctxtKeys[1]);

        CHECK_MESSAGE(ctxt == ctxtValues[3], "get context for key 1");
    });

    q.async(^{
        auto c = std::shared_ptr<const std::string>(nullptr);
        q.setSpecific(ctxtKeys[1], c, cleanup);

        auto ctxt = q.getSpecific(ctxtKeys[1]);
        CHECK_MESSAGE(ctxt == nullptr, "get context for key 1");
    });

    auto ctxt = q.getContext<const std::string>();
    CHECK_MESSAGE(ctxt == ctxtValues[0], "get context for app");
}

TEST_CASE("Dispatch Queue Specific") {
    // rdar://problem/8429188
    auto q = DispatchQueue { "dispatch.unittest.dispatch-context_for-key" };
    __block auto semaphore = DispatchSemaphore(0);

    q.async(^{
        g = new DispatchGroup();
        test_context_for_key();
        g->wait();
        CHECK_MESSAGE(ctxts_destroyed == 5, "contexts destroyed");
        delete g;
        semaphore.signal();
    });

    semaphore.wait();
}
