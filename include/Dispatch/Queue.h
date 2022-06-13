//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include "Dispatch/Object.h"
#include "Dispatch/Group.h"
#include "Dispatch/Block.h"

class DispatchIO;
class DispatchGroup;

enum class DispatchPredicate {
    ON_QUEUE,
    ON_QUEUE_AS_BARRIER,
    NOT_ON_QUEUE
};

template <class T>
class DispatchSpecificKey {
public:
    typedef void (*ValueDestructor)(T *_Nullable);
};

template<typename T>
using DispatchSpecificCleanup = void (^)(std::shared_ptr<T>& value);

template <class T>
class _DispatchSpecificValue {
public:

    std::shared_ptr<T> value;

    DispatchSpecificCleanup<T> cleanup {nullptr};

    _DispatchSpecificValue(std::shared_ptr<T> value, DispatchSpecificCleanup<T> cleanup) : value(value), cleanup(cleanup) {}
};

template <class T>
void _destructDispatchSpecificValue(void *_Nullable ptr) {
    if (ptr) {
        auto specValue = static_cast<_DispatchSpecificValue<T> *>(ptr);
        if (specValue->cleanup) {
            specValue->cleanup(specValue->value);
        }
        delete specValue;
    }
}

class DispatchQueue: public DispatchObject {

public:

    inline DispatchQueue(const DispatchQueue& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    inline DispatchQueue& operator= (const DispatchQueue& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    DispatchQueue(DispatchQueue&& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }

    DispatchQueue& operator= (DispatchQueue&& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    inline ~DispatchQueue() override {
        if (_wrapped) {
            dispatch_release(_wrapped);
        }
    }

    void preconditionTest(DispatchPredicate condition) const;

    enum class Attributes: uint64_t {
        NONE               = 0,
        CONCURRENT         = 1,
        INITIALLY_INACTIVE = 2
    };

    enum class AutoreleaseFrequency: u_long {
        INHERIT   = DISPATCH_AUTORELEASE_FREQUENCY_INHERIT,
        WORK_ITEM = DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM,
        NEVER     = DISPATCH_AUTORELEASE_FREQUENCY_NEVER
    };

    inline static void concurrentPerform(size_t iterations, DISPATCH_NOESCAPE void (^work)(size_t)) {
        dispatch_apply(iterations, nullptr, ^(size_t i) {
            work(i);
        });
    }

    inline static DispatchQueue main() {
        return DispatchQueue(dispatch_get_main_queue());
    }

    inline static DispatchQueue global(DispatchQoS::QoSClass qos = DispatchQoS::QoSClass::DEFAULT) {
        return DispatchQueue(dispatch_get_global_queue(int(qos), 0));
    }

    template <class T>
    inline static std::shared_ptr<T> getCurrentSpecific(const DispatchSpecificKey<T>& key) {
        auto p = (_DispatchSpecificValue<T> *)(dispatch_get_specific(&key));
        return p ? p->value : nullptr;
    }

    template <class T>
    inline std::shared_ptr<T> getSpecific(const DispatchSpecificKey<T>& key) {
        auto p = (_DispatchSpecificValue<T> *)(dispatch_queue_get_specific(_wrapped, &key));
        return p ? p->value : nullptr;
    }

    template <class T>
    inline void setSpecific(const DispatchSpecificKey<T>& key, std::shared_ptr<T> value, DispatchSpecificCleanup<T> cleanup = nullptr) {
        if (value) {
            auto wrappedValue = new _DispatchSpecificValue<T>(value, cleanup);
            dispatch_queue_set_specific(_wrapped, &key, wrappedValue, _destructDispatchSpecificValue<T>);
        } else {
            dispatch_queue_set_specific(_wrapped, &key, nullptr, nullptr);
        }
    }

    template <class T>
    inline void setContext(std::shared_ptr<T> value, DispatchSpecificCleanup<T> cleanup = nullptr) {
        if (value) {
            auto wrappedValue = new _DispatchSpecificValue<T>(value, cleanup);
            dispatch_set_context(_wrapped,  wrappedValue);
            dispatch_set_finalizer_f(_wrapped, _destructDispatchSpecificValue<T>);
        } else {
            dispatch_set_context(_wrapped, nullptr);
        }
    }
    template <class T>
    inline std::shared_ptr<T> getContext() {
        auto p = (_DispatchSpecificValue<T> *)(dispatch_get_context(_wrapped));
        return p ? p->value : nullptr;
    }

    [[nodiscard]] inline std::string label() const {
        return (dispatch_queue_get_label(_wrapped));
    }

    DispatchQueue(
            const std::string& label,
            const DispatchQoS& qos,
            Attributes attributes,
            AutoreleaseFrequency autoreleaseFrequency,
            const DispatchQueue* _Nullable queue);

    explicit DispatchQueue(const std::string& label): DispatchQueue(
            label,
            DispatchQoS::unspecified(),
            Attributes::NONE,
            AutoreleaseFrequency::INHERIT,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, const DispatchQoS& qos): DispatchQueue(
            label,
            qos,
            Attributes::NONE,
            AutoreleaseFrequency::INHERIT,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, const DispatchQoS& qos, Attributes attributes): DispatchQueue(
            label,
            qos,
            attributes,
            AutoreleaseFrequency::INHERIT,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, const DispatchQoS& qos, Attributes attributes, AutoreleaseFrequency autoreleaseFrequency): DispatchQueue(
            label,
            qos,
            attributes,
            autoreleaseFrequency,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, Attributes attributes, AutoreleaseFrequency autoreleaseFrequency): DispatchQueue(
            label,
            DispatchQoS::unspecified(),
            attributes,
            autoreleaseFrequency,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, Attributes attributes): DispatchQueue(
            label,
            DispatchQoS::unspecified(),
            attributes,
            AutoreleaseFrequency::INHERIT,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, const DispatchQoS& qos, AutoreleaseFrequency autoreleaseFrequency): DispatchQueue(
            label,
            qos,
            Attributes::NONE,
            autoreleaseFrequency,
            nullptr
    ) {}
    DispatchQueue(const std::string& label, AutoreleaseFrequency autoreleaseFrequency, const DispatchQueue* _Nullable queue): DispatchQueue(
            label,
            DispatchQoS::unspecified(),
            Attributes::NONE,
            autoreleaseFrequency,
            queue
    ) {}
    DispatchQueue(const std::string& label, const DispatchQoS& qos, const DispatchQueue* _Nullable queue): DispatchQueue(
            label,
            qos,
            Attributes::NONE,
            AutoreleaseFrequency::INHERIT,
            queue
    ) {}
    DispatchQueue(const std::string& label, const DispatchQueue* _Nullable queue): DispatchQueue(
            label,
            DispatchQoS::unspecified(),
            Attributes::NONE,
            AutoreleaseFrequency::INHERIT,
            queue
    ) {}

    ///
    /// Submits a block for synchronous execution on this queue.
    ///
    /// Submits a work item to a dispatch queue like `async(execute:)`, however
    /// `sync(execute:)` will not return until the work item has finished.
    ///
    /// Work items submitted to a queue with `sync(execute:)` do not observe certain
    /// queue attributes of that queue when invoked (such as autorelease frequency
    /// and QoS class).
    ///
    /// Calls to `sync(execute:)` targeting the current queue will result
    /// in deadlock. Use of `sync(execute:)` is also subject to the same
    /// multi-party deadlock problems that may result from the use of a mutex.
    /// Use of `async(execute:)` is preferred.
    ///
    /// As an optimization, `sync(execute:)` invokes the work item on the thread which
    /// submitted it, except when the queue is the main queue or
    /// a queue targetting it.
    ///
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `async(execute:)`
    ///
    void sync(const DispatchWorkItem& workItem) const {
        dispatch_sync(_wrapped, workItem._block);
    }

    ///
    /// Submits a work item for asynchronous execution on a dispatch queue.
    ///
    /// `async(execute:)` is the fundamental mechanism for submitting
    /// work items to a dispatch queue.
    ///
    /// Calls to `async(execute:)` always return immediately after the work item has
    /// been submitted, and never wait for the work item to be invoked.
    ///
    /// The target queue determines whether the work item will be invoked serially or
    /// concurrently with respect to other work items submitted to that same queue.
    /// Serial queues are processed concurrently with respect to each other.
    ///
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `sync(execute:)`
    ///
    void async(const DispatchWorkItem& workItem) const {
        dispatch_async(_wrapped, workItem._block);
    }
    ///
    /// Submits a work item to a dispatch queue and associates it with the given
    /// dispatch group. The dispatch group may be used to wait for the completion
    /// of the work items it references.
    ///
    /// - parameter group: the dispatch group to associate with the submitted block.
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `sync(execute:)`
    ///
    void async(const DispatchGroup& group, const DispatchWorkItem& workItem) const;

    ///
    /// Submits a work item to a dispatch queue and optionally associates it with a
    /// dispatch group. The dispatch group may be used to wait for the completion
    /// of the work items it references.
    ///
    /// - parameter group: the dispatch group to associate with the submitted
    /// work item. If this is `nil`, the work item is not associated with a group.
    /// - parameter flags: flags that control the execution environment of the
    /// - parameter qos: the QoS at which the work item should be executed.
    ///     Defaults to `DispatchQoS.unspecified`.
    /// - parameter flags: flags that control the execution environment of the
    /// work item.
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `sync(execute:)`
    /// - SeeAlso: `DispatchQoS`
    /// - SeeAlso: `DispatchWorkItemFlags`
    ///
    inline void async(const DispatchGroup& group, const DispatchQoS& qos, DispatchWorkItemFlags flags, DispatchBlock work) const {
        _async(&group, qos, flags, work);
    }
    inline void async(const DispatchQoS& qos, DispatchWorkItemFlags flags, DispatchBlock work) const {
        _async(nullptr, qos, flags, work);
    }
    inline void async(DispatchWorkItemFlags flags, DispatchBlock work) const {
        _async(nullptr, DispatchQoS::unspecified(), flags, work);
    }
    inline void async(const DispatchQoS& qos, DispatchBlock work) const {
        _async(nullptr, qos, DispatchWorkItemFlags::NONE, work);
    }
    inline void async(DispatchBlock work) const {
        _async(nullptr, DispatchQoS::unspecified(), DispatchWorkItemFlags::NONE, work);
    }
    inline void async(const DispatchGroup& group, DispatchWorkItemFlags flags, DispatchBlock work) const {
        _async(&group, DispatchQoS::unspecified(), flags, work);
    }
    inline void async(const DispatchGroup& group, DispatchBlock work) const {
        _async(&group, DispatchQoS::unspecified(), DispatchWorkItemFlags::NONE, work);
    }
    inline void async(const DispatchGroup& group, const DispatchQoS& qos, DispatchBlock work) const {
        _async(&group, qos, DispatchWorkItemFlags::NONE, work);
    }

    ///
    /// Submits a block for synchronous execution on this queue.
    ///
    /// Submits a work item to a dispatch queue like `sync(execute:)`, and returns
    /// the value, of type `T`, returned by that work item.
    ///
    /// - parameter execute: The work item to be invoked on the queue.
    /// - returns the value returned by the work item.
    /// - SeeAlso: `sync(execute:)`
    ///
    template <class T>
    inline T sync(DISPATCH_NOESCAPE T (^work)(void)) const {
        T result;
        dispatch_sync(_wrapped, ^(void) {
            result = work();
        });
        return result;
    }
    void sync(DISPATCH_NOESCAPE DispatchBlock work) const;

    ///
    /// Submits a block for synchronous execution on this queue.
    ///
    /// Submits a work item to a dispatch queue like `sync(execute:)`, and returns
    /// the value, of type `T`, returned by that work item.
    ///
    /// - parameter flags: flags that control the execution environment of the
    /// - parameter execute: The work item to be invoked on the queue.
    /// - returns the value returned by the work item.
    /// - SeeAlso: `sync(execute:)`
    /// - SeeAlso: `DispatchWorkItemFlags`
    ///
    template <class T>
    T sync(DispatchWorkItemFlags flags, DISPATCH_NOESCAPE T (^work)(void)) {
        T result;
        if (flags == DispatchWorkItemFlags::BARRIER) {
            dispatch_barrier_sync(_wrapped, ^(void) {
                result = work();
            });
        } else if (flags != DispatchWorkItemFlags::NONE) {
            auto workItem = DispatchWorkItem(flags, ^(void) {
                result = work();
            });
            dispatch_sync(_wrapped, workItem._block);
        } else {
            dispatch_sync(_wrapped, ^(void) {
                result = work();
            });
        }
        return result;
    }
    inline void sync(DispatchWorkItemFlags flags, DISPATCH_NOESCAPE DispatchBlock work) const {
        if (flags == DispatchWorkItemFlags::BARRIER) {
            dispatch_barrier_sync(_wrapped, work);
        } else if (flags != DispatchWorkItemFlags::NONE) {
            auto workItem = DispatchWorkItem(flags, work);
            dispatch_sync(_wrapped, workItem._block);
        } else {
            dispatch_sync(_wrapped, work);
        }
    }
    ///
    /// Submits a work item to a dispatch queue for asynchronous execution after
    /// a specified time.
    ///
    /// - parameter deadline: the time after which the work item should be executed,
    /// given as a `DispatchTime`.
    /// - parameter qos: the QoS at which the work item should be executed.
    ///	Defaults to `DispatchQoS.unspecified`.
    /// - parameter flags: flags that control the execution environment of the
    /// work item.
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `async(execute:)`
    /// - SeeAlso: `asyncAfter(deadline:execute:)`
    /// - SeeAlso: `DispatchQoS`
    /// - SeeAlso: `DispatchWorkItemFlags`
    /// - SeeAlso: `DispatchTime`
    ///
    inline void asyncAfter(
            const DispatchTime& deadline,
            DispatchBlock work,
            const DispatchQoS& qos = DispatchQoS::unspecified(),
            DispatchWorkItemFlags flags = DispatchWorkItemFlags::NONE) const
    {
        if (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE) {
            auto workItem = DispatchWorkItem(work, qos, flags);
            dispatch_after(deadline.rawValue, _wrapped, workItem._block);
        } else {
            dispatch_after(deadline.rawValue, _wrapped, work);
        }
    }

    ///
    /// Submits a work item to a dispatch queue for asynchronous execution after
    /// a specified time.
    ///
    /// - parameter deadline: the time after which the work item should be executed,
    /// given as a `DispatchWallTime`.
    /// - parameter qos: the QoS at which the work item should be executed.
    ///	Defaults to `DispatchQoS.unspecified`.
    /// - parameter flags: flags that control the execution environment of the
    /// work item.
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `async(execute:)`
    /// - SeeAlso: `asyncAfter(wallDeadline:execute:)`
    /// - SeeAlso: `DispatchQoS`
    /// - SeeAlso: `DispatchWorkItemFlags`
    /// - SeeAlso: `DispatchWallTime`
    ///
    void asyncAfter(
            DispatchWallTime wallDeadline,
            DispatchBlock work,
            const DispatchQoS& qos = DispatchQoS::unspecified(),
            DispatchWorkItemFlags flags = DispatchWorkItemFlags::NONE) const
    {
        if (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE) {
            auto workItem = DispatchWorkItem(work, qos, flags);
            dispatch_after(wallDeadline.rawValue, _wrapped, workItem._block);
        } else {
            dispatch_after(wallDeadline.rawValue, _wrapped, work);
        }
    }

    ///
    /// Submits a work item to a dispatch queue for asynchronous execution after
    /// a specified time.
    ///
    /// - parameter deadline: the time after which the work item should be executed,
    /// given as a `DispatchTime`.
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `asyncAfter(deadline:qos:flags:execute:)`
    /// - SeeAlso: `DispatchTime`
    ///
    inline void asyncAfter(DispatchTime deadline, const DispatchWorkItem& execute) const {
        dispatch_after(deadline.rawValue, _wrapped, execute._block);
    }

    ///
    /// Submits a work item to a dispatch queue for asynchronous execution after
    /// a specified time.
    ///
    /// - parameter deadline: the time after which the work item should be executed,
    /// given as a `DispatchWallTime`.
    /// - parameter execute: The work item to be invoked on the queue.
    /// - SeeAlso: `asyncAfter(wallDeadline:qos:flags:execute:)`
    /// - SeeAlso: `DispatchTime`
    ///
    inline void asyncAfter(DispatchWallTime wallDeadline, const DispatchWorkItem& execute) const {
        dispatch_after(wallDeadline.rawValue, _wrapped, execute._block);
    }

    [[nodiscard]] inline DispatchQoS qos() const {
        int relPri = 0;
        auto rawQoS = dispatch_queue_get_qos_class(_wrapped, &relPri);
        auto cls = DispatchQoS::QoSClass(rawQoS);
        return {cls, relPri};
    }

    inline void apply(size_t iterations, DISPATCH_NOESCAPE void (^work)(size_t)) const {
        dispatch_apply(iterations, _wrapped, ^(size_t i) {
            work(i);
        });
    }


    dispatch_queue_t _wrapped {nullptr};
private:
    explicit DispatchQueue(dispatch_queue_t queue): _wrapped(queue) {}
    DispatchQueue(const std::string& label, dispatch_queue_attr_t _Nullable attr);
    DispatchQueue(const std::string& label, dispatch_queue_attr_t _Nullable attr, const DispatchQueue* _Nullable queue);

    void _async(
            const DispatchGroup* _Nullable group,
            const DispatchQoS& qos,
            DispatchWorkItemFlags flags,
            DispatchBlock work
    ) const;

    inline dispatch_object_t wrapped() override {
        return _wrapped;
    }

    friend class DispatchObject;
    friend class DispatchIO;
    friend class DispatchWorkItem;
    friend class DispatchGroup;
    friend class DispatchSource;
    friend class DispatchData;
};

inline void dispatchPrecondition(DispatchPredicate condition, const DispatchQueue& queue) {
#ifndef NDEBUG
    queue.preconditionTest(condition);
#endif
}
