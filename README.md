# Message Serialize
A simple serialize class to binary serialize and deserialize C++ objects.

# Table of Contents

- [Message Serialize](#message-serialize)
- [Table of Contents](#table-of-contents)
- [Introduction](#introduction)
  - [Purpose](#purpose)
- [Example Usage](#example-usage)
- [Error Handling](#error-handling)
- [Protocol Evolution](#protocol-evolution)
- [Endianness](#endianness)
- [Transport Protocol](#transport-protocol)
  - [Implementation](#implementation)
  - [Encoding](#encoding)
  - [Primitive Data Type Encoding](#primitive-data-type-encoding)
  - [STL Container Encoding](#stl-container-encoding)
  - [User Defined Encoding](#user-defined-encoding)


# Introduction

The `serialize` class can be used to serialize and deserialize structured data into a binary format suitable for communication protocols, data storage, and inter-process communication (IPC). The STL Input/Output Stream Library is used to hold the encoded octet streams in readiness for transmission or parsing. A single header file `serialize.h` implements the serialize functionality. Any C++14 or higher compiler is supported.

1. **Purpose:** The serialize class is designed to provide a framework for serializing and deserializing C++ objects into binary format, enabling data interchange between different systems or storage mediums.

1. **Interface:** The class provides an abstract interface `serialize::I` that all serialized user-defined classes must implement. This interface consists of `write` and `read` functions, responsible for writing class members to an output stream and reading them from an input stream, respectively.

1. **Supported Data Types:** The serialize class supports various C++ data types for serialization, including literals, strings (`std::string` and `std::wstring`), vectors, maps, lists, sets, and character arrays (`char[]`).

1. **Endianness Handling:** It provides functions to determine the endianness of the system and read/write endianness information from/to streams.

1. **Error Handling:** The class handles various parsing errors, such as type mismatch, stream errors, string length exceeding limits, and invalid input, ensuring robustness during serialization and deserialization.

1. **Protocol Evolution:** It supports parsing objects with additional or missing data fields, allowing flexibility in the data interchange protocol. Extra data received after parsing an object is discarded, and missing data fields are ignored, facilitating protocol evolution without breaking compatibility.

1. **Serialization and Deserialization Functions:** The class provides specialized read and write functions for different data types, including user-defined types, strings, vectors (including `std::vector<bool>`), and character arrays.

## Purpose 

Numerous libraries are available for encoding transport payloads. The `serialize` class has a few advantages: 

1. **Simplicity:** A single header file with less than 1000 source lines of code.

1. **Ease of Use:** Inherit from `serialize::I` and override `write()` and `read()`; that's it. 

1. **Flexibility:** Serialize built-in or user defined data types, and STL data containers (e.g. `std::list`, `std::string`).

1. **Efficiency:** Binary encoding efficiently serializes/deserializes messages using STL streams.

1. **Robustness:** Automatic endianness handling and parser resilience to message changes over time. 

# Example Usage

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
        ms.read(is, cstr);
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

# Error Handling 
Errors and parser progress are monitored by registering a callback function pointer.  

```cpp
#include "serialize.h"
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

# Protocol Evolution

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

# Endianness

The C++ built-in data types are sent big-endian. Multibyte built-in data types are encoded in multiple octets. Each octet is 8-bits. All built-in multibyte data types are byte swapped for endianness by the sender or receiver as necessary based upon the detected CPU endianness. The serialize class automatically performs the byte swapping when marshalling the octet stream. No alignment bytes are added to the octet stream regardless of the built-in data type size. 

# Transport Protocol

The `serialize` binary output stream is typically used for a protocol payload. See the link below for an a compact, C language simple socket-like transport protocol with easy porting to any system.

[Simple Socket Protocol](https://github.com/endurodave/SimpleSocketProtocol)

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
std::ostream& write(std::ostream& os, std::string& s) { ... }

std::ostream& write (std::ostream& os, const std::wstring& s) { ... }

template <class T>
std::ostream& write(std::ostream& os, std::vector<T>& container) { ... }

template <class K, class V, class P>
std::ostream& write(std::ostream& os, std::map<K, V, P>& container) { ... }
// ...

// Read APIs
std::istream& read (std::istream& is, std::string& s) { ... }

std::istream& read (std::istream& is, std::wstring& s) { ... }

template <class T>
std::istream& read(std::istream& is, std::vector<T>& container) { ... }

template <class K, class V, class P>
std::istream& read(std::istream& is, std::map<K, V, P>& container) { ... }
// ...
```

## Encoding

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

## Primitive Data Type Encoding

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

`char[]` string encoding (null terminated):

```cpp
struct char_arr_data {
   Type type = STRING;      // 8-bits
   unsigned short size;     // 16-bits, size is strlen() + 1 
   char str[length];        // size x 8-bits
};
```

Do not use arrays of numeric values (e.g. `float[]`). Instead, use STL container classes (e.g. `std::list<float>`).

## STL Container Encoding

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

The `std::map`, `std::set`, `std::vector` all follow a similar binary encoding mechanism to `std::list`. 

## User Defined Encoding

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