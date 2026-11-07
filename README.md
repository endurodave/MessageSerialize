![License MIT](https://img.shields.io/github/license/BehaviorTree/BehaviorTree.CPP?color=blue)
[![conan Ubuntu](https://github.com/endurodave/MessageSerialize/actions/workflows/cmake_ubuntu.yml/badge.svg)](https://github.com/endurodave/MessageSerialize/actions/workflows/cmake_ubuntu.yml)
[![conan Ubuntu](https://github.com/endurodave/MessageSerialize/actions/workflows/cmake_clang.yml/badge.svg)](https://github.com/endurodave/MessageSerialize/actions/workflows/cmake_clang.yml)
[![conan Windows](https://github.com/endurodave/MessageSerialize/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/endurodave/MessageSerialize/actions/workflows/cmake_windows.yml)

# Message Serialize
A simple serialize class to binary serialize and deserialize C++ objects.

## Introduction

`MessageSerialize` is a lightweight, header-only C++14 library for binary serialization. It provides a simple framework to serialize and deserialize C++ objects and STL containers to/from binary streams, suitable for communication protocols, data storage, or IPC.

### Key Features
* **Simple**: Single header-only implementation, no dependencies beyond the C++ standard library.
* **Easy to Use**: Inherit from `serialize::I` and override `read()` and `write()`.
* **Broad Support**: Primitives, `std::string`, `std::wstring`, `char[]`, and STL containers (`vector`, `array`, `map`, `list`, `set`), including nested containers (e.g. `std::vector<std::vector<int>>`, `std::map<std::string, std::list<int>>`) and containers of strings.
* **Custom Allocators**: Strings and containers with any standard-compliant allocator serialize the same as the `std::` defaults.
* **Bounded Input**: Configurable string, container and nesting limits, checked on write and on read, so a malformed or hostile message can't make the receiver allocate without bound or recurse too deep.
* **Protocol Resilience**: Forward/backward compatibility automatically handles extra or missing fields.
* **Robust**: Automatic endianness swapping, deep-copy pointer container support, and error reporting.
* **Efficient**: Minimal metadata overhead using standard STL streams.
* **Tested**: Extensive unit tests (`SerializeTests.cpp`) for all supported types, limits and error conditions.

## Example Usage

C++ objects inherit from `serialize::I` to support serialize and deserialize. Methods for serialization (`write`) and deserialization (`read`) are implemented to write/read each object member. `Date` shows a simple example.

```cpp
class Date : public serialize::I
{
public:
    Date() = default;
    Date(int16_t d, int16_t m, int16_t y) : day(d), month(m), year(y) {}
    virtual ~Date() = default;

    virtual ostream& write(serialize& ms, ostream& os) override
    {
        ms.write(os, day);
        ms.write(os, month);
        ms.write(os, year);
        return os;
    }

    virtual istream& read(serialize& ms, istream& is) override
    {
        ms.read(is, day);
        ms.read(is, month);
        ms.read(is, year);
        return is;
    }

    int16_t day = 0;
    int16_t month = 0;
    int16_t year = 0;
};
```

`Log` uses `Date` as a data member. 

```cpp
class Log : public serialize::I
{
public:
    enum class LogType : uint16_t { ALARM, DIAGNOSTIC };

    virtual ostream& write(serialize& ms, ostream& os) override
    {
        ms.write(os, logType);
        ms.write(os, date);
        return os;
    }

    virtual istream& read(serialize& ms, istream& is) override
    {
        ms.read(is, logType);
        ms.read(is, date);
        return is;
    }

    LogType logType = LogType::ALARM;
    Date date;
};
```

`AlarmLog` inherits from the `Log` base class showing how complex data hierarchies can be constructed.

```cpp
class AlarmLog : public Log
{
public:
    virtual ostream& write(serialize& ms, ostream& os) override
    {
        Log::write(ms, os);
        ms.write(os, alarmValue);
        return os;
    }

    virtual istream& read(serialize& ms, istream& is) override
    {
        Log::read(ms, is);
        ms.read(is, alarmValue);
        return is;
    }

    uint32_t alarmValue = 0;
};
```

The code snippet below demonstrates creating, transmitting, and receiving a binary-encoded `AlarmLog` message. At runtime, serialize and deserialize an `AlarmLog` object to a `std::stringstream`. Check stream `good()` to ensure parsing success before using object data. The data bytes within the stream can be sent as a communication protocol payload, for instance.

```cpp
// Create a serialize instance
static serialize ms;

// Create an AlarmLog message
AlarmLog writeLog;
writeLog.date.day = 31;
writeLog.date.month = 12;
writeLog.date.year = 2024;
writeLog.logType = Log::LogType::ALARM;
writeLog.alarmValue = 0x11223344;

// Write log to stringstream
stringstream ss(ios::in | ios::out | ios::binary);
ms.write(ss, writeLog);

// Copy outgoing stringstream data bytes to a raw character buffer for transmission
auto size = ss.tellp();
char* binary_buf = static_cast<char*>(malloc(size));
ss.rdbuf()->sgetn(binary_buf, size);

// Alternatively, don't copy and just send data directly from stringstream 
//extern void SendMsg(const void* data, uint32_t dataSize);
//SendMsg(ss.str().c_str(), ss.tellp());

// TODO: Send binary_buf to somewhere
// TODO: Receive binary_buf from somewhere

// Convert incoming bytes to a stream for parsing
istringstream is(std::string(binary_buf, size), std::ios::in | std::ios::binary);

// Read log from stringstream
AlarmLog readLog;
ms.read(is, readLog);
if (is.good())
{
    // Parse succeeded; use readLog values
    cout << "AlarmLog Parse Success! " << readLog.alarmValue << endl;
}
else
{
    cout << "ERROR: AlarmLog" << endl;
}

free(binary_buf);
```

`AllData` shows more examples using C++ container classes such as `std::list`, character arrays, and more.

```cpp
class AllData : public serialize::I
{
public:
    AllData() = default;
    virtual ~AllData()
    {
        for (auto& ptr : dataVectorPtr) 
            delete ptr;
        dataVectorPtr.clear();

        for (auto& ptr : dataListPtr)
            delete ptr;
        dataListPtr.clear();

        for (auto& ptr : dataMapPtr)
            delete ptr.second;
        dataMapPtr.clear();

        for (auto& ptr : dataSetPtr)
            delete ptr;
        dataSetPtr.clear();
    }

    AllData(const AllData& other) = delete;
    AllData& operator=(const AllData& other) = delete;

    virtual ostream& write(serialize& ms, ostream& os)
    {
        ms.write(os, valueInt);
        ms.write(os, valueInt8);
        ms.write(os, valueInt16);
        ms.write(os, valueInt32);
        ms.write(os, valueInt64);
        ms.write(os, valueUInt8);
        ms.write(os, valueUInt16);
        ms.write(os, valueUInt32);
        ms.write(os, valueUInt64);
        ms.write(os, valueFloat);
        ms.write(os, valueDouble);
        ms.write(os, color);
        ms.write(os, cstr);
        ms.write(os, str);
        ms.write(os, wstr);
        ms.write(os, dataVectorBool);
        ms.write(os, dataVectorFloat);
        ms.write(os, dataVectorPtr);
        ms.write(os, dataVectorValue);
        ms.write(os, dataVectorInt);
        ms.write(os, dataListPtr);
        ms.write(os, dataListValue);
        ms.write(os, dataListInt);
        ms.write(os, dataMapPtr);
        ms.write(os, dataMapValue);
        ms.write(os, dataMapInt);
        ms.write(os, dataSetPtr);
        ms.write(os, dataSetValue);
        ms.write(os, dataSetInt);
        return os;
    }

    virtual istream& read(serialize& ms, istream& is)
    {
        ms.read(is, valueInt);
        ms.read(is, valueInt8);
        ms.read(is, valueInt16);
        ms.read(is, valueInt32);
        ms.read(is, valueInt64);
        ms.read(is, valueUInt8);
        ms.read(is, valueUInt16);
        ms.read(is, valueUInt32);
        ms.read(is, valueUInt64);
        ms.read(is, valueFloat);
        ms.read(is, valueDouble);
        ms.read(is, color);
        ms.read(is, cstr, sizeof(cstr));
        ms.read(is, str);
        ms.read(is, wstr);
        ms.read(is, dataVectorBool);
        ms.read(is, dataVectorFloat);
        ms.read(is, dataVectorPtr);
        ms.read(is, dataVectorValue);
        ms.read(is, dataVectorInt);
        ms.read(is, dataListPtr);
        ms.read(is, dataListValue);
        ms.read(is, dataListInt);
        ms.read(is, dataMapPtr);
        ms.read(is, dataMapValue);
        ms.read(is, dataMapInt);
        ms.read(is, dataSetPtr);
        ms.read(is, dataSetValue);
        ms.read(is, dataSetInt);
        return is;
    }

    int valueInt = 4;
    int8_t valueInt8 = 8;
    int16_t valueInt16 = 16;
    int32_t valueInt32 = 32;
    int64_t valueInt64 = 64;
    uint8_t valueUInt8 = 8;
    uint16_t valueUInt16 = 16;
    uint32_t valueUInt32 = 32;
    uint64_t valueUInt64 = 64;
    float valueFloat = 1.23f;
    double valueDouble = 3.21;
    Color color = Color::BLUE;
    char cstr[32] = { 0 };
    string str;
    wstring wstr;
    vector<bool> dataVectorBool;
    vector<float> dataVectorFloat;
    vector<Date*> dataVectorPtr;
    vector<Date> dataVectorValue;
    vector<int> dataVectorInt;
    list<Date*> dataListPtr;
    list<Date> dataListValue;
    list<int> dataListInt;
    map<int, Date*> dataMapPtr;
    map<int, Date> dataMapValue;
    map<int, int> dataMapInt;
    set<Date*> dataSetPtr;
    set<Date> dataSetValue;
    set<int> dataSetInt;
};
```

See `main.cpp` for more examples. 

## Building and Testing

```
cmake -B build .
cmake --build build
./build/Serializer
```

`Serializer` runs the examples in `main.cpp`, then the unit tests in `SerializeTests.cpp`, and ends with `All serialize tests passed (global heap).` On a failure it prints `TEST FAILED` with the file and line and exits with status 1. The tests run in every build type, Release included.

| CMake option | Default | Effect |
|---|---|---|
| `MSG_SERIALIZE_ALLOCATOR` | `OFF` | `ON`: `dmq::x*` types use the fixed-block allocator (see [Fixed-Block Allocator](#fixed-block-allocator)) |
| `MSG_SERIALIZE_ALLOCATOR_SAFEGUARDS` | `OFF` | With the allocator: guard words and checks on every block, to catch overruns and double frees |

The GitHub workflows (Windows, Ubuntu GCC, Clang) build and run both the global heap and the fixed-block allocator variants.

Only `msg_serialize.h` is needed to use the library; `allocator/` is optional.

## Fixed-Block Allocator

The optional `allocator/` directory is a fixed-block memory allocator, for embedded systems where the global heap is unavailable, too slow or fragments. It comes from [DelegateMQ](https://github.com/DelegateMQ/DelegateMQ) and keeps its names (namespace `dmq`, `DMQ_*` macros), but needs nothing else from it. See [allocator/README.md](allocator/README.md).

Declare message fields with the `dmq::x*` types from `allocator/xtypes.h`. The `MSG_SERIALIZE_ALLOCATOR` build option picks their memory, with no code changes:

| Type | `MSG_SERIALIZE_ALLOCATOR=OFF` | `MSG_SERIALIZE_ALLOCATOR=ON` |
|---|---|---|
| `dmq::xstring` | `std::string` | `std::basic_string` on fixed-block pools |
| `dmq::xlist<T>`, `dmq::xmap<K, V>`, `dmq::xset<T>` | `std::list`, `std::map`, `std::set` | the same containers on fixed-block pools |
| `dmq::xstringstream`, `dmq::xostringstream` | `std::stringstream`, `std::ostringstream` | stream buffers on fixed-block pools |
| `XALLOCATOR` (in a class) | nothing | class `operator new`/`delete` from the pools |

```cpp
#include "msg_serialize.h"
#include "xtypes.h"

class Status : public serialize::I
{
    XALLOCATOR                          // 'new Status' uses the pools when enabled
public:
    dmq::xstring name;
    dmq::xlist<int32_t> codes;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, name);
        return ms.write(os, codes);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, name);
        return ms.read(is, codes);
    }
};
```

The allocator doesn't change the wire format: a `dmq::xlist<dmq::xstring>` written on one node reads into a `std::list<std::string>` on another.

### Choosing Containers on Embedded Targets

What a message's fields allocate matters more on an MCU than how they're serialized. With the allocator, memory comes from fixed-block pools, one pool per power-of-two block size, and blocks are kept for reuse rather than returned to the heap.

| Field type | Allocation | Embedded use |
|---|---|---|
| `std::array<T, N>`, `char[N]` | None | **Best.** Fixed size, no heap and no pool. |
| `dmq::xlist`, `dmq::xmap`, `dmq::xset` | One same-size node per element, from one fixed-block pool | **Good.** Freed nodes are reused exactly. |
| `dmq::xstring` | One block sized to the string, from a fixed-block pool | **Good**, bounded by `MSG_SERIALIZE_MAX_STRING_SIZE`. |
| `std::vector` | One contiguous block, reallocated larger as it grows | **Host-side only.** Each growth step needs a larger block size, so a vector touches several pools and each keeps its blocks. In static-pool mode the largest block is 4096 bytes. |

On the MCU, declare message fields with `std::array`, `dmq::xlist`/`xmap`/`xset` and `dmq::xstring`. On desktop or gateway nodes, `std::vector` is fine, and one end can use `std::vector<T>` while the other uses `std::array<T, N>` for the same field.

## Error Handling 
Errors and parser progress are monitored by registering a callback function pointer.  

```cpp
#include "msg_serialize.h"
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

// Message serializer instance
static serialize ms;

void ErrorHandlerCallback(serialize::ParsingError error, int line, const char* file)
{
    // Output parsing error message
    cout << "PARSE ERROR: " << file << " " << line << " " << static_cast<int>(error) << endl;
}

void ParseHandlerCallback(const type_info& typeId, size_t size)
{
    // Output parser progress
    cout << typeId.name() << " " << size << endl;
}

int main(void)
{
    ms.setErrorHandler(&ErrorHandlerCallback);
    ms.setParseHandler(&ParseHandlerCallback);

    //...

    return 0;
}
```
Check for errors with `getLastError()` to get the last parse error code.

A failed read or write also sets the stream's `failbit`, so checking `is.good()` / `os.good()` after a call is enough to detect any error.

| Error | Cause |
|---|---|
| `TYPE_MISMATCH` | The type tag in the stream doesn't match the type being read |
| `STREAM_ERROR` | The stream was already bad, or ran out of data mid-value |
| `STRING_TOO_LONG` | A string exceeds `MSG_SERIALIZE_MAX_STRING_SIZE`, or doesn't fit the `char[]` it is read into |
| `CONTAINER_TOO_MANY` | A container exceeds `MSG_SERIALIZE_MAX_CONTAINER_SIZE` |
| `INVALID_INPUT` | Invalid data, e.g. a `std::array<T, N>` receiving a count other than `N` |
| `NON_SEEKABLE_STREAM` | A user-defined object was written to a stream that can't seek |
| `END_OF_FILE` | Read past the end of the stream |
| `NESTING_TOO_DEEP` | `serialize::I` objects nested deeper than `MSG_SERIALIZE_MAX_NESTING` on read |

## Limits

The parser limits are compile-time settings. Define any of them before including `msg_serialize.h`, or on the compiler command line (e.g. `-DMSG_SERIALIZE_MAX_STRING_SIZE=1024`), to override the default. Use the same values on both ends of a link. They apply on write and on read, so a received message can't make the receiver allocate more than this. Exceeding one fails the stream with the error shown.

| Setting | Default | Limits | Error |
|---|---|---|---|
| `MSG_SERIALIZE_MAX_STRING_SIZE` | 256 | String length in bytes, not counting a terminator (none is sent); at most 65535 | `STRING_TOO_LONG` |
| `MSG_SERIALIZE_MAX_CONTAINER_SIZE` | 200 | Elements in a vector, array, list, set or map; at most 65535. Each nested container is checked on its own. A `std::array` larger than this is a compile error | `CONTAINER_TOO_MANY` |
| `MSG_SERIALIZE_MAX_NESTING` | 8 | Depth of `serialize::I` objects nested inside each other, checked on read | `NESTING_TOO_DEEP` |

## Strings

`std::string`, `char*` and `char[]` share one encoding: a `STRING` tag, a 16-bit length and the characters, with no terminator. A string reads back the same whichever way it was sent, and a `std::string` whose data contains or ends in a zero byte keeps every byte. Reading into a `char[]` adds the terminator, and fails with `STRING_TOO_LONG` if the buffer has no room for it:

```cpp
char name[16];
ms.read(is, name, sizeof(name));   // capacity including the terminator
```

> **Wire change:** earlier versions sent a `char*` with its terminator. A current reader still accepts that. An older reader that reads a current `char*` into a `char[]` gets no terminator, so update both ends together.

## Fixed-Size Arrays

`std::array<T, N>` is supported and needs no heap, which suits embedded targets. It is sent exactly like a `std::vector` (the `VECTOR` tag, a count, the elements), so one end can use `std::vector<T>` and the other `std::array<T, N>` for the same field. Reading into a `std::array` fails with `INVALID_INPUT` unless exactly `N` elements arrive. Reading into a `std::vector` reserves the received count first, so it allocates once.


## Protocol Evolution

The `serialize` class parsing handles deserializing objects even if the number of object data fields don’t match the ones known at compile time due to protocol changes. 

For instance, assume protocol version 1 data structure:

```cpp
// DataV1 is a version 1 data structure
class DataV1 : public serialize::I
{
public:
    virtual ostream& write(serialize& ms, ostream& os) override
    {
        ms.write(os, data);
        return os;
    }

    virtual istream& read(serialize& ms, istream& is) override
    {
        ms.read(is, data);
        return is;
    }
    int data = 0;
};
```

Later, the protocol data structure changes to add one more data member:

```cpp
// DataV2 is version 2 data structure with a new data member added
class DataV2 : public serialize::I
{
public:
    virtual ostream& write(serialize& ms, ostream& os) override
    {
        ms.write(os, data);
        ms.write(os, dataNew);
        return os;
    }

    virtual istream& read(serialize& ms, istream& is) override
    {
        ms.read(is, data);
        ms.read(is, dataNew);
        return is;
    }

    int data = 0;
    int dataNew = 0;    // NEW!
};
```

The `serialize` will not fail if a V1 system receives a V2 data structure, or vice versa. If V1 receives a V2 object, the extra data is not parsed and the bytes are skipped. If a V2 system receives a V1 object, the new V2 object data members are not set since the V1 received object does not contain the newly added V2 data.

For message protocol resiliency to message changes to work, certain code change rules must adhered to ensure  interoperability with prior protocol iterations. Once a protocol is released, the rules for making updates to the message objects are:

* Do not delete an existing data field. 
* Do not change an existing data field type.
* Do not change the data field serialize/deserialize order within `read()` and `write()`.
* Introduce new data fields to the end of a message object.

## Endianness

The wire format is little-endian. Multibyte built-in data types are encoded in multiple octets. Each octet is 8-bits. On big-endian hosts the bytes are automatically swapped before writing and after reading so that the on-wire representation is always little-endian. The serialize class performs this byte swapping transparently. No alignment bytes are added to the octet stream regardless of the built-in data type size.

## Seekable Stream Requirement

Serializing user-defined objects (classes that inherit from `serialize::I`) requires a **seekable** output stream. After writing the object body, the library seeks back to backfill the object's size field. Suitable stream types include `std::stringstream` and `std::fstream` (opened in binary mode). Non-seekable streams such as `std::cout` or a raw socket buffer will trigger a `NON_SEEKABLE_STREAM` error and set the stream's `failbit`.

```cpp
// OK — stringstream is seekable
std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
ms.write(ss, myObject);

// OK — fstream is seekable
std::ofstream file("data.bin", std::ios::binary);
ms.write(file, myObject);
```

Always open streams in **binary mode** (`std::ios::binary`). Text-mode streams apply newline translation on some platforms, which corrupts binary data.

## Comparison

The table below compares `MessageSerialize` with other popular C++ serialization libraries. 

| Feature | **MessageSerialize** | **Bitsery** | **Msgpack** | **Cereal** |
| :--- | :--- | :--- | :--- | :--- |
| **Design Goal** | Simplicity & Robustness | Performance & Size | Cross-language | Ease of Use |
| **Complexity** | Very Low (1 header) | Low (Header-only) | Moderate | Moderate |
| **Metadata Overhead** | Low (8-bit tags + size) | **Zero** (by default) | Low (Type tags) | Moderate |
| **Memory Allocation** | Optional (for pointers) | **Minimal/None** | Often required | Required (Tracking) |
| **Protocol Evolution** | **Automatic (Size-based)**| Manual (Extensions) | Natural (Schemaless) | Explicit (Versions) |
| **Embedded Use** | High (Seekable req.) | Excellent | Good (mpack) | Fair |
| **Fixed Allocators** | Via STL Allocators | Yes | Varies by impl | Limited |

### Key Differences

1. **Simplicity vs. Performance**: While `Bitsery` is faster and produces smaller payloads by omitting all metadata, `MessageSerialize` includes 1-byte type tags and object sizes. This metadata allows `MessageSerialize` to automatically handle protocol changes (like adding fields) without any versioning macros or manual extension management.

1. **Protocol Evolution**: `MessageSerialize` uses a unique size-backfilling mechanism. When a reader encounters a user-defined object, it knows the total byte-size. If the reader's class has fewer fields than the data (newer version), it simply skips the remaining bytes. If it has more fields (older version), it stops when the data runs out. This is more robust than `Cereal`'s strict versioning and simpler than `Bitsery`'s extensions.

1. **Embedded Constraints**: `MessageSerialize` is highly suitable for embedded systems but requires a **seekable stream** (like `stringstream` or a file) for user-defined objects because it must backfill the object size after serialization. Libraries like `Bitsery` can stream directly to a non-seekable socket if configured correctly.

1. **Memory Management**: Like `Cereal`, `MessageSerialize` leverages STL containers. For strict no-heap environments, `MessageSerialize` can be used with STL containers and fixed-block allocators (e.g., `std::vector<int, MyFixedAlloc>`), as the library is templated to support any standard-compliant allocator. The optional [Fixed-Block Allocator](#fixed-block-allocator) in `allocator/` provides one, and `std::array` needs no allocation at all.

### Embedded Tradeoffs

Choosing a serializer for an embedded target involves constraints that don't apply on Linux or Windows: limited heap, no or restricted exception support, code-size budgets, and endianness differences between the MCU and any host it talks to.

| Serializer | Embedded suitability | Notes |
|---|---|---|
| **MessageSerialize** | Good | No external dependencies, automatic endianness handling, builds without exceptions (errors are reported through the stream state). Bounded input: string, container and nesting limits are checked on read. Types inherit `serialize::I` and implement `read()`/`write()`. Slightly larger wire format due to type tags and size-prefix versioning. |
| **Bitsery** | Best for tight constraints | Header-only, designed for real-time/embedded use, smallest payloads. Plain structs work with a `serialize()` function, no base class. No type tags on the wire, so a mismatch isn't detected; endianness is configured explicitly rather than handled automatically. |
| **MessagePack** (msgpack-c) | Linux/Windows | Allocates dynamically (`msgpack::sbuffer`, `std::vector`) and is an external dependency: manageable on a host, a concern on an MCU with a small RTOS heap. Its strength is cross-language messaging. |
| **Cereal** | Poor | Heavy template machinery and reliance on exceptions and `<memory>` increase code size considerably. Rarely justified on a Cortex-M class target. |
| **RapidJSON** | Poor | JSON payloads (field names, brackets, quotes) are typically 3–10× larger than an equivalent binary format. Use it when a host needs human-readable output or REST interoperability, not for high-rate data on an MCU. |

**Guidance:**

- **Default embedded choice**: MessageSerialize. It works out of the box with no external dependencies, and handles endianness automatically, which matters when an MCU talks to an x86 host.
- **Smallest and fastest**: Bitsery, when payload size or throughput matters most and both ends are trusted C++ code built together.
- **Other languages on the far end**: MessagePack, on host-side nodes.
- **Host-side only**: Cereal and RapidJSON.

## Transport Protocol

The `serialize` binary output stream is typically used for a protocol payload. See the links below for transport protocol examples. 

[DelegateMQ](https://github.com/DelegateMQ) - a C++ messaging library for messaging between threads and across processors.

[Simple Socket Protocol](https://github.com/endurodave/SimpleSocketProtocol) - a compact, C language simple socket-like transport protocol.

## Implementation

All user defined data types inherit from `serialize::I`:

```cpp
class serialize
{
public:
    /// @brief Abstract interface that all serialized user defined classes inherit.
    class I
    {
    public:
        /// Inheriting class implements the write function. Write each
        /// class member to the ostream. Write in the same order as read().
        /// Each level within the hierarchy must implement. Ensure base 
        /// write() implementation is called if necessary. 
        /// @param[in] ms - the message serialize instance
        /// @param[in] is - the input stream
        /// @return The input stream
        virtual std::ostream& write(serialize& ms, std::ostream& os) = 0;

        /// Inheriting class implements the read function. Read each
        /// class member to the ostream. Read in the same order as write().
        /// Each level within the hierarchy must implement. Ensure base 
        /// read() implementation is called if necessary. 
        /// @param[in] ms - the message serialize instance
        /// @param[in] is - the input stream
        /// @return The input stream
        virtual std::istream& read(serialize& ms, std::istream& is) = 0;
    };
```

Numerous `serialize` overloads handle `write`/`read` of different data types:

```cpp
// Write APIs
template<typename T>
std::ostream& write(std::ostream& os, T& t_, bool prependType = true) { ... }

std::ostream& write(std::ostream& os, const std::string& s) { ... }

std::ostream& write(std::ostream& os, const std::wstring& s) { ... }

template <class T, class Alloc>
std::ostream& write(std::ostream& os, std::vector<T, Alloc>& container) { ... }

template <class K, class V, class P, class Alloc>
std::ostream& write(std::ostream& os, std::map<K, V, P, Alloc>& container) { ... }
// ...

// Read APIs
template<typename T>
std::istream& read(std::istream& is, T& t_, bool readPrependedType = true) { ... }

std::istream& read(std::istream& is, std::string& s) { ... }

std::istream& read(std::istream& is, std::wstring& s) { ... }

std::istream& read(std::istream& is, char* str, size_t maxLen) { ... }

template <class T, class Alloc>
std::istream& read(std::istream& is, std::vector<T, Alloc>& container) { ... }

template <class K, class V, class P, class Alloc>
std::istream& read(std::istream& is, std::map<K, V, P, Alloc>& container) { ... }
// ...
```

### Encoding

Each data types is prepended with an 8-bit type:

```cpp
enum class Type 
{
    UNKNOWN = 0,
    LITERAL = 1,
    STRING = 8,
    WSTRING = 9,
    VECTOR = 20,
    MAP = 21,
    LIST = 22,
    SET = 23,
    ENDIAN = 30,
    USER_DEFINED = 31,
};
```

### Primitive Data Type Encoding

The `struct` definitions below are used to conveying the serialized data memory layout for primitive data types. The structures themselves do not exist within the source code. 

For instance, a `short` is encoded as an 8-bit `Type` followed by 16-bit `short` value.

```cpp
struct short_data {
   Type type = LITERAL;     // 8-bits
   short s;                 // 16-bits
};
```

Similarly, a `long` encoding is shown below.

```cpp
struct long_data {
   Type type = LITERAL;     // 8-bits
   long l;                  // 32-bits
};
```

All other numeric primitive data types are encoded similarly (e.g. float, double, unsigned long, ...). Primitive numeric data types are automatically byte swapped to handle endianness.

`char[]` / `char*` string encoding (same as `std::string`, no terminator):

```cpp
struct char_arr_data {
   Type type = STRING;      // 8-bits
   unsigned short size;     // 16-bits, size is strlen()
   char str[size];          // size x 8-bits
};
```

Do not use C arrays of numeric values (e.g. `float[]`). Instead, use `std::array<float, N>` or an STL container (e.g. `std::list<float>`).

### STL Container Encoding

`xstring` encoding:
```cpp
struct string_data {
   Type type = STRING;      // 8-bits
   unsigned short size;     // 16-bits, size is std::string::size()  
   char str[size];          // size x 8-bits
};
```

`xwstring` encoding:
```cpp
struct wstring_data {
   Type type = WSTRING;     // 8-bits
   unsigned short size;     // 16-bits, size is std::wstring::size()  
   wchar_t str[size];       // size x 16-bits
};
```

`std::list<T>` encoding:
```cpp
struct list_data {
   Type type = LIST;        // 8-bits
   unsigned short size;     // 16-bits, size is std::list::size()
   T data[size];            // size x sizeof(T) x 8-bits 
};
```

`std::list<T&>` encoding:
```cpp
struct list_ref_data {
   Type type = LIST;        // 8-bits
   unsigned short size;     // 16-bits, size is std::list::size()
   T data[size];            // size x sizeof(T) bits x 8-bits
};
```

`std::list<T*>` encoding:
```cpp
struct list_ptr_data {
   Type type = LIST;        // 8-bits
   unsigned short size;     // 16-bits, size is std::list::size()
   T data[size];            // size x sizeof(T*) x 8-bits
};
```

The `std::map`, `std::set`, `std::vector` all follow a similar binary encoding mechanism to `std::list`. `std::array<T, N>` uses the `VECTOR` encoding. A container element that is itself a container or a string uses that type's full encoding, so containers nest to any depth.

### User Defined Encoding

Any user defined class that inherits from `serialize::I`. The size of a user defined type depends on the number of octets required to serialize the object (not `sizeof(T)`). The size is the total octet count of all data fields contained within the user defined object instance. 

```cpp
struct user_defined_data {
   Type type = USER_DEFINED;    // 8-bits
   unsigned short size;         // 16-bits, total size of message object octets
   char data[size];             // size is the number of message object octets serialized
};
```

Each user defined object is the aggregate octet count from any combination of:

* Primitive data types
* Container data types
* User defined data types

Any complex message object topology using inheritance and/or composition is supported.