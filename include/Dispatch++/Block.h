//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include <dispatch/dispatch.h>
#include "Dispatch++/QoS.h"
#include "Dispatch++/Time.h"

enum class DispatchWorkItemFlags: unsigned int {
    BARRIER                 = DISPATCH_BLOCK_BARRIER,
    DETACHED                = DISPATCH_BLOCK_DETACHED,
    ASSIGN_CURRENT_CONTEXT  = DISPATCH_BLOCK_ASSIGN_CURRENT,
    NO_QOS                  = DISPATCH_BLOCK_NO_QOS_CLASS,
    INHERIT_QOS             = DISPATCH_BLOCK_INHERIT_QOS_CLASS,
    ENFORCE_QOS             = DISPATCH_BLOCK_ENFORCE_QOS_CLASS,
    NONE                    = 0x00
};

class DispatchQueue;

/// The dispatch_block_t typealias is different from usual closures in that it
/// uses @convention(block). This is to avoid unnecessary bridging between
/// C blocks and Swift closures, which interferes with dispatch APIs that depend
/// on the referential identity of a block. Particularly, dispatch_block_create.
typedef void (^DispatchBlock)(void);
typedef void (^dispatch_block_t)(void);

#define DISPATCH_NOESCAPE __attribute__((__noescape__))

class DispatchWorkItem {

public:

    DispatchWorkItem(const DispatchWorkItem& other) = delete;
    DispatchWorkItem(DispatchWorkItem&& other) = default;
    DispatchWorkItem& operator= (DispatchWorkItem&& other) = default;
    DispatchWorkItem& operator= (const DispatchWorkItem& other) = delete;
    virtual ~DispatchWorkItem() = default;

    inline explicit DispatchWorkItem(
            DispatchBlock block,
            const DispatchQoS& qos = DispatchQoS::unspecified(),
            DispatchWorkItemFlags flags = DispatchWorkItemFlags::NONE)
    {
        auto block_flags = dispatch_block_flags_t(flags);
        auto qos_class = dispatch_qos_class_t(qos.qosClass);
        _block = dispatch_block_create_with_qos_class(block_flags, qos_class, qos.relativePriority, block);
    }

    inline void perform() const {
        _block();
    }

    inline void wait() const {
        dispatch_block_wait(_block, DispatchTime::distantFuture().rawValue);
    }

    [[nodiscard]] inline DispatchTimeoutResult wait(DispatchTime timeout) const {
        return dispatch_block_wait(_block, timeout.rawValue) == 0 ? DispatchTimeoutResult::SUCCESS : DispatchTimeoutResult::TIMED_OUT;
    }

    [[nodiscard]] inline DispatchTimeoutResult wait(DispatchWallTime wallTimeout) const {
        return dispatch_block_wait(_block, wallTimeout.rawValue) == 0 ? DispatchTimeoutResult::SUCCESS : DispatchTimeoutResult::TIMED_OUT;
    }

    void notify(
            const DispatchQueue& queue,
            DispatchBlock execute,
            const DispatchQoS& qos = DispatchQoS::unspecified(),
            DispatchWorkItemFlags flags = DispatchWorkItemFlags::NONE) const;

    void notify(const DispatchQueue& queue, const DispatchWorkItem& work) const;

    inline void cancel() const {
        dispatch_block_cancel(_block);
    }

    [[nodiscard]] inline bool isCancelled() const {
        return dispatch_block_testcancel(_block) != 0;
    }

    DispatchBlock _block;

private:

    // Used by DispatchQueue.synchronously<T> to provide a path through
    // dispatch_block_t, as we know the lifetime of the block in question.
    inline explicit DispatchWorkItem(DispatchWorkItemFlags flags, DISPATCH_NOESCAPE DispatchBlock noescapeBlock) {
        auto block_flags = dispatch_block_flags_t(flags);
        _block = dispatch_block_create(block_flags, noescapeBlock);
    }

    friend class DispatchGroup;
    friend class DispatchQueue;

};
