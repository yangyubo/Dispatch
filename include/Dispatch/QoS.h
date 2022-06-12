//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------
#pragma once


#include <dispatch/dispatch.h>
#include <future>

/// qos_class_t

struct DispatchQoS {

    enum class QoSClass: unsigned int  {
        USER_INTERACTIVE  = 0x21,
        USER_INITIATED    = 0x19,
        DEFAULT           = 0x15,
        UTILITY           = 0x11,
        BACKGROUND        = 0x09,
        UNSPECIFIED       = 0x00
    };

    QoSClass qosClass;
    int relativePriority;

    inline DispatchQoS(QoSClass qosClass, int relativePriority) : qosClass(qosClass), relativePriority(relativePriority) {}

    bool operator==(const DispatchQoS& other) const;
    inline bool operator!=(const DispatchQoS& other) const {
        return !(*this == other);
    }

    static DispatchQoS& background();
    static DispatchQoS& utility();
    static DispatchQoS& default_qos();
    static DispatchQoS& userInitiated();
    static DispatchQoS& userInteractive();
    static DispatchQoS& unspecified();

};
