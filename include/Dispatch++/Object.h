//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include <string>
#include <dispatch/dispatch.h>
#include "Dispatch++/QoS.h"
#include "Dispatch++/Block.h"
#include "Dispatch++/Utils.h"

DISPATCH_NOTHROW DISPATCH_NORETURN
inline void dispatchMain() {
    dispatch_main();
}

class DispatchQueue;


class DispatchObject {

public:

    DispatchObject() = default;

    // Copy and move constructors and assignment operators should be
    // implemented explicitly in derived classes.
    // Retain wrapped object when copy/move.
    DispatchObject(const DispatchObject& other) = delete;
    DispatchObject(DispatchObject&& other) = delete;
    DispatchObject& operator= (const DispatchObject& other) = delete;
    DispatchObject& operator= (DispatchObject&& other) = delete;

    virtual ~DispatchObject() = default;

    void setTarget(const DispatchQueue& queue);

    inline virtual void activate() {
        dispatch_activate(wrapped());
    }

    inline virtual void suspend() {
        dispatch_suspend(wrapped());
    }

    inline virtual void resume() {
        dispatch_resume(wrapped());
    }

protected:

    virtual dispatch_object_t wrapped()=0;

};
