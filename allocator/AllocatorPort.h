#ifndef _ALLOCATOR_PORT_H
#define _ALLOCATOR_PORT_H

/// @file AllocatorPort.h
/// @brief Platform glue for the fixed-block allocator: the mutex, fault and
/// out-of-memory hooks it needs. Port these for a target without std::mutex
/// or exceptions.
///
/// The allocator sources come from DelegateMQ (https://github.com/DelegateMQ/DelegateMQ)
/// and keep its names (namespace dmq, DMQ_* macros), so they can be re-synced
/// by copying. They need nothing from DelegateMQ beyond this file.

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <new>

namespace dmq {
    using Mutex = std::mutex;

    template <class T>
    using LockGuard = std::lock_guard<T>;
}

/// Unrecoverable fault (allocator corruption, capacity exceeded). Checked in
/// every build type.
#ifndef DMQ_ASSERT
#define DMQ_ASSERT() std::abort()
#endif

#ifndef DMQ_ASSERT_TRUE
#define DMQ_ASSERT_TRUE(x) do { if (!(x)) DMQ_ASSERT(); } while (0)
#endif

/// Out of memory: throws std::bad_alloc, or faults when exceptions are off.
#ifndef BAD_ALLOC
#if defined(__cpp_exceptions) && !defined(DMQ_ASSERTS)
#define BAD_ALLOC() throw std::bad_alloc()
#else
#define BAD_ALLOC() DMQ_ASSERT()
#endif
#endif

/// Max number of block-size allocators xallocator creates (one per power of two
/// in use) in heap blocks mode.
#ifndef DMQ_XALLOCATOR_MAX_ALLOCATORS
#define DMQ_XALLOCATOR_MAX_ALLOCATORS 15
#endif

#endif // _ALLOCATOR_PORT_H
