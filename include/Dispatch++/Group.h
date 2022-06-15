//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include "Dispatch++/Object.h"
#include "Dispatch++/Time.h"
#include "Dispatch++/Block.h"
#include "Dispatch++/Queue.h"
#include <dispatch/dispatch.h>

class DispatchGroup : DispatchObject {
public:

    inline DispatchGroup() {
        _wrapped = dispatch_group_create();
    }
    DispatchGroup(const DispatchGroup& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    DispatchGroup& operator= (const DispatchGroup& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    inline DispatchGroup(DispatchGroup&& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    inline DispatchGroup& operator= (DispatchGroup&& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }
    inline ~DispatchGroup() override {
        if (_wrapped) {
            dispatch_release(_wrapped);
        }
    }

    inline void enter() const {
        dispatch_group_enter(_wrapped);
    }

    inline void leave() const {
        dispatch_group_leave(_wrapped);
    }

    // Extension

    void notify(
            const DispatchQueue& queue,
            DispatchBlock work,
            const DispatchQoS& qos = DispatchQoS::unspecified(),
            DispatchWorkItemFlags flags = DispatchWorkItemFlags::NONE) const;

    void notify(const DispatchQueue& queue, const DispatchWorkItem& work) const;

    inline void wait() const {
        dispatch_group_wait(_wrapped, DispatchTime::distantFuture().rawValue);
    }

    [[nodiscard]] inline DispatchTimeoutResult wait(DispatchTime timeout) const {
        return dispatch_group_wait(_wrapped, timeout.rawValue) == 0 \
 ? DispatchTimeoutResult::SUCCESS : DispatchTimeoutResult::TIMED_OUT;
    }

    [[nodiscard]] inline DispatchTimeoutResult wait(DispatchWallTime wallTimeout) const {
        return dispatch_group_wait(_wrapped, wallTimeout.rawValue) == 0 \
 ? DispatchTimeoutResult::SUCCESS : DispatchTimeoutResult::TIMED_OUT;
    }

private:

    dispatch_group_t _wrapped {nullptr};

    inline dispatch_object_t wrapped() override {
        return _wrapped;
    }

    friend class DispatchQueue;

};
