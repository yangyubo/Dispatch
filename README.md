# C++ Wrapper for Grand Central Dispatch

`Dispatch` provides header-only C++ APIs for [Grand Central Dispatch](https://github.com/apple/swift-corelibs-libdispatch)  (GCD or libdispatch).

## Project Status

`Dispatch` is tested under:

1. macOS 12.3, Apple Silicon
2. GNU Linux 4.9, ARM Cortex A7 and A9, with Busybox.

It should also work on every platform that [libdispatch supports](https://github.com/apple/swift-corelibs-libdispatch/blob/main/INSTALL.md). 

## Install

Clone the repository as a git submodule, or copy `include/Dispatch` folder to your project. 

1. Add `Dispatch` include directory in header search path.
2. [libdispatch](https://github.com/apple/swift-corelibs-libdispatch/) is required.
3. LLVM 12.0 or higher is required, enable C++20 features.

## Features

- The API align with [Swift APIs](https://github.com/apple/swift-corelibs-libdispatch/tree/main/src/swift).
- Headers only.
- Tested.

## Usage

```c++
#include <Dispatch/Dispatch.h>

int main() {
  DispatchQueue queue = DispatchQueue{"dispatch.dispatch-after"};
  
  // after 6 seconds
  auto time_a = DispatchTime::now() + DispatchTimeInterval::seconds(6);
  
  queue.asyncAfter(time_a, ^{
    printf("Hello, World!\n");
  });
}
```

## TODO

- Support `DISPATCH_SOURCE_TYPE_PROC`.

## Documentation

The [official Dispatch documentation for Swift](https://developer.apple.com/documentation/dispatch) can be applied to `Dispatch`.
