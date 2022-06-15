//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include "Data.h"
#include "Queue.h"
#include "Block.h"
#include "Group.h"
#include "IO.h"
#include "Block.h"
#include "QoS.h"
#include "Source.h"
#include "Time.h"
#include "Utils.h"

// MARK: - DispatchBlock

inline void DispatchWorkItem::notify(
        const DispatchQueue &queue,
        DispatchBlock execute,
        const DispatchQoS& qos,
        DispatchWorkItemFlags flags) const
{
    if (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE) {
        auto item = DispatchWorkItem(execute, qos, flags);
        dispatch_block_notify(_block, queue._wrapped, item._block);
    } else {
        dispatch_block_notify(_block, queue._wrapped, execute);
    }
}

inline void DispatchWorkItem::notify(const DispatchQueue &queue, const DispatchWorkItem& work) const {
    dispatch_block_notify(_block, queue._wrapped, work._block);
}


// MARK: - DispatchData

inline DispatchData::DispatchData(const void *bytes, size_t count) {
    _wrapped = bytes == nullptr ? dispatch_data_empty
                                : dispatch_data_create(
                    bytes,
                    count,
                    nullptr,
                    DISPATCH_DATA_DESTRUCTOR_DEFAULT
            );
}

inline DispatchData::DispatchData(const void *bytesNoCopy, int count, Deallocator deallocator) {
    auto block = deallocator == Deallocator::FREE ? _dispatch_data_destructor_free : _dispatch_data_destructor_munmap;
    _wrapped = bytesNoCopy == nullptr ? dispatch_data_empty
                                      : dispatch_data_create(
                    bytesNoCopy,
                    count,
                    nullptr,
                    block
            );
}
inline DispatchData::DispatchData(const void *bytesNoCopy, int count, const DispatchQueue &queue, Deallocator deallocator) {
    auto block = deallocator == Deallocator::FREE ? _dispatch_data_destructor_free : _dispatch_data_destructor_munmap;
    _wrapped = bytesNoCopy == nullptr ? dispatch_data_empty
                                      : dispatch_data_create(
                    bytesNoCopy,
                    count,
                    queue._wrapped,
                    block
            );
}

inline DispatchData::DispatchData(const void *bytesNoCopy, int count, const DispatchQueue &queue, void (^deallocator)(void)) {
    _wrapped = bytesNoCopy == nullptr ? dispatch_data_empty
                                      : dispatch_data_create(
                    bytesNoCopy,
                    count,
                    queue._wrapped,
                    deallocator
            );
}

inline void DispatchData::append(const void *bytes, size_t count) {
    // Nil base address does nothing.
    if (bytes == nullptr) { return; }

    auto data = dispatch_data_create(bytes, count, nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    auto concat_data = dispatch_data_create_concat(_wrapped, data);

    dispatch_release(_wrapped);
    dispatch_release(data);
    _wrapped = concat_data;
}

inline void DispatchData::append(const DispatchData& other) {
    auto data = dispatch_data_create_concat(_wrapped, other._wrapped);
    dispatch_release(_wrapped);
    _wrapped = data;
}

inline void DispatchData::_copyBytesHelper(void *toPointer, int startIndex, int endIndex) {
    __block auto copiedCount = 0;
    auto rangeSize = endIndex - startIndex;
    if (rangeSize <= 0) { return; }

    dispatch_data_apply(_wrapped, ^bool(dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
        if (offset >= endIndex) { return false; } // This region is after endIndex.
        auto copyOffset = startIndex > offset ? startIndex - offset : 0; // offset of first byte, in this region

        if (copyOffset >= size) { return true; } // This region is before startIndex

        auto count = std::min(rangeSize - copiedCount, int(size - copyOffset));
        memcpy((char *)toPointer + copiedCount, (const char*)buffer + copyOffset, count);
        copiedCount += count;

        return copiedCount < rangeSize;
    });
}

inline uint8_t DispatchData::operator[](int index) {
    size_t offset = 0;
    auto subdata = dispatch_data_copy_region(_wrapped, index, &offset);

    const void* ptr = nullptr;
    size_t size = 0;

    auto map = dispatch_data_create_map(subdata, &ptr, &size);
    auto result = ((uint8_t *)ptr)[index-offset];

    dispatch_release(map);
    dispatch_release(subdata);

    return result;
}

// MARK: - DispatchGroup

inline void DispatchGroup::notify(
        const DispatchQueue &queue,
        DispatchBlock work,
        const DispatchQoS& qos,
        DispatchWorkItemFlags flags) const
{
    if (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE) {
        auto item = DispatchWorkItem(work, qos, flags);
        dispatch_group_notify(_wrapped, queue._wrapped, item._block);
    } else {
        dispatch_group_notify(_wrapped, queue._wrapped, work);
    }
}

inline void DispatchGroup::notify(const DispatchQueue &queue, const DispatchWorkItem& work) const {
    dispatch_group_notify(_wrapped, queue._wrapped, work._block);
}

// MARK: - DispatchIO

inline DispatchIO::DispatchIO(uint type, dispatch_fd_t fd, const DispatchQueue &queue, void (^handler)(int)) {
    _wrapped = dispatch_io_create(dispatch_io_type_t(type), dispatch_fd_t(fd), queue._wrapped, handler);
}

inline DispatchIO::DispatchIO(uint type, const char *path, int oflag, mode_t mode, const DispatchQueue &queue, void (^handler)(int error)) {
    _wrapped = dispatch_io_create_with_path(dispatch_io_type_t(type), path, oflag, mode, queue._wrapped, handler);
}

inline DispatchIO::DispatchIO(uint type, const DispatchIO& io, const DispatchQueue &queue, void (^handler)(int error)) {
    _wrapped = dispatch_io_create_with_io(dispatch_io_type_t(type), io._wrapped, queue._wrapped, handler);
}

// MARK: - DispatchObject

inline void DispatchObject::setTarget(const DispatchQueue& queue)  {
    dispatch_set_target_queue(wrapped(), queue._wrapped);
}


// MARK: - DispatchQoS

inline DispatchQoS& DispatchQoS::background() {
    static DispatchQoS value(DispatchQoS::QoSClass::BACKGROUND, 0);
    return value;
}

inline DispatchQoS& DispatchQoS::utility() {
    static DispatchQoS value(DispatchQoS::QoSClass::UTILITY, 0);
    return value;
}

inline DispatchQoS& DispatchQoS::default_qos() {
    static DispatchQoS value(DispatchQoS::QoSClass::DEFAULT, 0);
    return value;
}

inline DispatchQoS& DispatchQoS::userInitiated() {
    static DispatchQoS value(DispatchQoS::QoSClass::USER_INITIATED, 0);
    return value;
}

inline DispatchQoS& DispatchQoS::userInteractive() {
    static DispatchQoS value(DispatchQoS::QoSClass::USER_INTERACTIVE, 0);
    return value;
}

inline DispatchQoS& DispatchQoS::unspecified() {
    static DispatchQoS value(DispatchQoS::QoSClass::UNSPECIFIED, 0);
    return value;
}

inline bool DispatchQoS::operator==(const DispatchQoS &other) const {
    return qosClass == other.qosClass && relativePriority == other.relativePriority;
}

// MARK: - DispatchQueue

inline static dispatch_queue_attr_t convert_to_dispatch_attr(DispatchQueue::Attributes attributes) {
    dispatch_queue_attr_t attr = nullptr;

    if (uint64_t(attributes) & uint64_t(DispatchQueue::Attributes::CONCURRENT)) {
        attr = DISPATCH_QUEUE_CONCURRENT;
    }

    if (uint64_t(attributes) & uint64_t(DispatchQueue::Attributes::INITIALLY_INACTIVE)) {
        attr = dispatch_queue_attr_make_initially_inactive(attr);
    }

    return attr;
}

inline DispatchQueue::DispatchQueue(
        const std::string& label,
        const DispatchQoS& qos,
        Attributes attributes,
        AutoreleaseFrequency autoreleaseFrequency,
        const DispatchQueue* _Nullable queue)
{
    auto attr = convert_to_dispatch_attr(attributes);

    if (autoreleaseFrequency != AutoreleaseFrequency::INHERIT) {
        auto frequency = dispatch_autorelease_frequency_t(autoreleaseFrequency);
        attr = dispatch_queue_attr_make_with_autorelease_frequency(attr, frequency);
    }

    if (qos != DispatchQoS::unspecified()) {
        attr = dispatch_queue_attr_make_with_qos_class(attr, dispatch_qos_class_t(qos.qosClass), qos.relativePriority);
    }

    auto target = queue == nullptr ? nullptr : queue->_wrapped;
    if (target) {
        _wrapped = dispatch_queue_create_with_target(label.c_str(), attr, target);
    } else {
        _wrapped = dispatch_queue_create(label.c_str(), attr);
    }
}

inline DispatchQueue::DispatchQueue(const std::string& label, dispatch_queue_attr_t _Nullable attr) {
    _wrapped = dispatch_queue_create(label.c_str(), attr);
}

inline DispatchQueue::DispatchQueue(const std::string& label, dispatch_queue_attr_t _Nullable attr, const DispatchQueue* _Nullable queue) {
    auto target = queue == nullptr ? nullptr : queue->_wrapped;
    _wrapped = dispatch_queue_create_with_target(label.c_str(), attr, target);
}

inline void DispatchQueue::sync(DISPATCH_NOESCAPE DispatchBlock work) const {
    dispatch_sync(_wrapped, work);
}

inline void DispatchQueue::_async(
        const DispatchGroup* _Nullable group,
        const DispatchQoS& qos,
        DispatchWorkItemFlags flags,
        DispatchBlock work) const
{
    if (group == nullptr && qos == DispatchQoS::unspecified()) {
        // Fast-path route for the most common API usage
        if (flags == DispatchWorkItemFlags::NONE) {
            dispatch_async(_wrapped, work);
        } else if (flags == DispatchWorkItemFlags::BARRIER) {
            dispatch_barrier_async(_wrapped, work);
        }

        return;
    }

    auto block = work;
    if (qos != DispatchQoS::unspecified() || flags != DispatchWorkItemFlags::NONE) {
        auto workItem = DispatchWorkItem(work, qos, flags);
        block = workItem._block;
    }

    if (group != nullptr) {
        dispatch_group_async(group->_wrapped, _wrapped, block);
    } else {
        dispatch_async(_wrapped, block);
    }
}

inline bool DispatchQueue::_dispatchPreconditionTest(DispatchPredicate condition) const {
    switch (condition) {
        case DispatchPredicate::ON_QUEUE:
            dispatch_assert_queue(_wrapped);
            break;
        case DispatchPredicate::ON_QUEUE_AS_BARRIER:
            dispatch_assert_queue_barrier(_wrapped);
            break;
        case DispatchPredicate::NOT_ON_QUEUE:
            dispatch_assert_queue_not(_wrapped);
            break;
    }
    return true;
}

inline void DispatchQueue::dispatchPrecondition(DispatchPredicate condition) const {
  DISPATCH_ASSERT(_dispatchPreconditionTest(condition), "Precondition failed");
}

inline void DispatchQueue::async(const DispatchGroup& group, const DispatchWorkItem& workItem) const {
    dispatch_group_async(group._wrapped, _wrapped, workItem._block);
}

// MARK: - DispatchSource

inline std::shared_ptr<DispatchSourceRead> DispatchSource::makeReadSource(int32_t fileDescriptor, const DispatchQueue *queue)
{
    auto handle = uint(fileDescriptor);
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, handle, 0, wrapped);

    return std::shared_ptr<DispatchSourceRead>(new DispatchSource(source));
}

inline std::shared_ptr<DispatchSourceSignal> DispatchSource::makeSignalSource(int32_t signal, const DispatchQueue *queue) {
    auto handle = uint(signal);
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, handle, 0, wrapped);

    return std::shared_ptr<DispatchSourceSignal>(new DispatchSource(source));
}

inline std::shared_ptr<DispatchSourceTimer> DispatchSource::makeTimerSource(TimerFlags flags, const DispatchQueue *queue) {
    auto handle = uint(flags);
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, handle, 0, wrapped);

    return std::shared_ptr<DispatchSourceTimer>(new DispatchSource(source));
}

inline std::shared_ptr<DispatchSourceUserDataAdd> DispatchSource::makeUserDataAddSource(const DispatchQueue *queue) {
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0, wrapped);

    return std::shared_ptr<DispatchSourceUserDataAdd>(new DispatchSource(source));
}

inline std::shared_ptr<DispatchSourceUserDataOr> DispatchSource::makeUserDataOrSource(const DispatchQueue *queue) {
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_OR, 0, 0, wrapped);

    return std::shared_ptr<DispatchSourceUserDataOr>(new DispatchSource(source));
}

inline std::shared_ptr<DispatchSourceUserDataReplace> DispatchSource::makeUserDataReplaceSource(const DispatchQueue *queue) {
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_REPLACE, 0, 0, wrapped);

    return std::shared_ptr<DispatchSourceUserDataReplace>(new DispatchSource(source));
}

inline std::shared_ptr<DispatchSourceWrite> DispatchSource::makeWriteSource(int32_t fileDescriptor, const DispatchQueue *queue)
{
    auto handle = uint(fileDescriptor);
    auto wrapped = queue == nullptr ? nullptr : queue->_wrapped;
    auto source = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, handle, 0, wrapped);

    return std::shared_ptr<DispatchSourceWrite>(new DispatchSource(source));
}

// MARK: - DispatchTime

inline DispatchTime& DispatchTime::distantFuture() {
    static DispatchTime value(dispatch_time(DISPATCH_TIME_FOREVER, 0));
    return value;
}

inline DispatchWallTime &DispatchWallTime::distantFuture() {
    static DispatchWallTime value(~0ull);
    return value;
}

// Returns m1 * m2, clamped to the range [Int64.min, Int64.max].
// Because of the way this function is used, we can always assume
// that m2 > 0.
inline static int64_t clampedInt64Product(int64_t m1, int64_t m2) {
    DISPATCH_ASSERT(m2 > 0, "multiplier must be positive");
    int64_t p = m1 * m2;

    // Catch and compute overflow during multiplication of two large integers:
    // https://stackoverflow.com/a/1815371/1371716
    if (m1 != 0 && (p / m1) != m2) {
        // Overflow, clamp the result to the range [Int64.min, Int64.max].
        return (m1 > 0) ? INT64_MAX : INT64_MIN;
    }
    return  p;
}

// Returns its argument clamped to the range [Int64.min, Int64.max].
inline static int64_t toInt64Clamped(double value) {
    if (std::isnan(value)) {
        return INT64_MAX;
    }

    if (value >= double(INT64_MAX)) {
        return INT64_MAX;
    }

    if (value <= double(INT64_MIN)) {
        return INT64_MIN;
    }

    return int64_t(value);
}


inline DispatchTimeInterval DispatchTimeInterval::seconds(int64_t s) {
    auto i = clampedInt64Product(int64_t(s), int64_t(NSEC_PER_SEC));
    return DispatchTimeInterval(i);
}

inline DispatchTimeInterval DispatchTimeInterval::milliseconds(int64_t ms) {
    auto i = clampedInt64Product(int64_t(ms), int64_t(NSEC_PER_MSEC));
    return DispatchTimeInterval(i);
}

inline DispatchTimeInterval DispatchTimeInterval::microseconds(int64_t us) {
    auto i = clampedInt64Product(int64_t(us), int64_t(NSEC_PER_USEC));
    return DispatchTimeInterval(i);
}

inline DispatchTimeInterval DispatchTimeInterval::nanoseconds(int64_t ns) {
    return DispatchTimeInterval(ns);
}

inline DispatchTimeInterval& DispatchTimeInterval::never() {
    static DispatchTimeInterval value(INT64_MAX);
    return value;
}

inline bool DispatchTimeInterval::operator==(const DispatchTimeInterval &other) const {
    return false;
}

// MARK: - DispatchTime operators

inline DispatchTime operator+(DispatchTime time, DispatchTimeInterval interval) {
    auto t = dispatch_time(time.rawValue, interval.rawValue);
    return DispatchTime(t);
}

inline DispatchTime operator-(DispatchTime time, DispatchTimeInterval interval) {
    auto t = dispatch_time(time.rawValue, -interval.rawValue);
    return DispatchTime(t);
}

inline DispatchTime operator+(DispatchTime time, double seconds) {
    auto t = dispatch_time(time.rawValue, toInt64Clamped(seconds * double(NSEC_PER_SEC)));
    return DispatchTime(t);
}
inline DispatchTime operator-(DispatchTime time, double seconds) {
    auto t = dispatch_time(time.rawValue, toInt64Clamped(-seconds * double(NSEC_PER_SEC)));
    return DispatchTime(t);
}

// MARK: - DispatchWallTime operators

inline DispatchWallTime operator+(DispatchWallTime time, DispatchTimeInterval interval) {
    auto t = dispatch_time(time.rawValue, interval.rawValue);
    return DispatchWallTime(t);
}

inline DispatchWallTime operator-(DispatchWallTime time, DispatchTimeInterval interval) {
    auto t = dispatch_time(time.rawValue, -interval.rawValue);
    return DispatchWallTime(t);
}

inline DispatchWallTime operator+(DispatchWallTime time, double seconds) {
    auto t = dispatch_time(time.rawValue, toInt64Clamped(seconds * double(NSEC_PER_SEC)));
    return DispatchWallTime(t);
}
inline DispatchWallTime operator-(DispatchWallTime time, double seconds) {
    auto t = dispatch_time(time.rawValue, toInt64Clamped(-seconds * double(NSEC_PER_SEC)));
    return DispatchWallTime(t);
}
