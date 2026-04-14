/// @file main.cpp
/// @see https://github.com/endurodave/MessageSerialize
/// David Lafreniere, 2024.

#include "msg_serialize.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdint>

#define ASSERT_TRUE(expr) assert(expr)

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

        // static_assert; pointers not allowed
        //ms.write(os, pdate);
        //ms.write(os, pint);
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

        // static_assert; pointers not allowed
        //ms.read(is, pdate);
        //ms.read(is, pint);
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

    // Pointer to data not allowed; static_assert triggered
    Date* pdate = nullptr;
    int* pint = nullptr;
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
    int dataNew = 0;    // NEW!
};

// ---------------------------------------------------------------------------
// Test message types
// ---------------------------------------------------------------------------

struct SimpleMsg : public serialize::I
{
    virtual ~SimpleMsg() = default;
    int32_t id    = 0;
    float   value = 0.0f;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, id);
        return ms.write(os, value);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, id);
        return ms.read(is, value);
    }
};

struct NestedMsg : public serialize::I
{
    SimpleMsg inner;
    int32_t   extra = 0;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, inner);
        return ms.write(os, extra);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, inner);
        return ms.read(is, extra);
    }
};

struct MsgV1 : public serialize::I
{
    int32_t field1 = 0;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        return ms.write(os, field1);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        return ms.read(is, field1);
    }
};

struct MsgV2 : public serialize::I
{
    int32_t field1 = 0;
    int32_t field2 = 0;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, field1);
        return ms.write(os, field2);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, field1);
        return ms.read(is, field2);
    }
};

struct AllPrimitivesMsg : public serialize::I
{
    bool     b   = false;
    int8_t   i8  = 0;
    uint8_t  u8  = 0;
    int16_t  i16 = 0;
    uint16_t u16 = 0;
    int32_t  i32 = 0;
    uint32_t u32 = 0;
    int64_t  i64 = 0;
    uint64_t u64 = 0;
    float    f   = 0.0f;
    double   d   = 0.0;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, b);
        ms.write(os, i8);
        ms.write(os, u8);
        ms.write(os, i16);
        ms.write(os, u16);
        ms.write(os, i32);
        ms.write(os, u32);
        ms.write(os, i64);
        ms.write(os, u64);
        ms.write(os, f);
        return ms.write(os, d);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, b);
        ms.read(is, i8);
        ms.read(is, u8);
        ms.read(is, i16);
        ms.read(is, u16);
        ms.read(is, i32);
        ms.read(is, u32);
        ms.read(is, i64);
        ms.read(is, u64);
        ms.read(is, f);
        return ms.read(is, d);
    }
};

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

template<typename T>
static bool RoundTrip(T& src, T& dst)
{
    std::ostringstream oss;
    serialize ser;
    ser.write(oss, src);
    if (!oss.good())
        return false;
    std::istringstream iss(oss.str());
    ser.read(iss, dst);
    return !iss.fail();
}

template<typename T>
static bool RoundTripPrimitive(T val, T& out)
{
    std::ostringstream oss;
    serialize ser;
    ser.write(oss, val);
    if (!oss.good())
        return false;
    std::istringstream iss(oss.str());
    ser.read(iss, out);
    return !iss.fail();
}

// ---------------------------------------------------------------------------
// Test functions
// ---------------------------------------------------------------------------

static void PrimitiveTests()
{
    {
        bool out = false;
        ASSERT_TRUE(RoundTripPrimitive(true, out));
        ASSERT_TRUE(out == true);
        ASSERT_TRUE(RoundTripPrimitive(false, out));
        ASSERT_TRUE(out == false);
    }
    {
        int8_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int8_t)-128, out));
        ASSERT_TRUE(out == -128);

        uint8_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint8_t)255, uout));
        ASSERT_TRUE(uout == 255);
    }
    {
        int16_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int16_t)-32768, out));
        ASSERT_TRUE(out == -32768);

        uint16_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint16_t)65535, uout));
        ASSERT_TRUE(uout == 65535);
    }
    {
        int32_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int32_t)-1, out));
        ASSERT_TRUE(out == -1);
        ASSERT_TRUE(RoundTripPrimitive((int32_t)0x12345678, out));
        ASSERT_TRUE(out == 0x12345678);

        uint32_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint32_t)0xDEADBEEFu, uout));
        ASSERT_TRUE(uout == 0xDEADBEEFu);
    }
    {
        int64_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int64_t)0x0102030405060708LL, out));
        ASSERT_TRUE(out == 0x0102030405060708LL);

        uint64_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint64_t)0xFFFFFFFFFFFFFFFFULL, uout));
        ASSERT_TRUE(uout == 0xFFFFFFFFFFFFFFFFULL);
    }
    {
        float out = 0.0f;
        ASSERT_TRUE(RoundTripPrimitive(3.14159f, out));
        ASSERT_TRUE(out == 3.14159f);
        ASSERT_TRUE(RoundTripPrimitive(-0.0f, out));
        ASSERT_TRUE(out == -0.0f);
    }
    {
        double out = 0.0;
        ASSERT_TRUE(RoundTripPrimitive(2.718281828459045, out));
        ASSERT_TRUE(out == 2.718281828459045);
    }
    {
        int32_t out = 99;
        ASSERT_TRUE(RoundTripPrimitive((int32_t)0, out));
        ASSERT_TRUE(out == 0);
    }
}

static void StringTests()
{
    serialize ser;
    {
        std::string src = "Hello, World!";
        std::string dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::string src;
        std::string dst = "non-empty";
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(dst.empty() || dst == "non-empty");
    }
    {
        std::string src(256, 'A');
        std::string dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::string src(257, 'B');
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, src);
        ASSERT_TRUE(!oss.good());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
    }
    {
        std::wstring src = L"Wide string test";
        std::wstring dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
}

static void CharArrayTests()
{
    serialize ser;
    {
        const char* src = "hello";
        char dst[32] = {};
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(std::strcmp(dst, src) == 0);
    }
    {
        const char* src = "abc";
        char dst[4] = {};
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(std::strcmp(dst, src) == 0);
    }
    {
        const char* src = "toolongstring";
        char dst[4] = {};
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.clearLastError();
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
    }
}

static void UserDefinedObjectTests()
{
    {
        SimpleMsg src;
        src.id = 42; src.value = 1.23f;
        SimpleMsg dst;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.id == src.id);
        ASSERT_TRUE(dst.value == src.value);
    }
    {
        SimpleMsg src;
        SimpleMsg dst;
        dst.id = 99; dst.value = 9.9f;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.id == 0);
        ASSERT_TRUE(dst.value == 0.0f);
    }
    {
        NestedMsg src;
        src.inner.id = 7; src.inner.value = 3.14f; src.extra = -100;
        NestedMsg dst;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.inner.id == src.inner.id);
        ASSERT_TRUE(dst.inner.value == src.inner.value);
        ASSERT_TRUE(dst.extra == src.extra);
    }
    {
        AllPrimitivesMsg src;
        src.b = true; src.i8 = -1; src.u8 = 255;
        src.i16 = -32768; src.u16 = 65535;
        src.i32 = 0x12345678; src.u32 = 0xDEADBEEFu;
        src.i64 = 0x0102030405060708LL; src.u64 = 0xFFFFFFFFFFFFFFFFULL;
        src.f = 3.14159f; src.d = 2.71828182845;
        AllPrimitivesMsg dst;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.b == src.b); ASSERT_TRUE(dst.i8 == src.i8);
        ASSERT_TRUE(dst.u8 == src.u8); ASSERT_TRUE(dst.i16 == src.i16);
        ASSERT_TRUE(dst.u16 == src.u16); ASSERT_TRUE(dst.i32 == src.i32);
        ASSERT_TRUE(dst.u32 == src.u32); ASSERT_TRUE(dst.i64 == src.i64);
        ASSERT_TRUE(dst.u64 == src.u64); ASSERT_TRUE(dst.f == src.f);
        ASSERT_TRUE(dst.d == src.d);
    }
}

static void VersioningTests()
{
    serialize ser;
    {
        MsgV2 v2src; v2src.field1 = 111; v2src.field2 = 222;
        std::ostringstream oss;
        ser.write(oss, v2src);
        ASSERT_TRUE(oss.good());
        MsgV1 v1dst;
        std::istringstream iss(oss.str());
        ser.read(iss, v1dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(v1dst.field1 == 111);
    }
    {
        MsgV1 v1src; v1src.field1 = 333;
        std::ostringstream oss;
        ser.write(oss, v1src);
        ASSERT_TRUE(oss.good());
        MsgV2 v2dst; v2dst.field2 = 999;
        std::istringstream iss(oss.str());
        ser.read(iss, v2dst);
        ASSERT_TRUE(v2dst.field1 == 333);
        ASSERT_TRUE(v2dst.field2 == 999);
    }
}

static void ContainerValueTests()
{
    serialize ser;
    {
        std::vector<int> src = {1, 2, 3, 100, -50};
        std::vector<int> dst;
        std::ostringstream oss; ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst == src);
    }
    {
        std::vector<int> src;
        std::vector<int> dst = {9, 8, 7};
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.empty());
    }
    {
        std::vector<bool> src = {true, false, true, true, false};
        std::vector<bool> dst;
        std::ostringstream oss; ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst == src);
    }
    {
        std::list<int> src = {10, 20, 30};
        std::list<int> dst;
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst == src);
    }
    {
        std::set<int> src = {5, 3, 1, 4, 2};
        std::set<int> dst;
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst == src);
    }
    {
        std::map<int, int> src = {{1, 10}, {2, 20}, {3, 30}};
        std::map<int, int> dst;
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst == src);
    }
    {
        std::vector<SimpleMsg> src(3);
        src[0].id = 1; src[0].value = 1.1f;
        src[1].id = 2; src[1].value = 2.2f;
        src[2].id = 3; src[2].value = 3.3f;
        std::vector<SimpleMsg> dst;
        std::ostringstream oss; ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst.size() == 3);
        ASSERT_TRUE(dst[0].id == 1 && dst[0].value == 1.1f);
        ASSERT_TRUE(dst[1].id == 2 && dst[1].value == 2.2f);
        ASSERT_TRUE(dst[2].id == 3 && dst[2].value == 3.3f);
    }
    {
        std::map<int, SimpleMsg> src;
        src[1].id = 10; src[1].value = 0.1f;
        src[2].id = 20; src[2].value = 0.2f;
        std::map<int, SimpleMsg> dst;
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[1].id == 10 && dst[1].value == 0.1f);
        ASSERT_TRUE(dst[2].id == 20 && dst[2].value == 0.2f);
    }
}

static void ContainerPointerTests()
{
    serialize ser;
    {
        SimpleMsg a, b;
        a.id = 1; a.value = 1.0f; b.id = 2; b.value = 2.0f;
        std::vector<SimpleMsg*> src = {&a, &b};
        std::ostringstream oss; ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::vector<SimpleMsg*> dst;
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail()); ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[0] != nullptr && dst[0]->id == 1 && dst[0]->value == 1.0f);
        ASSERT_TRUE(dst[1] != nullptr && dst[1]->id == 2 && dst[1]->value == 2.0f);
        for (auto* p : dst) delete p;
    }
    {
        SimpleMsg a; a.id = 5; a.value = 5.0f;
        std::vector<SimpleMsg*> src = {&a, nullptr};
        std::ostringstream oss; ser.write(oss, src);
        std::vector<SimpleMsg*> dst;
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[0] != nullptr && dst[0]->id == 5);
        ASSERT_TRUE(dst[1] == nullptr);
        delete dst[0];
    }
    {
        SimpleMsg a; a.id = 7; a.value = 7.7f;
        std::list<SimpleMsg*> src = {&a};
        std::ostringstream oss; ser.write(oss, src);
        std::list<SimpleMsg*> dst;
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 1);
        ASSERT_TRUE(dst.front() != nullptr && dst.front()->id == 7);
        delete dst.front();
    }
    {
        SimpleMsg a; a.id = 11; a.value = 11.0f;
        std::map<int, SimpleMsg*> src; src[1] = &a; src[2] = nullptr;
        std::ostringstream oss; ser.write(oss, src);
        std::map<int, SimpleMsg*> dst;
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[1] != nullptr && dst[1]->id == 11);
        ASSERT_TRUE(dst[2] == nullptr);
        delete dst[1];
    }
    {
        SimpleMsg a; a.id = 99; a.value = 9.9f;
        std::set<SimpleMsg*> src = {&a};
        std::ostringstream oss; ser.write(oss, src);
        std::set<SimpleMsg*> dst;
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 1);
        ASSERT_TRUE((*dst.begin()) != nullptr && (*dst.begin())->id == 99);
        delete *dst.begin();
    }
}

static void EndiannessTests()
{
    serialize ser;
    {
        std::ostringstream oss;
        ser.writeEndian(oss);
        ASSERT_TRUE(oss.good());
        bool le = !serialize::LE();
        std::istringstream iss(oss.str());
        ser.readEndian(iss, le);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(le == serialize::LE());
    }
    {
        const int32_t val = 0x01020304;
        int32_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive(val, out));
        ASSERT_TRUE(out == val);
    }
    {
        const uint64_t val = 0x0102030405060708ULL;
        uint64_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive(val, out));
        ASSERT_TRUE(out == val);
    }
    {
        ASSERT_TRUE(serialize::LE() == serialize::LE());
    }
}

static void ErrorTests()
{
    serialize ser;
    {
        std::string src = "hello";
        std::ostringstream oss;
        ser.write(oss, src);
        int32_t dst = 0;
        std::istringstream iss(oss.str());
        ser.clearLastError();
        ser.read(iss, dst);
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::TYPE_MISMATCH);
    }
    {
        std::istringstream iss("");
        int32_t dst = 0;
        ser.clearLastError();
        ser.read(iss, dst);
        ASSERT_TRUE(iss.fail());
    }
    {
        std::vector<int> big(201, 0);
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, big);
        ASSERT_TRUE(!oss.good());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::CONTAINER_TOO_MANY);
    }
    {
        static bool handlerCalled = false;
        static serialize::ParsingError lastErr = serialize::ParsingError::NONE;
        serialize localSer;
        localSer.setErrorHandler([](serialize::ParsingError err, int, const char*) {
            handlerCalled = true;
            lastErr = err;
        });
        std::ostringstream oss;
        std::string tooBig(257, 'X');
        localSer.write(oss, tooBig);
        ASSERT_TRUE(handlerCalled);
        ASSERT_TRUE(lastErr == serialize::ParsingError::STRING_TOO_LONG);
    }
    {
        ser.clearLastError();
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::NONE);
    }
}

static void MultiObjectStreamTests()
{
    serialize ser;
    {
        SimpleMsg a, b;
        a.id = 1; a.value = 1.1f; b.id = 2; b.value = 2.2f;
        std::ostringstream oss;
        ser.write(oss, a); ser.write(oss, b);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        SimpleMsg ra, rb;
        ser.read(iss, ra); ser.read(iss, rb);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(ra.id == 1 && ra.value == 1.1f);
        ASSERT_TRUE(rb.id == 2 && rb.value == 2.2f);
    }
    {
        std::ostringstream oss;
        int32_t n = 42;
        SimpleMsg msg; msg.id = 7; msg.value = 3.0f;
        std::string s = "test";
        ser.write(oss, n); ser.write(oss, msg); ser.write(oss, s);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        int32_t rn = 0; SimpleMsg rmsg; std::string rs;
        ser.read(iss, rn); ser.read(iss, rmsg); ser.read(iss, rs);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(rn == 42);
        ASSERT_TRUE(rmsg.id == 7 && rmsg.value == 3.0f);
        ASSERT_TRUE(rs == "test");
    }
}

static void EmptyStringBugFixTest()
{
    serialize ser;
    {
        std::string src;
        std::string dst = "previous data";
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.empty());
    }
    {
        std::wstring src;
        std::wstring dst = L"previous data";
        std::ostringstream oss; ser.write(oss, src);
        std::istringstream iss(oss.str()); ser.read(iss, dst);
        ASSERT_TRUE(dst.empty());
    }
}

class NonSeekableBuffer : public std::stringbuf
{
protected:
    pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
    pos_type seekpos(pos_type, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
};

static void NonSeekableStreamTest()
{
    serialize ser;
    SimpleMsg msg;
    NonSeekableBuffer buf;
    std::iostream nss(&buf);
    ser.clearLastError();
    ser.write(nss, msg);
    ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::NON_SEEKABLE_STREAM);
    ASSERT_TRUE(nss.fail());
}

static void SerializeTests()
{
    PrimitiveTests();
    StringTests();
    CharArrayTests();
    UserDefinedObjectTests();
    VersioningTests();
    ContainerValueTests();
    ContainerPointerTests();
    EndiannessTests();
    ErrorTests();
    MultiObjectStreamTests();
    EmptyStringBugFixTest();
    NonSeekableStreamTest();
    cout << "All serialize tests passed." << endl;
}

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
    }

    // File example
    {
        SerializeToFile(outData);
        AllData allData;
        DeserializeFromFile(allData);

        // Spot check a few values
        assert(allData.color == Color::BLUE);
        assert(allData.dataListInt.size() == 2);
        assert(allData.dataMapInt.size() == 2);
        assert(allData.valueInt == 4);
        assert(allData.valueInt32 == 32);
        assert(allData.valueInt64 == 64);
    }

    // Stringstream example
    {
        stringstream ss(ios::in | ios::out | ios::binary);
        SerializeToStringstream(ss, outData);
        AllData allData;
        DeserializeFromStringstream(ss, allData);

        // Spot check a few values
        assert(allData.color == Color::BLUE);
        assert(allData.dataListInt.size() == 2);
        assert(allData.dataMapInt.size() == 2);
        assert(allData.valueInt == 4);
        assert(allData.valueInt32 == 32);
        assert(allData.valueInt64 == 64);
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

    SerializeTests();

    return 0;
}
