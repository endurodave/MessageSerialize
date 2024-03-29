# Message Serialize
A simple serialize class to binary serialize and deserialize C++ objects.

## Introduction

The `serialize` class can be used to serialize and deserialize structured data into a binary format suitable for communication protocols, data storage, and inter-process communication (IPC). A single header file `serialize.h` implements the serialize functionality.

1. **Purpose:** The serialize class is designed to provide a framework for serializing and deserializing C++ objects into binary format, enabling data interchange between different systems or storage mediums.

1. **Interface:** The class provides an abstract interface `I` that all serialized user-defined classes must implement. This interface consists of write and read functions, responsible for writing class members to an output stream and reading them from an input stream, respectively.

1. **Supported Data Types:** The serialize class supports various C++ data types for serialization, including literals, strings (`std::string` and `std::wstring`), vectors, maps, lists, sets, and character arrays (`char[]`).

1. **Endianness Handling:** It provides functions to determine the endianness of the system and read/write endianness information from/to streams.

1. **Error Handling:** The class handles various parsing errors, such as type mismatch, stream errors, string length exceeding limits, and invalid input, ensuring robustness during serialization and deserialization.

1. **Protocol Evolution:** It supports parsing objects with additional or missing data fields, allowing flexibility in the data interchange protocol. Extra data received after parsing an object is discarded, and missing data fields are ignored, facilitating protocol evolution without breaking compatibility.

1. **Serialization and Deserialization Functions:** The class provides specialized read and write functions for different data types, including user-defined types, strings, vectors (including `std::vector<bool>`), and character arrays.

1. **Template Function for Built-in Types:** It includes a template function for reading/writing built-in types (`T`) from/to streams, handling pointers and non-pointer types differently.

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

	// Define less-than operator
	bool operator<(const Date& other) const 
	{
		if (year != other.year)
			return year < other.year;
		if (month != other.month)
			return month < other.month;
		return day < other.day;
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

At runtime, serialize and deserialize an `AlarmLog` object to a `std::stringstream`. Check stream `good()` to ensure parsing success before using object data. The data bytes within the stream can be sent over a communication protocol, for instance.

```cpp
AlarmLog writeLog;
writeLog.logType = Log::LogType::ALARM;
writeLog.alarmValue = 123;

// Write log to stringstream
stringstream ss(ios::in | ios::out | ios::binary);
ms.write(ss, writeLog);

// Read log from stringstream
AlarmLog readLog;
ms.read(ss, readLog);
if (ss.good())
{
    // Parse succeeded; use readLog values
    cout << "AlarmLog Parse Success! " << readLog.alarmValue << endl;
}
else
{
    cout << "ERROR: AlarmLog" << endl;
}
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

## Error Handling 
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

## Interoperability

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
	int dataNew = 0;	// NEW!
};
```

The `serialize` will not fail if a V1 system receives a V2 data structure, or vice versa. If V1 receives a V2 object, the extra data is not parsed and the bytes are skipped. If a V2 system receives a V1 object, the new V2 object data members are not set since the V1 received object does not contain the newly added V2 data.

For message protocol resiliency to message changes to work, certain code change rules must adhered to ensure  interoperability with prior protocol iterations. Once a protocol is released, the rules for making updates to the message objects are:

* Do not delete an existing data field. 
* Do not change an existing data field type.
* Do not change the data field serialize/deserialize order within `read()` and `write()`.
* Introduce new data fields to the end of a message object.

## Endianness

The `serialize` class automatically handles endianness byte swapping to support communication protocols between different CPU architectures.



