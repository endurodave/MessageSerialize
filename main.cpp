/// @file main.cpp
/// @see https://github.com/endurodave/MessageSerialize
/// David Lafreniere, 2024.

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

// Some systems specify enum's as 16-bit or 32-bit. Specifying explicitly improves 
// cross platform compatibility (e.g. uint16_t) and reduces message size.
enum class Color : uint16_t { RED, GREEN, BLUE };

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

void CreateData(AllData& data)
{
	strcpy(data.cstr, "Hello World!");
	data.str = "Hello World!";
	data.wstr = L"Hello World Wide!";

	data.dataVectorBool.push_back(false);
	data.dataVectorBool.push_back(true);

	data.dataVectorFloat.push_back(1.23f);
	data.dataVectorFloat.push_back(3.21f);

	data.dataVectorPtr.push_back(new Date(1, 1, 2001));
	data.dataVectorPtr.push_back(new Date(2, 2, 2002));

	data.dataVectorValue.push_back(Date(1, 1, 2001));
	data.dataVectorValue.push_back(Date(2, 2, 2002));

	data.dataVectorInt.push_back(1);
	data.dataVectorInt.push_back(2);

	data.dataListPtr.push_back(new Date(1, 1, 2001));
	data.dataListPtr.push_back(new Date(2, 2, 2002));

	data.dataListValue.push_back(Date(1, 1, 2001));
	data.dataListValue.push_back(Date(2, 2, 2002));

	data.dataListInt.push_back(1);
	data.dataListInt.push_back(2);

	data.dataMapPtr[0] = new Date(1, 1, 2001);
	data.dataMapPtr[1] = new Date(2, 2, 2002);

	data.dataMapValue[0] = Date(1, 1, 2001);
	data.dataMapValue[1] = Date(2, 2, 2002);

	data.dataMapInt[0] = 1;
	data.dataMapInt[1] = 2;

	data.dataSetPtr.insert(new Date(1, 1, 2001));
	data.dataSetPtr.insert(new Date(2, 2, 2002));

	data.dataSetValue.insert(Date(1, 1, 2001));
	data.dataSetValue.insert(Date(2, 2, 2002));

	data.dataSetInt.insert(1);
	data.dataSetInt.insert(2);
}

void SerializeToFile(const AllData& data)
{
	// Write binary stream to disk
	ofstream outputFile("serialize.bin", ios::binary);
	if (!outputFile.is_open()) 
	{
		cerr << "ERROR: Failed to open the file for writing." << endl;
		return;
	}

	// Write Item serialize data to file
	ms.write(outputFile, data);
	if (!outputFile.good())
		cout << "ERROR: SerializeFromFile" << endl;

	outputFile.close();
}

void DeserializeFromFile(AllData& allData)
{
	// Open the file for reading
	ifstream inputFile("serialize.bin", ios::binary);
	if (!inputFile.is_open()) 
	{
		cerr << "ERROR: Failed to open the file for reading." << endl;
		return;
	}

	// Read Item instance from file 
	ms.read(inputFile, allData);
	if (!inputFile.good())
		cout << "ERROR: DeserializeFromFile" << endl;

	inputFile.close();
}

void SerializeToStringstream(stringstream& ss, const AllData& data)
{
	// Serialize Item instance to stringstream
	ms.write(ss, data);
	if (!ss.good())
		cout << "ERROR: SerializeToStringstream" << endl;
}

void DeserializeFromStringstream(stringstream& ss, AllData& allData)
{
	// Deserialize Item instance from stringstream
	ms.read(ss,allData);
	if (!ss.good())
		cout << "ERROR: DeserializeFromStringstream" << endl;
}

int main(void)
{
	ms.setErrorHandler(&ErrorHandlerCallback);
	ms.setParseHandler(&ParseHandlerCallback);

	AllData outData;
	CreateData(outData);

	// Log example
	{
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
	}

	// File example
	{
		SerializeToFile(outData);
		AllData allData;
		DeserializeFromFile(allData);
	}

	// Stringstream example
	{
		stringstream ss(ios::in | ios::out | ios::binary);
		SerializeToStringstream(ss, outData);
		AllData allData;
		DeserializeFromStringstream(ss, allData);
	}

	// Test serialize DataV1 and deserialize with DataV2 example
	{
		DataV1 dataV1;
		dataV1.data = 111;

		stringstream ss(ios::in | ios::out | ios::binary);
		ms.write(ss, dataV1);
		if (!ss.good())
			cout << "ERROR: dataV1" << endl;

		DataV2 dataV2;
		ms.read(ss, dataV2);
		if (!ss.good())
			cout << "ERROR: dataV2" << endl;
	}

	// Test serialize DataV2 and deserialize with DataV1 example
	{
		DataV2 dataV2;
		dataV2.data = 111;
		dataV2.dataNew = 222;

		stringstream ss(ios::in | ios::out | ios::binary);
		ms.write(ss, dataV2);
		if (!ss.good())
			cout << "ERROR: dataV2" << endl;

		DataV1 dataV1;
		ms.read(ss, dataV1);
		if (!ss.good())
			cout << "ERROR: dataV1" << endl;
	}

	// TODO: Send serialized data to another CPU as part of a 
	// send/receive binary protocol.

    return 0;
}
