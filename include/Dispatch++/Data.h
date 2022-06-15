//------------------------------------------------------------------------------
// This source file is part of the Dispatch++ open source project
//
// Copyright (c) 2022 - 2022 Dispatch++ authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once
#include <dispatch/dispatch.h>
#include "Dispatch++/Object.h"

class DispatchIO;

class DispatchData: DispatchObject {

public:

    // A dispatch data object representing a zero-length memory region.
    inline DispatchData() {
        _wrapped = dispatch_data_empty;
        dispatch_retain(_wrapped);
    }

    DispatchData(const DispatchData& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    DispatchData& operator= (const DispatchData& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }

    inline DispatchData(DispatchData&& other) noexcept {
        _wrapped = other._wrapped;
        dispatch_retain(_wrapped);
    }
    inline DispatchData& operator= (DispatchData&& other) noexcept {
        if (this != &other) {
            _wrapped = other._wrapped;
            dispatch_retain(_wrapped);
        }
        return *this;
    }
    inline ~DispatchData() override {
        if (_wrapped) {
            dispatch_release(_wrapped);
        }
    }

    enum struct Deallocator: uint8_t {
        FREE, UNMAP
    };

    /// Initialize a `Data` with copied memory content.
    ///
    /// - parameter bytes: A pointer to the memory. It will be copied.
    /// - parameter count: The number of bytes to copy.
    DispatchData(const void *bytes, size_t count);

    /// Initialize a `Data` without copying the bytes.
    ///
    /// - parameter bytesNoCopy: A pointer to the bytes.
    /// - parameter count: The size of the bytes.
    /// - parameter deallocator: Specifies the mechanism to free the indicated buffer.
    DispatchData(const void *bytesNoCopy, int count, Deallocator deallocator);
    DispatchData(const void *bytesNoCopy, int count, const DispatchQueue &queue, Deallocator deallocator);

    DispatchData(const void *bytesNoCopy, int count, const DispatchQueue &queue, void (^deallocator)(void));

    [[nodiscard]] inline size_t count() const {
        return dispatch_data_get_size(_wrapped);
    }

    /// Append bytes to the data.
    ///
    /// - parameter bytes: A pointer to the bytes to copy in to the data.
    /// - parameter count: The number of bytes to copy.
    void append(const void *bytes, size_t count);

    /// Append data to the data.
    ///
    /// - parameter data: The data to append to this data.
    void append(const DispatchData& other);

    /// Copy the contents of the data to a pointer.
    ///
    /// - parameter toPointer: A pointer to the buffer you wish to copy the bytes into. The buffer must be large
    ///	enough to hold `count` bytes.
    /// - parameter count: The number of bytes to copy.
    inline void copyBytes(void *toPointer, int count) {
        if (toPointer == nullptr) { return; }
        _copyBytesHelper(toPointer, 0, count);
    }

    /// Copy a subset of the contents of the data to a pointer.
    ///
    /// - parameter toPointer: A pointer to the buffer you wish to copy the bytes into. The buffer must be large
    ///	enough to hold `count` bytes.
    /// - parameter range: The range in the `Data` to copy.
    inline void copyBytes(void *toPointer, int startIndex, int endIndex) {
        if (toPointer == nullptr) { return; }
        _copyBytesHelper(toPointer, startIndex, endIndex);
    }


    /// Sets or returns the byte at the specified index.
    uint8_t operator[](int index);

    /// Return a new copy of the data in a specified range.
    ///
    /// - parameter range: The range to copy.
    inline DispatchData subdata(int startIndex, int endIndex) {
        auto length = endIndex - startIndex;
        auto subrange = dispatch_data_create_subrange(_wrapped, startIndex, length);
        return DispatchData(subrange);
    }

    inline DispatchData region(int location, int& offset) {
        size_t loffset = 0;
        auto data = dispatch_data_copy_region(_wrapped, location, &loffset);
        offset = int(loffset);
        return DispatchData(data);
    }

    void withUnsafeBytes(DISPATCH_NOESCAPE void (^action)(const void *bytes, size_t count)) {
        const void* ptr = nullptr;
        size_t size = 0;

        auto data = dispatch_data_create_map(_wrapped, &ptr, &size);
        action(ptr, size);

        dispatch_release(data);
    }

    template<typename Result>
    Result withUnsafeBytes(DISPATCH_NOESCAPE Result (^action)(const void *bytes, size_t count)) {
        const void* ptr = nullptr;
        size_t size = 0;

        auto data = dispatch_data_create_map(_wrapped, &ptr, &size);
        auto result = action(ptr, size);

        dispatch_release(data);
        return result;
    }

private:

    dispatch_data_t _wrapped {nullptr};

    inline dispatch_object_t wrapped() override {
        return _wrapped;
    }

    inline explicit DispatchData(dispatch_data_t data, bool owned = true) {
        _wrapped = data;
        if (!owned) {
            dispatch_retain(data);
        }
    }

    void _copyBytesHelper(void *toPointer, int startIndex, int endIndex);

    friend DispatchIO;
};
