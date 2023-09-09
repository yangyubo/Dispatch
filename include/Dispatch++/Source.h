//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include "Dispatch++/QoS.h"
#include "Dispatch++/Block.h"
#include "Dispatch++/Object.h"
#include <dispatch/dispatch.h>
#include <cmath>

typedef void (^DispatchSourceHandler)(void);

class DispatchSourceProtocol {
public:
    inline void setEventHandler(DispatchSourceHandler _Nullable handler) {
        setEventHandler(DispatchQoS::unspecified(), DispatchWorkItemFlags::NONE, handler);
    }
    inline void setEventHandler(DispatchQoS qos, DispatchSourceHandler _Nullable handler) {
        setEventHandler(qos, DispatchWorkItemFlags::NONE, handler);
    }
    inline void setEventHandler(DispatchWorkItemFlags flags, DispatchSourceHandler _Nullable handler) {
        setEventHandler(DispatchQoS::unspecified(), flags, handler);
    }
    virtual void setEventHandler(DispatchQoS qos, DispatchWorkItemFlags flags, DispatchSourceHandler _Nullable handler) = 0;
    virtual void setEventHandler(DispatchWorkItem handler) = 0;

    inline void setCancelHandler(DispatchSourceHandler _Nullable handler) {
        setCancelHandler(DispatchQoS::unspecified(), DispatchWorkItemFlags::NONE, handler);
    }
    inline void setCancelHandler(DispatchQoS qos, DispatchSourceHandler _Nullable handler) {
        setCancelHandler(qos, DispatchWorkItemFlags::NONE, handler);
    }
    inline void setCancelHandler(DispatchWorkItemFlags flags, DispatchSourceHandler _Nullable handler) {
        setCancelHandler(DispatchQoS::unspecified(), flags, handler);
    }
    virtual void setCancelHandler(DispatchQoS qos, DispatchWorkItemFlags flags, DispatchSourceHandler _Nullable handler) = 0;
    virtual void setCancelHandler(DispatchWorkItem handler) = 0;

    inline void setRegistrationHandler(DispatchSourceHandler _Nullable handler) {
        setRegistrationHandler(DispatchQoS::unspecified(), DispatchWorkItemFlags::NONE, handler);
    }
    inline void setRegistrationHandler(DispatchQoS qos, DispatchSourceHandler _Nullable handler) {
        setRegistrationHandler(qos, DispatchWorkItemFlags::NONE, handler);
    }
    inline void setRegistrationHandler(DispatchWorkItemFlags flags, DispatchSourceHandler _Nullable handler) {
        setRegistrationHandler(DispatchQoS::unspecified(), flags, handler);
    }
    virtual void setRegistrationHandler(DispatchQoS qos, DispatchWorkItemFlags flags, DispatchSourceHandler _Nullable handler) = 0;
    virtual void setRegistrationHandler(DispatchWorkItem handler) = 0;

    virtual void activate() = 0;
    virtual void cancel() = 0;
    virtual void resume() = 0;
    virtual void suspend() = 0;

    virtual uint getHandle() = 0;
    virtual uint getMask() = 0;
    virtual uint getData() = 0;
    virtual bool isCancelled() = 0;
};

class DispatchSourceUserDataAdd: public DispatchSourceProtocol {
public:
    virtual void dataAdd(uint data) = 0;
};

class DispatchSourceUserDataOr: public DispatchSourceProtocol {
public:
    virtual void dataOr(uint data) = 0;
};

class DispatchSourceUserDataReplace: public DispatchSourceProtocol {
public:
    virtual void dataReplace(uint data) = 0;
};

class DispatchSourceRead: public DispatchSourceProtocol {
};

class DispatchSourceSignal: public DispatchSourceProtocol {
};

class DispatchSourceTimer: public DispatchSourceProtocol {
public:
    inline void schedule(DispatchTime deadline) {
        schedule(deadline, DispatchTimeInterval::never(), DispatchTimeInterval::nanoseconds(0));
    }
    inline void schedule(DispatchTime deadline, DispatchTimeInterval repeatingInterval) {
        schedule(deadline, repeatingInterval, DispatchTimeInterval::nanoseconds(0));
    }
    virtual void schedule(
            DispatchTime deadline,
            DispatchTimeInterval repeatingInterval,
            DispatchTimeInterval leeway) = 0;

    inline void schedule(DispatchTime deadline, double repeatingInterval) {
        schedule(deadline, repeatingInterval, DispatchTimeInterval::nanoseconds(0));
    }
    virtual void schedule(
            DispatchTime deadline,
            double repeatingInterval,
            DispatchTimeInterval leeway) = 0;

    inline void schedule(DispatchWallTime wallDeadline) {
        schedule(wallDeadline, DispatchTimeInterval::never(), DispatchTimeInterval::nanoseconds(0));
    }
    inline void schedule(DispatchWallTime wallDeadline, DispatchTimeInterval repeatingInterval) {
        schedule(wallDeadline, repeatingInterval, DispatchTimeInterval::nanoseconds(0));
    }
    virtual void schedule(
            DispatchWallTime wallDeadline,
            DispatchTimeInterval repeatingInterval,
            DispatchTimeInterval leeway) = 0;

    inline void schedule(DispatchWallTime wallDeadline, double repeatingInterval) {
        schedule(wallDeadline, repeatingInterval, DispatchTimeInterval::nanoseconds(0));
    }
    virtual void schedule(
            DispatchWallTime wallDeadline,
            double repeatingInterval,
            DispatchTimeInterval leeway) = 0;
};

class DispatchSourceWrite: public DispatchSourceProtocol {
};

class DispatchSource: public DispatchObject, public DispatchSourceRead,
                      public  DispatchSourceSignal, public DispatchSourceTimer,
                      public DispatchSourceUserDataAdd, public DispatchSourceUserDataOr,
                      public DispatchSourceUserDataReplace, public DispatchSourceWrite
{
public:

    DispatchSource(const DispatchSource& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    DispatchSource& operator= (const DispatchSource& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    inline DispatchSource(DispatchSource&& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    inline DispatchSource& operator= (DispatchSource&& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }
    inline ~DispatchSource() override {
        dispatch_release(_wrapped);
    }

    // MARK: - DispatchSourceProtocol

    inline void setEventHandler(
            DispatchQoS qos,
            DispatchWorkItemFlags flags,
            DispatchSourceHandler _Nullable handler) override
    {
        if (handler != nullptr && (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE))
        {
            auto item = DispatchWorkItem(handler, qos, flags);
            dispatch_source_set_event_handler(_wrapped, item._block);
        } else {
            dispatch_source_set_event_handler(_wrapped, handler);
        }
    }

    inline void setEventHandler(DispatchWorkItem handler) override {
        dispatch_source_set_event_handler(_wrapped, handler._block);
    }

    inline void setCancelHandler(
            DispatchQoS qos,
            DispatchWorkItemFlags flags,
            DispatchSourceHandler _Nullable handler) override
    {
        if (handler != nullptr && (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE))
        {
            auto item = DispatchWorkItem(handler, qos, flags);
            dispatch_source_set_cancel_handler(_wrapped, item._block);
        } else {
            dispatch_source_set_cancel_handler(_wrapped, handler);
        }
    }

    inline void setCancelHandler(DispatchWorkItem handler) override {
        dispatch_source_set_cancel_handler(_wrapped, handler._block);
    }

    inline void setRegistrationHandler(
            DispatchQoS qos,
            DispatchWorkItemFlags flags,
            DispatchSourceHandler _Nullable handler) override
    {
        if (handler != nullptr && (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE))
        {
            auto item = DispatchWorkItem(handler, qos, flags);
            dispatch_source_set_registration_handler(_wrapped, item._block);
        } else {
            dispatch_source_set_registration_handler(_wrapped, handler);
        }
    }

    inline void setRegistrationHandler(DispatchWorkItem handler) override {
        dispatch_source_set_registration_handler(_wrapped, handler._block);
    }

    inline void activate() override {
        DispatchObject::activate();
    }

    inline void resume() override {
        DispatchObject::resume();
    }

    inline void suspend() override {
        DispatchObject::suspend();
    }

    inline void cancel() override {
        dispatch_source_cancel(_wrapped);
    }

    inline uint getHandle() override {
        return dispatch_source_get_handle(_wrapped);
    }

    inline uint getMask() override {
        return dispatch_source_get_mask(_wrapped);
    }

    inline uint getData() override {
        return dispatch_source_get_data(_wrapped);
    }

    inline bool isCancelled() override {
        return dispatch_source_testcancel(_wrapped) != 0;
    }

    // MARK: - DispatchSource

    enum struct TimerFlags: uint {
        NONE = 0,
        STRICT = 1
    };

    enum struct FileSystemEvent: uint {
        DELETE  = 0x1,
        WRITE   = 0x2,
        EXTEND  = 0x4,
        ATTRIB  = 0x8,
        LINK    = 0x10,
        RENAME  = 0x20,
        REVOKE  = 0x40,
        FUNLOCK = 0x100,
        ALL     = DELETE | WRITE | EXTEND | ATTRIB | LINK | RENAME | REVOKE | FUNLOCK
    };

    static std::shared_ptr<DispatchSourceRead> makeReadSource(int32_t fileDescriptor, const DispatchQueue *queue = nullptr);
    static std::shared_ptr<DispatchSourceSignal> makeSignalSource(int32_t signal, const DispatchQueue *queue = nullptr);

    inline static std::shared_ptr<DispatchSourceTimer> makeTimerSource(const DispatchQueue *queue) {
        return makeTimerSource(TimerFlags::NONE, queue);
    }
    static std::shared_ptr<DispatchSourceTimer> makeTimerSource(TimerFlags flags = TimerFlags::NONE, const DispatchQueue *queue = nullptr);

    static std::shared_ptr<DispatchSourceUserDataAdd> makeUserDataAddSource(const DispatchQueue *queue = nullptr);
    static std::shared_ptr<DispatchSourceUserDataOr> makeUserDataOrSource(const DispatchQueue *queue = nullptr);
    static std::shared_ptr<DispatchSourceUserDataReplace> makeUserDataReplaceSource(const DispatchQueue *queue = nullptr);
    static std::shared_ptr<DispatchSourceWrite> makeWriteSource(int32_t fileDescriptor, const DispatchQueue *queue = nullptr);

    // MARK: - DispatchSourceTimer

    ///
    /// Sets the deadline, repeat interval and leeway for a timer event.
    ///
    /// Once this function returns, any pending source data accumulated for the previous timer values
    /// has been cleared. The next timer event will occur at `deadline` and every `repeating` units of
    /// time thereafter until the timer source is canceled. If the value of `repeating` is `.never`,
    /// or is defaulted, the timer fires only once.
    ///
    /// Delivery of a timer event may be delayed by the system in order to improve power consumption
    /// and system performance. The upper limit to the allowable delay may be configured with the `leeway`
    /// argument; the lower limit is under the control of the system.
    ///
    /// For the initial timer fire at `deadline`, the upper limit to the allowable delay is set to
    /// `leeway`. For the subsequent timer fires at `deadline + N * repeating`, the upper
    /// limit is the smaller of `leeway` and `repeating/2`.
    ///
    /// The lower limit to the allowable delay may vary with process state such as visibility of the
    /// application UI. If the timer source was created with flags `TimerFlags.strict`, the system
    /// will make a best effort to strictly observe the provided `leeway` value, even if it is smaller
    /// than the current lower limit. Note that a minimal amount of delay is to be expected even if
    /// this flag is specified.
    ///
    /// Calling this method has no effect if the timer source has already been canceled.
    ///
    /// - parameter deadline: the time at which the first timer event will be delivered, subject to the
    ///     leeway and other considerations described above. The deadline is based on Mach absolute
    ///     time.
    /// - parameter repeating: the repeat interval for the timer, or `.never` if the timer should fire
    ///		only once.
    /// - parameter leeway: the leeway for the timer.
    ///
    inline void schedule(
            DispatchTime deadline,
            DispatchTimeInterval repeatingInterval,
            DispatchTimeInterval leeway) override
    {
        dispatch_source_set_timer(
                _wrapped,
                deadline.rawValue,
                repeatingInterval == DispatchTimeInterval::never() ? ~0 : repeatingInterval.rawValue,
                uint64_t (leeway.rawValue)
        );
    }

    ///
    /// Sets the deadline, repeat interval and leeway for a timer event.
    ///
    /// Once this function returns, any pending source data accumulated for the previous timer values
    /// has been cleared. The next timer event will occur at `deadline` and every `repeating` seconds
    /// thereafter until the timer source is canceled. If the value of `repeating` is `.infinity`,
    /// the timer fires only once.
    ///
    /// Delivery of a timer event may be delayed by the system in order to improve power consumption
    /// and system performance. The upper limit to the allowable delay may be configured with the `leeway`
    /// argument; the lower limit is under the control of the system.
    ///
    /// For the initial timer fire at `deadline`, the upper limit to the allowable delay is set to
    /// `leeway`. For the subsequent timer fires at `deadline + N * repeating`, the upper
    /// limit is the smaller of `leeway` and `repeating/2`.
    ///
    /// The lower limit to the allowable delay may vary with process state such as visibility of the
    /// application UI. If the timer source was created with flags `TimerFlags.strict`, the system
    /// will make a best effort to strictly observe the provided `leeway` value, even if it is smaller
    /// than the current lower limit. Note that a minimal amount of delay is to be expected even if
    /// this flag is specified.
    ///
    /// Calling this method has no effect if the timer source has already been canceled.
    ///
    /// - parameter deadline: the time at which the timer event will be delivered, subject to the
    ///     leeway and other considerations described above. The deadline is based on Mach absolute
    ///     time.
    /// - parameter repeating: the repeat interval for the timer in seconds, or `.infinity` if the timer
    ///		should fire only once.
    /// - parameter leeway: the leeway for the timer.
    ///
    inline void schedule(
            DispatchTime deadline,
            double repeatingInterval,
            DispatchTimeInterval leeway) override
    {
        auto interval = std::isinf(repeatingInterval) ? ~0 : uint64_t(repeatingInterval * double(NSEC_PER_SEC));
        dispatch_source_set_timer(
                _wrapped,
                deadline.rawValue,
                interval,
                uint64_t(leeway.rawValue)
        );
    }
    ///
    /// Sets the deadline, repeat interval and leeway for a timer event.
    ///
    /// Once this function returns, any pending source data accumulated for the previous timer values
    /// has been cleared. The next timer event will occur at `wallDeadline` and every `repeating` units of
    /// time thereafter until the timer source is canceled. If the value of `repeating` is `.never`,
    /// or is defaulted, the timer fires only once.
    ///
    /// Delivery of a timer event may be delayed by the system in order to improve power consumption and
    /// system performance. The upper limit to the allowable delay may be configured with the `leeway`
    /// argument; the lower limit is under the control of the system.
    ///
    /// For the initial timer fire at `wallDeadline`, the upper limit to the allowable delay is set to
    /// `leeway`. For the subsequent timer fires at `wallDeadline + N * repeating`, the upper
    /// limit is the smaller of `leeway` and `repeating/2`.
    ///
    /// The lower limit to the allowable delay may vary with process state such as visibility of the
    /// application UI. If the timer source was created with flags `TimerFlags.strict`, the system
    /// will make a best effort to strictly observe the provided `leeway` value, even if it is smaller
    /// than the current lower limit. Note that a minimal amount of delay is to be expected even if
    /// this flag is specified.
    ///
    /// Calling this method has no effect if the timer source has already been canceled.
    ///
    /// - parameter wallDeadline: the time at which the timer event will be delivered, subject to the
    ///     leeway and other considerations described above. The deadline is based on
    ///     `gettimeofday(3)`.
    /// - parameter repeating: the repeat interval for the timer, or `.never` if the timer should fire
    ///		only once.
    /// - parameter leeway: the leeway for the timer.
    ///
    inline void schedule(
            DispatchWallTime wallDeadline,
            DispatchTimeInterval repeatingInterval,
            DispatchTimeInterval leeway) override
    {
        dispatch_source_set_timer(
                _wrapped,
                wallDeadline.rawValue,
                repeatingInterval == DispatchTimeInterval::never() ? ~0 : repeatingInterval.rawValue,
                uint64_t (leeway.rawValue)
        );
    }
    ///
    /// Sets the deadline, repeat interval and leeway for a timer event that fires at least once.
    ///
    /// Once this function returns, any pending source data accumulated for the previous timer values
    /// has been cleared. The next timer event will occur at `wallDeadline` and every `repeating` seconds
    /// thereafter until the timer source is canceled. If the value of `repeating` is `.infinity`,
    /// the timer fires only once.
    ///
    /// Delivery of a timer event may be delayed by the system in order to improve power consumption
    /// and system performance. The upper limit to the allowable delay may be configured with the `leeway`
    /// argument; the lower limit is under the control of the system.
    ///
    /// For the initial timer fire at `wallDeadline`, the upper limit to the allowable delay is set to
    /// `leeway`. For the subsequent timer fires at `wallDeadline + N * repeating`, the upper
    /// limit is the smaller of `leeway` and `repeating/2`.
    ///
    /// The lower limit to the allowable delay may vary with process state such as visibility of the
    /// application UI. If the timer source was created with flags `TimerFlags.strict`, the system
    /// will make a best effort to strictly observe the provided `leeway` value, even if it is smaller
    /// than the current lower limit. Note that a minimal amount of delay is to be expected even if
    /// this flag is specified.
    ///
    /// Calling this method has no effect if the timer source has already been canceled.
    ///
    /// - parameter wallDeadline: the time at which the timer event will be delivered, subject to the
    ///     leeway and other considerations described above. The deadline is based on
    ///     `gettimeofday(3)`.
    /// - parameter repeating: the repeat interval for the timer in seconds, or `.infinity` if the timer
    ///		should fire only once.
    /// - parameter leeway: the leeway for the timer.
    ///
    inline void schedule(
            DispatchWallTime wallDeadline,
            double repeatingInterval,
            DispatchTimeInterval leeway) override
    {
        auto interval = std::isinf(repeatingInterval) ? ~0 : uint64_t(repeatingInterval * double(NSEC_PER_SEC));
        dispatch_source_set_timer(
                _wrapped,
                wallDeadline.rawValue,
                interval,
                uint64_t(leeway.rawValue)
        );
    }

    // MARK: - DispatchSourceUserDataAdd

    /// Merges data into a dispatch source of type `DISPATCH_SOURCE_TYPE_DATA_ADD`
    /// and submits its event handler block to its target queue.
    ///
    /// - parameter data: the value to add to the current pending data. A value of zero
    ///		has no effect and will not result in the submission of the event handler block.
    inline void dataAdd(uint data) override {
        dispatch_source_merge_data(_wrapped, data);
    }

    // MARK: - DispatchSourceUserDataOr

    /// Merges data into a dispatch source of type `DISPATCH_SOURCE_TYPE_DATA_OR` and
    /// submits its event handler block to its target queue.
    ///
    /// - parameter data: The value to OR into the current pending data. A value of zero
    ///		has no effect and will not result in the submission of the event handler block.
    inline void dataOr(uint data) override {
        dispatch_source_merge_data(_wrapped, data);
    }

    // MARK: - DispatchSourceUserDataReplace

    /// Merges data into a dispatch source of type `DISPATCH_SOURCE_TYPE_DATA_REPLACE`
    /// and submits its event handler block to its target queue.
    ///
    /// - parameter data: The value that will replace the current pending data.
    ///		A value of zero will be stored but will not result in the submission of the event
    ///		handler block.
    inline void dataReplace(uint data) override {
        dispatch_source_merge_data(_wrapped, data);
    }


private:
    dispatch_source_t _wrapped { nullptr };

    dispatch_object_t wrapped() override  {
        return _wrapped;
    }

    explicit DispatchSource(dispatch_source_t source) : _wrapped(source) {}

};
