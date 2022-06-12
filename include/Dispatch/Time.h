//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include <dispatch/dispatch.h>

struct DispatchTime {

    dispatch_time_t rawValue;

    static inline DispatchTime now() {
        auto t = dispatch_time(DISPATCH_TIME_NOW, 0);
        return DispatchTime(t);
    }

    static DispatchTime& distantFuture();

    explicit DispatchTime(dispatch_time_t rawValue) : rawValue(rawValue) {}

    inline bool operator<(const DispatchTime& other) const {
        return rawValue < other.rawValue;
    }

    inline bool operator==(const DispatchTime& other) const {
        return rawValue == other.rawValue;
    }

    inline bool operator>(const DispatchTime& other) const {
        return rawValue > other.rawValue;
    }

};

struct DispatchWallTime {

    dispatch_time_t rawValue;

    static inline DispatchWallTime now() {
        auto t = dispatch_walltime(nullptr, 0);
        return DispatchWallTime(t);
    }

    static DispatchWallTime& distantFuture();

    explicit DispatchWallTime(dispatch_time_t rawValue) : rawValue(rawValue) {}
    explicit DispatchWallTime(const timespec& timespec) {
        rawValue = dispatch_walltime(&timespec, 0);
    }

    inline bool operator<(const DispatchWallTime& other) const {
        dispatch_time_t negativeOne = ~0ull;
        if (other.rawValue == negativeOne) {
            return rawValue != negativeOne;
        } else if (rawValue == negativeOne) {
            return false;
        }
        return -rawValue < -other.rawValue;
    }

    inline bool operator==(const DispatchWallTime& other) const {
        return rawValue == other.rawValue;
    }

};

enum class DispatchTimeoutResult {
    // static let KERN_OPERATION_TIMED_OUT:Int = 49
    SUCCESS,
    TIMED_OUT
};

/// Represents a time interval that can be used as an offset from a `DispatchTime`
/// or `DispatchWallTime`.
///
/// For example:
/// 	let inOneSecond = DispatchTime.now() + DispatchTimeInterval.seconds(1)
///
/// If the requested time interval is larger then the internal representation
/// permits, the result of adding it to a `DispatchTime` or `DispatchWallTime`
/// is `DispatchTime.distantFuture` and `DispatchWallTime.distantFuture`
/// respectively. Such time intervals compare as equal:
///
/// 	let t1 = DispatchTimeInterval.seconds(Int.max)
///		let t2 = DispatchTimeInterval.milliseconds(Int.max)
///		let result = t1 == t2   // true

struct DispatchTimeInterval {

    int64_t rawValue;

    explicit DispatchTimeInterval(int64_t rawValue) : rawValue(rawValue) {}

    static DispatchTimeInterval seconds(int s);
    static DispatchTimeInterval milliseconds(int ms);
    static DispatchTimeInterval microseconds(int us);
    static DispatchTimeInterval nanoseconds(int ns);
    static DispatchTimeInterval& never();

    bool operator==(const DispatchTimeInterval& other) const;

};

DispatchTime operator+(DispatchTime time, DispatchTimeInterval interval);
DispatchTime operator-(DispatchTime time, DispatchTimeInterval interval);
DispatchTime operator+(DispatchTime time, double seconds);
DispatchTime operator-(DispatchTime time, double seconds);

DispatchWallTime operator+(DispatchWallTime time, DispatchTimeInterval interval);
DispatchWallTime operator-(DispatchWallTime time, DispatchTimeInterval interval);
DispatchWallTime operator+(DispatchWallTime time, double seconds);
DispatchWallTime operator-(DispatchWallTime time, double seconds);

