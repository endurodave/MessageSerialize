# Allocator Suite

A robust, fixed-block memory allocator suite for C++ applications, designed for performance and deterministic behavior in resource-constrained or real-time systems.

## Features

- **Fixed-Block Allocation**: Prevents heap fragmentation by using pre-allocated pools of equal-sized blocks.
- **C++ Class Integration**: `DECLARE_ALLOCATOR` and `IMPLEMENT_ALLOCATOR` macros provide easy integration with custom classes.
- **XALLOCATOR Macro**: Overloads `operator new` and `operator delete` for seamless use.
- **STL Compatibility**: `stl_allocator<T>` allows STL containers (`std::vector`, `std::list`, `std::map`, etc.) to use the fixed-block pool.
- **Smart Pointer Support**: `xmake_shared<T>` creates `std::shared_ptr` with both the object and control block allocated from the pool.
- **C API**: `xmalloc`, `xfree`, and `xrealloc` functions for C-style memory management.

## Components

- **Allocator**: The core engine for fixed-block management.
- **AllocatorPool**: A template-based pool for specific types.
- **stl_allocator**: STL-compliant allocator wrapper.
- **xmake_shared**: Helper for pool-allocated shared pointers.
- **xnew / xdelete**: Helper templates for placement new/destroy in the pool.

## Basic Usage

### Using XALLOCATOR in a class

```cpp
class MyClass {
    XALLOCATOR
public:
    int data;
};

// Now 'new MyClass' uses the fixed-block allocator
MyClass* obj = new MyClass();
delete obj;
```

### Using STL containers

```cpp
#include "xlist.h"

// xlist is a typedef for std::list using stl_allocator
xlist<int> myList;
myList.push_back(10);
```

### Using xmake_shared

```cpp
auto sp = xmake_shared<MyClass>(args...);
```

## Memory Alignment

The `xallocator` implementation ensures that all allocated blocks are correctly aligned for the target architecture:

- **Power-of-Two Rounding**: All requested block sizes are rounded up to the next power of two (8, 16, 32, 64, etc.). This inherently satisfies the natural alignment requirements of all standard C++ types (`int`, `double`, `long long`, etc.) and modern SIMD/AVX vector instructions.
- **Header Alignment**: A `BLOCK_HEADER_SIZE` (16 bytes on 64-bit, 8 bytes on 32-bit) is added to each block to store metadata. Both the raw block and the user's data pointer remain aligned on 8 or 16-byte boundaries.
- **Efficiency vs. Safety**: This strategy prioritizes "automatic" alignment and safety over maximum memory density. While it may result in some internal fragmentation, it ensures that memory is always safe for any data type without manual alignment configuration.

## Use with MessageSerialize

These files come from [DelegateMQ](https://github.com/DelegateMQ/DelegateMQ) and keep its names (namespace `dmq`, `DMQ_*` macros) so they can be re-synced by copying, but need nothing from it. `AllocatorPort.h` supplies the mutex, fault and out-of-memory hooks; port it for a target without `std::mutex` or exceptions.

Enable the allocator with the CMake option `MSG_SERIALIZE_ALLOCATOR=ON`, which defines `DMQ_ALLOCATOR` and builds `Allocator.cpp`/`xallocator.cpp`. Include `xtypes.h` and declare message fields with `dmq::xstring`, `dmq::xlist`, `dmq::xmap`, `dmq::xset`: with the option on they use the fixed-block pools, with it off they are the plain `std::` types on the global heap. `msg_serialize.h` serializes both the same way, so the choice does not change the wire format.

| Setting | Default | Meaning |
|---|---|---|
| `DMQ_ALLOCATOR` | off | Use the fixed-block allocator (set by `MSG_SERIALIZE_ALLOCATOR=ON`) |
| `DMQ_ALLOCATOR_SAFEGUARDS` | off | Guard words and checks on every block, to catch overruns and double frees |
| `DMQ_XALLOCATOR_MAX_ALLOCATORS` | 15 | Max block-size pools (one per power of two in use) |
| `STATIC_POOLS` (in `xallocator.cpp`) | off | Static pools of fixed counts instead of heap-backed blocks |
