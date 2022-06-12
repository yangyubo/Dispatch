//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include "Dispatch/Object.h"

class DispatchSemaphore: DispatchObject {

public:

    DispatchSemaphore(const DispatchSemaphore& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    DispatchSemaphore& operator= (const DispatchSemaphore& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    inline DispatchSemaphore(DispatchSemaphore&& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    inline DispatchSemaphore& operator= (DispatchSemaphore&& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }
    inline ~DispatchSemaphore() override {
        dispatch_release(wrapped());
    }

    inline explicit DispatchSemaphore(int value) {
        _wrapped = dispatch_semaphore_create(value);
    }

    inline int signal() {
        return int(dispatch_semaphore_signal(_wrapped));
    }

    inline void wait() const {
        dispatch_semaphore_wait(_wrapped, DispatchTime::distantFuture().rawValue);
    }

    [[nodiscard]] inline DispatchTimeoutResult wait(DispatchTime timeout) const {
        return dispatch_semaphore_wait(_wrapped, timeout.rawValue) == 0 \
                        ? DispatchTimeoutResult::SUCCESS : DispatchTimeoutResult::TIMED_OUT;
    }

    [[nodiscard]] inline DispatchTimeoutResult wait(DispatchWallTime wallTimeout) const {
        return dispatch_semaphore_wait(_wrapped, wallTimeout.rawValue) == 0 \
                        ? DispatchTimeoutResult::SUCCESS : DispatchTimeoutResult::TIMED_OUT;
    }

private:

    inline dispatch_object_t wrapped() override {
        return _wrapped;
    }

    dispatch_semaphore_t _wrapped {nullptr};

};


