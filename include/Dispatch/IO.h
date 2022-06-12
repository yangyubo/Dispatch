//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include "Dispatch/Object.h"
#include "Dispatch/Queue.h"
#include "Dispatch/Data.h"
#include <dispatch/dispatch.h>

class DispatchIO: DispatchObject {

public:

    enum class StreamType: uint {
        STREAM = 0, RANDOM = 1
    };

    enum class CloseFlags: uint {
        NONE = 0,
        STOP = 1
    };

    enum class IntervalFlags: uint {
        NONE            = 0,
        STRICT_INTERVAL = 1
    };

    DispatchIO(const DispatchIO& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    DispatchIO& operator= (const DispatchIO& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    inline DispatchIO(DispatchIO&& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    inline DispatchIO& operator= (DispatchIO&& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }
    inline ~DispatchIO() override {
        if (_wrapped) {
            dispatch_release(_wrapped);
        }
    }

    inline DispatchIO(
            StreamType type,
            int fileDescriptor,
            const DispatchQueue& queue,
            void (^cleanupHandler)(int error)
            ): DispatchIO(uint(type), fileDescriptor, queue, cleanupHandler) {}

    inline DispatchIO(
            StreamType type,
            const std::string& path,
            int oflag,
            mode_t mode,
            const DispatchQueue& queue,
            void (^cleanupHandler)(int error)
    ): DispatchIO(uint(type), path.c_str(), oflag, mode, queue, cleanupHandler) {}


    inline static void read(int fromFileDescriptor, int maxLength, const DispatchQueue& runningQueue, \
                            void (^handler)(const std::shared_ptr<DispatchData>, int))
    {
        dispatch_read(
                dispatch_fd_t(fromFileDescriptor),
                maxLength,
                runningQueue._wrapped,
                ^(dispatch_data_t data, int error) {
                    std::shared_ptr<DispatchData> borrowedData = nullptr;
                    borrowedData.reset(new DispatchData(data, false));
                    handler(borrowedData, error);
                });
    }

    inline void read(
            off_t offset,
            int length,
            const DispatchQueue& queue,
            void (^ioHandler)(bool, const std::shared_ptr<DispatchData>, int))
    {
        dispatch_io_read(
                _wrapped,
                offset,
                length,
                queue._wrapped,
                ^(bool done, dispatch_data_t _Nullable data, int error)
                {
            if (data == nullptr) {
                ioHandler(done, nullptr, error);
            } else {
                std::shared_ptr<DispatchData> borrowedData = nullptr;
                borrowedData.reset(new DispatchData(data, false));
                ioHandler(done, borrowedData, error);
            }
        });
    }

    inline static void write(
            int toFileDescriptor,
            const DispatchData& wdata,
            const DispatchQueue& runningQueue,
            void (^handler)(const std::shared_ptr<DispatchData> data, int error))
    {
        dispatch_write(
                dispatch_fd_t(toFileDescriptor),
                wdata._wrapped,
                runningQueue._wrapped,
                ^(dispatch_data_t _Nullable data, int error) {
                    if (data == nullptr) {
                        handler(nullptr, error);
                    } else {
                        std::shared_ptr<DispatchData> borrowedData = nullptr;
                        borrowedData.reset(new DispatchData(data, false));
                        handler(borrowedData, error);
                    }
                });
    }

    inline void write(
            off_t offset,
            const DispatchData& wdata,
            const DispatchQueue& queue,
            void (^ioHandler)(bool, const std::shared_ptr<DispatchData>, int))
    {
        dispatch_io_write(
                _wrapped,
                offset,
                wdata._wrapped,
                queue._wrapped,
                ^(bool done, dispatch_data_t _Nullable data, int error) {
                    if (data == nullptr) {
                        ioHandler(done, nullptr, error);
                    } else {
                        std::shared_ptr<DispatchData> borrowedData = nullptr;
                        borrowedData.reset(new DispatchData(data, false));
                        ioHandler(done, borrowedData, error);
                    }
                });
    }

    inline void setInterval(const DispatchTimeInterval& interval, IntervalFlags flags = IntervalFlags::NONE) {
        dispatch_io_set_interval(_wrapped, interval.rawValue, dispatch_io_interval_flags_t(flags));
    }

    inline void close(CloseFlags flags = CloseFlags::NONE) {
        dispatch_io_close(_wrapped, dispatch_io_close_flags_t(flags));
    }

    inline void barrier(void (^execute)(void)) const {
        dispatch_io_barrier(_wrapped, execute);
    }

    [[nodiscard]] inline int fileDescriptor() const {
        return dispatch_io_get_descriptor(_wrapped);
    }

    inline void setHighWater(int limit) const {
        dispatch_io_set_high_water(_wrapped, limit);
    }

    inline void setLowWater(int limit) const {
        dispatch_io_set_low_water(_wrapped, limit);
    }

private:
    DispatchIO(uint type, dispatch_fd_t fd, const DispatchQueue &queue, void (^handler)(int error));
    DispatchIO(uint type, const char *path, int oflag, mode_t mode, const DispatchQueue &queue, void (^handler)(int error));
    DispatchIO(uint type, const DispatchIO& io, const DispatchQueue &queue, void (^handler)(int error));

    dispatch_io_t _wrapped {nullptr};

    inline dispatch_object_t wrapped() override {
        return _wrapped;
    }

};

