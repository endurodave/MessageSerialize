/// @file SerializeTests.cpp
/// @brief Unit tests for msg_serialize.h
///
/// Covers: primitives, strings, char arrays, user-defined objects, nested
/// objects, protocol versioning, all supported container types (by value and
/// by pointer), endianness, error detection, and the char* maxLen bounds check
/// introduced in the code review.
///
/// @see https://github.com/endurodave/MessageSerialize
/// David Lafreniere, 2026.

#include "msg_serialize.h"
#include "xtypes.h"          // allocator/: dmq::xstring, xlist, xmap, ...
#ifdef DMQ_ALLOCATOR
#include "xallocator.h"
#include "Allocator.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <memory>

#include <sstream>
#include <cstring>
#include <cstdint>
#include <array>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>

// Checked in every build type (unlike assert(), which NDEBUG removes)
#define ASSERT_TRUE(expr) \
    do { if (!(expr)) { std::fprintf(stderr, "TEST FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__); std::exit(1); } } while (0)

// Limits the tests check against (msg_serialize.h configuration)
static constexpr size_t MAX_STRING = MSG_SERIALIZE_MAX_STRING_SIZE;
static constexpr size_t MAX_CONTAINER = MSG_SERIALIZE_MAX_CONTAINER_SIZE;

/// Minimal non-default allocator, to check strings and containers with a
/// custom allocator serialize like the std:: defaults.
template <class T>
struct TestAllocator : std::allocator<T>
{
    template <class U> struct rebind { using other = TestAllocator<U>; };
    TestAllocator() = default;
    template <class U> TestAllocator(const TestAllocator<U>&) {}
};
template <class T, class U>
bool operator==(const TestAllocator<T>&, const TestAllocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const TestAllocator<T>&, const TestAllocator<U>&) { return false; }

// ---------------------------------------------------------------------------
// Test message types
// ---------------------------------------------------------------------------

struct SimpleMsg : public serialize::I
{
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

/// Chain of N nested USER_DEFINED levels (DeepMsg<N> contains DeepMsg<N-1>, down
/// to the DeepMsg<0> leaf). Used to exceed MAX_PARSE_STACK_DEPTH on read.
template <int N>
struct DeepMsg : public serialize::I
{
    DeepMsg<N - 1> inner;

    std::ostream& write(serialize& ms, std::ostream& os) override { return ms.write(os, inner); }
    std::istream& read(serialize& ms, std::istream& is) override { return ms.read(is, inner); }
};

template <>
struct DeepMsg<0> : public serialize::I
{
    int32_t val = 0;

    std::ostream& write(serialize& ms, std::ostream& os) override { return ms.write(os, val); }
    std::istream& read(serialize& ms, std::istream& is) override { return ms.read(is, val); }
};

/// Version 1: one field.
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

/// Version 2: two fields (backward-compatible superset of V1).
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
// Helpers
// ---------------------------------------------------------------------------

/// Serialize src, deserialize into dst; returns true if both streams stayed good.
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
    // eof is acceptable after consuming all bytes
    return !iss.fail();
}

/// Serialize a single primitive value and deserialize it back.
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
// Primitive tests
// ---------------------------------------------------------------------------

static void PrimitiveTests()
{
    // bool
    {
        bool out = false;
        ASSERT_TRUE(RoundTripPrimitive(true, out));
        ASSERT_TRUE(out == true);
        ASSERT_TRUE(RoundTripPrimitive(false, out));
        ASSERT_TRUE(out == false);
    }

    // int8_t / uint8_t (single byte — no endian swap path exercised)
    {
        int8_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int8_t)-128, out));
        ASSERT_TRUE(out == -128);

        uint8_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint8_t)255, uout));
        ASSERT_TRUE(uout == 255);
    }

    // int16_t / uint16_t
    {
        int16_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int16_t)-32768, out));
        ASSERT_TRUE(out == -32768);

        uint16_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint16_t)65535, uout));
        ASSERT_TRUE(uout == 65535);
    }

    // int32_t / uint32_t
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

    // int64_t / uint64_t
    {
        int64_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive((int64_t)0x0102030405060708LL, out));
        ASSERT_TRUE(out == 0x0102030405060708LL);

        uint64_t uout = 0;
        ASSERT_TRUE(RoundTripPrimitive((uint64_t)0xFFFFFFFFFFFFFFFFULL, uout));
        ASSERT_TRUE(uout == 0xFFFFFFFFFFFFFFFFULL);
    }

    // float
    {
        float out = 0.0f;
        ASSERT_TRUE(RoundTripPrimitive(3.14159f, out));
        ASSERT_TRUE(out == 3.14159f);
        ASSERT_TRUE(RoundTripPrimitive(-0.0f, out));
        ASSERT_TRUE(out == -0.0f);
    }

    // double
    {
        double out = 0.0;
        ASSERT_TRUE(RoundTripPrimitive(2.718281828459045, out));
        ASSERT_TRUE(out == 2.718281828459045);
    }

    // Zero values
    {
        int32_t out = 99;
        ASSERT_TRUE(RoundTripPrimitive((int32_t)0, out));
        ASSERT_TRUE(out == 0);
    }
}

// ---------------------------------------------------------------------------
// std::string tests
// ---------------------------------------------------------------------------

static void StringTests()
{
    serialize ser;

    // Normal round-trip
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

    // Empty string
    {
        std::string src;
        std::string dst = "non-empty";
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        // empty string returns false from check_slength (size==0), dst unchanged
        ASSERT_TRUE(dst.empty() || dst == "non-empty"); // implementation skips read
    }

    // String at max allowed length (MSG_SERIALIZE_MAX_STRING_SIZE)
    {
        std::string src(MAX_STRING, 'A');
        std::string dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }

    // String longer than MAX_STRING_SIZE — write sets failbit
    {
        std::string src(MAX_STRING + 1, 'B');
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, src);
        ASSERT_TRUE(!oss.good());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
    }

    // std::wstring round-trip
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

// ---------------------------------------------------------------------------
// char* (bounded) tests
// ---------------------------------------------------------------------------

static void CharArrayTests()
{
    serialize ser;

    // Normal round-trip within buffer
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

    // Buffer exactly fits (including null terminator)
    {
        const char* src = "abc";           // 4 bytes with null
        char dst[4] = {};
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(std::strcmp(dst, src) == 0);
    }

    // Destination buffer too small — read must set failbit
    {
        const char* src = "toolongstring";  // 14 bytes with null
        char dst[4] = {};
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.clearLastError();
        ser.read(iss, dst, sizeof(dst));  // maxLen = 4, string needs 14
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
    }

    // Sent as std::string (no null terminator on the wire): read must terminate
    {
        std::string src = "hello";
        char dst[16];
        std::memset(dst, 'X', sizeof(dst));
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(std::strcmp(dst, "hello") == 0);
    }

    // std::string exactly filling the buffer leaves no room for the terminator:
    // read fails, and the buffer is still terminated
    {
        std::string src = "abcd";
        char dst[4];
        std::memset(dst, 'X', sizeof(dst));
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.clearLastError();
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
        ASSERT_TRUE(dst[sizeof(dst) - 1] == '\0');
    }

    // const char* of 65535+ chars: the length used to wrap to 16 bits and send
    // an empty string; now write fails before writing anything
    {
        std::string longStr(70000, 'L');
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, longStr.c_str());
        ASSERT_TRUE(oss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
        ASSERT_TRUE(oss.str().empty());
    }

    // const char* at the limit: written. One over: write fails
    {
        std::string at(MAX_STRING, 'A');
        std::ostringstream ok;
        ser.write(ok, at.c_str());
        ASSERT_TRUE(ok.good());
    }
    {
        std::string over(MAX_STRING + 1, 'O');
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, over.c_str());
        ASSERT_TRUE(oss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STRING_TOO_LONG);
    }

    // Sent as const char*, read into std::string: no terminator was sent, so
    // none is read back
    {
        std::string dst;
        std::ostringstream oss;
        ser.write(oss, "hello");
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == "hello" && dst.size() == 5);
    }

    // A std::string whose data ends in a zero byte keeps every byte
    {
        const std::string src("abc\0", 4);
        std::string dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src && dst.size() == 4);
    }

    // An older sender's const char* (terminator on the wire) read into a char
    // buffer: still read correctly
    {
        const char legacy[] = "hi";
        std::ostringstream oss;
        ser.write(oss, std::string(legacy, sizeof(legacy)));   // "hi\0", 3 bytes
        char dst[3];
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(std::strcmp(dst, "hi") == 0);
    }

    // Truncated stream: payload shorter than its length field. Read must report
    // the stream error (not STRING_TOO_LONG) and leave dst an empty C string,
    // not read the bytes is.read() never filled.
    {
        std::string src = "abcd";
        char dst[4];
        std::memset(dst, 'X', sizeof(dst));
        std::ostringstream oss;
        ser.write(oss, src);
        std::string wire = oss.str();
        wire.resize(wire.size() - 2);   // drop the last 2 payload bytes
        std::istringstream iss(wire);
        ser.clearLastError();
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::STREAM_ERROR);
        ASSERT_TRUE(dst[0] == '\0');
    }

    // Too long for the buffer: dst is still left an empty C string
    {
        const char* src = "toolong";
        char dst[4];
        std::memset(dst, 'X', sizeof(dst));
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(dst[0] == '\0');
    }

    // Empty std::string: read gives an empty C string
    {
        std::string src;
        char dst[8];
        std::memset(dst, 'X', sizeof(dst));
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst, sizeof(dst));
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst[0] == '\0');
    }
}

// ---------------------------------------------------------------------------
// const containers
// ---------------------------------------------------------------------------

/// Writing a const container. The container write() overloads took non-const
/// references, so a const container (e.g. a remote delegate argument, which
/// DmqSerializer passes as const Args&) fell to the generic template and failed
/// to compile.
template <typename C>
static bool RoundTripConst(const C& src, C& dst)
{
    serialize ser;
    std::ostringstream oss;
    ser.write(oss, src);
    if (!oss.good())
        return false;
    std::istringstream iss(oss.str());
    ser.read(iss, dst);
    return !iss.fail();
}

static void ConstContainerTests()
{
    {
        const std::vector<int32_t> src{ 1, 2, 3 };
        std::vector<int32_t> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::vector<bool> src{ true, false, true };
        std::vector<bool> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::list<std::string> src{ "a", "b" };
        std::list<std::string> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::set<int32_t> src{ 5, 6 };
        std::set<int32_t> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::map<std::string, int32_t> src{ { "x", 1 } };
        std::map<std::string, int32_t> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    // Pointer containers
    {
        SimpleMsg a;
        a.id = 11;
        const std::vector<SimpleMsg*> src{ &a, nullptr };
        std::vector<SimpleMsg*> dst;
        ASSERT_TRUE(RoundTripConst(src, dst));
        ASSERT_TRUE(dst.size() == 2 && dst[0] && dst[0]->id == 11 && dst[1] == nullptr);
        delete dst[0];
    }
    {
        SimpleMsg a;
        a.id = 12;
        const std::list<SimpleMsg*> src{ &a };
        std::list<SimpleMsg*> dst;
        ASSERT_TRUE(RoundTripConst(src, dst));
        ASSERT_TRUE(dst.size() == 1 && dst.front()->id == 12);
        delete dst.front();
    }
    {
        SimpleMsg a;
        a.id = 13;
        const std::set<SimpleMsg*> src{ &a };
        std::set<SimpleMsg*> dst;
        ASSERT_TRUE(RoundTripConst(src, dst));
        ASSERT_TRUE(dst.size() == 1 && (*dst.begin())->id == 13);
        delete *dst.begin();
    }
    {
        SimpleMsg a;
        a.id = 14;
        const std::map<int32_t, SimpleMsg*> src{ { 1, &a } };
        std::map<int32_t, SimpleMsg*> dst;
        ASSERT_TRUE(RoundTripConst(src, dst));
        ASSERT_TRUE(dst.size() == 1 && dst[1]->id == 14);
        delete dst[1];
    }
}

// ---------------------------------------------------------------------------
// Nested containers
// ---------------------------------------------------------------------------

/// Containers as container elements or map values. Elements used to go through
/// write/read(x, false), the generic template, which doesn't handle containers.
static void NestedContainerTests()
{
    {
        const std::vector<std::vector<int32_t>> src{ { 1, 2 }, {}, { 3 } };
        std::vector<std::vector<int32_t>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::vector<std::vector<std::vector<int32_t>>> src{ { { 1 }, { 2, 3 } }, { { 4 } } };
        std::vector<std::vector<std::vector<int32_t>>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::list<std::vector<std::string>> src{ { "a", "b" }, { "c" } };
        std::list<std::vector<std::string>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::set<std::vector<int32_t>> src{ { 1, 2 }, { 3 } };
        std::set<std::vector<int32_t>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::map<std::string, std::vector<int32_t>> src{ { "x", { 1, 2 } }, { "y", {} } };
        std::map<std::string, std::vector<int32_t>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::vector<std::map<int32_t, std::string>> src{ { { 1, "one" } }, { { 2, "two" }, { 3, "three" } } };
        std::vector<std::map<int32_t, std::string>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::vector<std::vector<bool>> src{ { true, false }, { true } };
        std::vector<std::vector<bool>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::map<int32_t, std::list<int32_t>> src{ { 1, { 5, 6 } } };
        std::map<int32_t, std::list<int32_t>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    // Inner container over the limit fails the whole write
    {
        const std::vector<std::vector<int32_t>> src{ std::vector<int32_t>(MAX_CONTAINER + 1, 0) };
        std::vector<std::vector<int32_t>> dst;
        ASSERT_TRUE(!RoundTripConst(src, dst));
    }
    // Truncated nested payload: read fails
    {
        serialize ser;
        const std::vector<std::vector<int32_t>> src{ { 1, 2, 3 } };
        std::ostringstream oss;
        ser.write(oss, src);
        std::string wire = oss.str();
        wire.resize(wire.size() - 3);
        std::istringstream iss(wire);
        std::vector<std::vector<int32_t>> dst;
        ser.read(iss, dst);
        ASSERT_TRUE(iss.fail());
    }
}

// ---------------------------------------------------------------------------
// std::array, and vector reserve
// ---------------------------------------------------------------------------

static void ArrayTests()
{
    {
        const std::array<int32_t, 3> src{ { 1, 2, 3 } };
        std::array<int32_t, 3> dst{};
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        std::array<std::string, 2> src{ { "a", "bc" } };    // non-const write
        std::array<std::string, 2> dst{};
        serialize ser;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail() && dst == src);
    }
    {
        std::array<SimpleMsg, 2> src;
        src[0].id = 1;
        src[1].id = 2;
        const auto& csrc = src;
        std::array<SimpleMsg, 2> dst;
        serialize ser;
        std::ostringstream oss;
        ser.write(oss, csrc);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail() && dst[0].id == 1 && dst[1].id == 2);
    }
    // Same wire format as a vector: vector -> array and array -> vector
    {
        serialize ser;
        const std::vector<int32_t> v{ 4, 5 };
        std::ostringstream oss;
        ser.write(oss, v);
        std::istringstream iss(oss.str());
        std::array<int32_t, 2> a{};
        ser.read(iss, a);
        ASSERT_TRUE(!iss.fail() && a[0] == 4 && a[1] == 5);

        std::ostringstream oss2;
        ser.write(oss2, a);
        std::istringstream iss2(oss2.str());
        std::vector<int32_t> v2;
        ser.read(iss2, v2);
        ASSERT_TRUE(!iss2.fail() && v2 == v);
    }
    // Received count differs from N: read fails, array unchanged
    {
        serialize ser;
        const std::vector<int32_t> v{ 1, 2, 3 };
        std::ostringstream oss;
        ser.write(oss, v);
        std::istringstream iss(oss.str());
        std::array<int32_t, 2> a{ { 9, 9 } };
        ser.clearLastError();
        ser.read(iss, a);
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::INVALID_INPUT);
        ASSERT_TRUE(a[0] == 9 && a[1] == 9);
    }
    // Nested: arrays in containers, containers in arrays
    {
        const std::vector<std::array<int32_t, 2>> src{ { { 1, 2 } }, { { 3, 4 } } };
        std::vector<std::array<int32_t, 2>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::array<std::vector<int32_t>, 2> src{ { { 1 }, { 2, 3 } } };
        std::array<std::vector<int32_t>, 2> dst{};
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    {
        const std::map<int32_t, std::array<int32_t, 2>> src{ { 1, { { 7, 8 } } } };
        std::map<int32_t, std::array<int32_t, 2>> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
    }
    // vector read reserves the received count: one allocation, no regrowth
    {
        const std::vector<int32_t> src(50, 1);
        std::vector<int32_t> dst;
        ASSERT_TRUE(RoundTripConst(src, dst) && dst == src);
        ASSERT_TRUE(dst.capacity() == src.size());
    }
}

// ---------------------------------------------------------------------------
// Containers of strings
// ---------------------------------------------------------------------------

/// Container elements and map keys that are strings. The element loops used to
/// call write/read(x, false), which for a string picked the generic template:
/// writing didn't compile and reading cast the string to serialize::I*.
static void ContainerStringTests()
{
    serialize ser;

    {
        std::vector<std::string> src{ "alpha", "", "gamma" };
        std::vector<std::string> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::list<std::string> src{ "one", "two" };
        std::list<std::string> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::set<std::string> src{ "x", "y", "z" };
        std::set<std::string> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::map<std::string, int32_t> src{ { "a", 1 }, { "bb", 2 } };
        std::map<std::string, int32_t> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::map<int32_t, std::string> src{ { 1, "one" }, { 2, "two" } };
        std::map<int32_t, std::string> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    {
        std::vector<std::wstring> src{ L"wide", L"strings" };
        std::vector<std::wstring> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }
    // String keys with pointer values
    {
        SimpleMsg* m = new SimpleMsg();
        m->id = 7;
        std::map<std::string, SimpleMsg*> src{ { "k", m }, { "null", nullptr } };
        std::map<std::string, SimpleMsg*> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst["k"] != nullptr && dst["k"]->id == 7);
        ASSERT_TRUE(dst["null"] == nullptr);
        delete m;
        delete dst["k"];
    }
    // Strings and containers with a custom allocator
    {
        using AString = std::basic_string<char, std::char_traits<char>, TestAllocator<char>>;
        std::list<AString, TestAllocator<AString>> src{ "p", "q" };
        std::list<AString, TestAllocator<AString>> dst;
        std::map<int32_t, AString, std::less<int32_t>, TestAllocator<std::pair<const int32_t, AString>>> msrc{ { 1, "m" } };
        std::map<int32_t, AString, std::less<int32_t>, TestAllocator<std::pair<const int32_t, AString>>> mdst;
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        ser.write(ss, src);
        ser.write(ss, msrc);
        ser.read(ss, dst);
        ser.read(ss, mdst);
        ASSERT_TRUE(!ss.fail());
        ASSERT_TRUE(dst == src);
        ASSERT_TRUE(mdst == msrc);
    }
}

// ---------------------------------------------------------------------------
// User-defined object tests
// ---------------------------------------------------------------------------

static void UserDefinedObjectTests()
{
    // Simple round-trip
    {
        SimpleMsg src;
        src.id    = 42;
        src.value = 1.23f;
        SimpleMsg dst;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.id    == src.id);
        ASSERT_TRUE(dst.value == src.value);
    }

    // Default-constructed values survive round-trip
    {
        SimpleMsg src;
        SimpleMsg dst;
        dst.id    = 99;
        dst.value = 9.9f;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.id    == 0);
        ASSERT_TRUE(dst.value == 0.0f);
    }

    // Nested user-defined objects
    {
        NestedMsg src;
        src.inner.id    = 7;
        src.inner.value = 3.14f;
        src.extra       = -100;
        NestedMsg dst;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.inner.id    == src.inner.id);
        ASSERT_TRUE(dst.inner.value == src.inner.value);
        ASSERT_TRUE(dst.extra       == src.extra);
    }

    // All primitive types survive round-trip
    {
        AllPrimitivesMsg src;
        src.b   = true;
        src.i8  = -1;
        src.u8  = 255;
        src.i16 = -32768;
        src.u16 = 65535;
        src.i32 = 0x12345678;
        src.u32 = 0xDEADBEEFu;
        src.i64 = 0x0102030405060708LL;
        src.u64 = 0xFFFFFFFFFFFFFFFFULL;
        src.f   = 3.14159f;
        src.d   = 2.71828182845;
        AllPrimitivesMsg dst;
        ASSERT_TRUE(RoundTrip(src, dst));
        ASSERT_TRUE(dst.b   == src.b);
        ASSERT_TRUE(dst.i8  == src.i8);
        ASSERT_TRUE(dst.u8  == src.u8);
        ASSERT_TRUE(dst.i16 == src.i16);
        ASSERT_TRUE(dst.u16 == src.u16);
        ASSERT_TRUE(dst.i32 == src.i32);
        ASSERT_TRUE(dst.u32 == src.u32);
        ASSERT_TRUE(dst.i64 == src.i64);
        ASSERT_TRUE(dst.u64 == src.u64);
        ASSERT_TRUE(dst.f   == src.f);
        ASSERT_TRUE(dst.d   == src.d);
    }
}

// ---------------------------------------------------------------------------
// Protocol versioning tests
// ---------------------------------------------------------------------------

static void VersioningTests()
{
    serialize ser;

    // Sender writes V2 (larger), receiver reads V1 (smaller).
    // Extra field2 bytes must be silently discarded; field1 must be correct.
    {
        MsgV2 v2src;
        v2src.field1 = 111;
        v2src.field2 = 222;

        std::ostringstream oss;
        ser.write(oss, v2src);
        ASSERT_TRUE(oss.good());

        MsgV1 v1dst;
        std::istringstream iss(oss.str());
        ser.read(iss, v1dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(v1dst.field1 == 111);
    }

    // Sender writes V1 (smaller), receiver reads V2 (larger).
    // field2 is never populated — should remain default (0); stream stays valid.
    {
        MsgV1 v1src;
        v1src.field1 = 333;

        std::ostringstream oss;
        ser.write(oss, v1src);
        ASSERT_TRUE(oss.good());

        MsgV2 v2dst;
        v2dst.field2 = 999;  // pre-set to detect if it is wrongly overwritten
        std::istringstream iss(oss.str());
        ser.read(iss, v2dst);
        ASSERT_TRUE(v2dst.field1 == 333);
        ASSERT_TRUE(v2dst.field2 == 999);  // must be unchanged (not read)
    }
}

// ---------------------------------------------------------------------------
// Container tests — by value
// ---------------------------------------------------------------------------

static void ContainerValueTests()
{
    serialize ser;

    // vector<int>
    {
        std::vector<int> src = {1, 2, 3, 100, -50};
        std::vector<int> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }

    // vector<int> — empty
    {
        std::vector<int> src;
        std::vector<int> dst = {9, 8, 7};
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(dst.empty());
    }

    // vector<bool> — special case (packed storage)
    {
        std::vector<bool> src = {true, false, true, true, false};
        std::vector<bool> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }

    // list<int>
    {
        std::list<int> src = {10, 20, 30};
        std::list<int> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }

    // set<int>
    {
        std::set<int> src = {5, 3, 1, 4, 2};
        std::set<int> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }

    // map<int, int>
    {
        std::map<int, int> src = {{1, 10}, {2, 20}, {3, 30}};
        std::map<int, int> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst == src);
    }

    // vector<SimpleMsg> — vector of user-defined objects
    {
        std::vector<SimpleMsg> src(3);
        src[0].id = 1; src[0].value = 1.1f;
        src[1].id = 2; src[1].value = 2.2f;
        src[2].id = 3; src[2].value = 3.3f;
        std::vector<SimpleMsg> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst.size() == 3);
        ASSERT_TRUE(dst[0].id == 1 && dst[0].value == 1.1f);
        ASSERT_TRUE(dst[1].id == 2 && dst[1].value == 2.2f);
        ASSERT_TRUE(dst[2].id == 3 && dst[2].value == 3.3f);
    }

    // map<int, SimpleMsg>
    {
        std::map<int, SimpleMsg> src;
        src[1].id = 10; src[1].value = 0.1f;
        src[2].id = 20; src[2].value = 0.2f;
        std::map<int, SimpleMsg> dst;
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[1].id == 10 && dst[1].value == 0.1f);
        ASSERT_TRUE(dst[2].id == 20 && dst[2].value == 0.2f);
    }
}

// ---------------------------------------------------------------------------
// Container tests — by pointer (heap-allocated on deserialize)
// ---------------------------------------------------------------------------

static void ContainerPointerTests()
{
    serialize ser;

    // vector<SimpleMsg*>
    {
        SimpleMsg a, b;
        a.id = 1; a.value = 1.0f;
        b.id = 2; b.value = 2.0f;
        std::vector<SimpleMsg*> src = {&a, &b};

        std::ostringstream oss;
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());

        std::vector<SimpleMsg*> dst;
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[0] != nullptr && dst[0]->id == 1 && dst[0]->value == 1.0f);
        ASSERT_TRUE(dst[1] != nullptr && dst[1]->id == 2 && dst[1]->value == 2.0f);
        for (auto* p : dst) delete p;
    }

    // vector<SimpleMsg*> with a nullptr element
    {
        SimpleMsg a;
        a.id = 5; a.value = 5.0f;
        std::vector<SimpleMsg*> src = {&a, nullptr};

        std::ostringstream oss;
        ser.write(oss, src);

        std::vector<SimpleMsg*> dst;
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[0] != nullptr && dst[0]->id == 5);
        ASSERT_TRUE(dst[1] == nullptr);
        delete dst[0];
    }

    // list<SimpleMsg*>
    {
        SimpleMsg a;
        a.id = 7; a.value = 7.7f;
        std::list<SimpleMsg*> src = {&a};

        std::ostringstream oss;
        ser.write(oss, src);

        std::list<SimpleMsg*> dst;
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 1);
        ASSERT_TRUE(dst.front() != nullptr && dst.front()->id == 7);
        delete dst.front();
    }

    // map<int, SimpleMsg*>
    {
        SimpleMsg a;
        a.id = 11; a.value = 11.0f;
        std::map<int, SimpleMsg*> src;
        src[1] = &a;
        src[2] = nullptr;

        std::ostringstream oss;
        ser.write(oss, src);

        std::map<int, SimpleMsg*> dst;
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 2);
        ASSERT_TRUE(dst[1] != nullptr && dst[1]->id == 11);
        ASSERT_TRUE(dst[2] == nullptr);
        delete dst[1];
    }

    // set<SimpleMsg*>
    {
        SimpleMsg a;
        a.id = 99; a.value = 9.9f;
        std::set<SimpleMsg*> src = {&a};

        std::ostringstream oss;
        ser.write(oss, src);

        std::set<SimpleMsg*> dst;
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        ASSERT_TRUE(dst.size() == 1);
        ASSERT_TRUE((*dst.begin()) != nullptr && (*dst.begin())->id == 99);
        delete *dst.begin();
    }
}

// ---------------------------------------------------------------------------
// Endianness tests
// ---------------------------------------------------------------------------

static void EndiannessTests()
{
    serialize ser;

    // writeEndian / readEndian round-trip
    {
        std::ostringstream oss;
        ser.writeEndian(oss);
        ASSERT_TRUE(oss.good());

        bool le = !serialize::LE();  // start with opposite value
        std::istringstream iss(oss.str());
        ser.readEndian(iss, le);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(le == serialize::LE());
    }

    // Multi-byte integer: value must survive the endian encode/decode cycle
    // regardless of host byte order.
    {
        const int32_t val = 0x01020304;
        int32_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive(val, out));
        ASSERT_TRUE(out == val);
    }

    // uint64_t with all distinct bytes — a byte-order error would corrupt it
    {
        const uint64_t val = 0x0102030405060708ULL;
        uint64_t out = 0;
        ASSERT_TRUE(RoundTripPrimitive(val, out));
        ASSERT_TRUE(out == val);
    }

    // Verify LE() is consistent across calls
    {
        ASSERT_TRUE(serialize::LE() == serialize::LE());
    }
}

// ---------------------------------------------------------------------------
// Error detection tests
// ---------------------------------------------------------------------------

static void ErrorTests()
{
    serialize ser;

    // Type mismatch: write std::string (STRING tag), try to read int32_t (LITERAL tag)
    // — the tag byte differs so read_type must set failbit and report TYPE_MISMATCH.
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

    // Reading from empty stream — should fail gracefully
    {
        std::istringstream iss("");
        int32_t dst = 0;
        ser.clearLastError();
        ser.read(iss, dst);
        ASSERT_TRUE(iss.fail());
    }

    // Container exceeds MAX_CONTAINER_SIZE: write should fail
    {
        std::vector<int> big(MAX_CONTAINER + 1, 0);
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, big);
        ASSERT_TRUE(!oss.good());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::CONTAINER_TOO_MANY);
    }

    // USER_DEFINED nesting exceeds MSG_SERIALIZE_MAX_NESTING on read: must fail
    // that one parse gracefully (failbit + NESTING_TOO_DEEP), not hard-fault the
    // process. Write has no depth limit (only read() tracks stop-parse positions),
    // so a DeepMsg nested three levels past the limit writes fine and only
    // fails coming back on read.
    {
        DeepMsg<MSG_SERIALIZE_MAX_NESTING + 3> src;
        std::ostringstream oss;
        ser.clearLastError();
        ser.write(oss, src);
        ASSERT_TRUE(oss.good());

        DeepMsg<MSG_SERIALIZE_MAX_NESTING + 3> dst;
        std::istringstream iss(oss.str());
        ser.clearLastError();
        ser.read(iss, dst);
        ASSERT_TRUE(iss.fail());
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::NESTING_TOO_DEEP);
    }

    // Error handler callback is invoked on error
    {
        static bool handlerCalled = false;
        static serialize::ParsingError lastErr = serialize::ParsingError::NONE;

        serialize localSer;
        localSer.setErrorHandler([](serialize::ParsingError err, int, const char*) {
            handlerCalled = true;
            lastErr = err;
        });

        std::ostringstream oss;
        std::string tooBig(MAX_STRING + 1, 'X');
        localSer.write(oss, tooBig);

        ASSERT_TRUE(handlerCalled);
        ASSERT_TRUE(lastErr == serialize::ParsingError::STRING_TOO_LONG);
    }

    // clearLastError resets state
    {
        std::ostringstream oss;
        ser.write(oss, std::string(MAX_STRING + 1, 'X'));   // sets an error
        ASSERT_TRUE(ser.getLastError() != serialize::ParsingError::NONE);
        ser.clearLastError();
        ASSERT_TRUE(ser.getLastError() == serialize::ParsingError::NONE);
    }
}

// ---------------------------------------------------------------------------
// Multiple objects in one stream
// ---------------------------------------------------------------------------

static void MultiObjectStreamTests()
{
    serialize ser;

    // Write two independent objects into one stream; read them back in order.
    {
        SimpleMsg a, b;
        a.id = 1; a.value = 1.1f;
        b.id = 2; b.value = 2.2f;

        std::ostringstream oss;
        ser.write(oss, a);
        ser.write(oss, b);
        ASSERT_TRUE(oss.good());

        std::istringstream iss(oss.str());
        SimpleMsg ra, rb;
        ser.read(iss, ra);
        ser.read(iss, rb);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(ra.id == 1 && ra.value == 1.1f);
        ASSERT_TRUE(rb.id == 2 && rb.value == 2.2f);
    }

    // Mix of primitives and user objects in one stream.
    {
        std::ostringstream oss;
        int32_t n = 42;
        SimpleMsg msg;
        msg.id = 7; msg.value = 3.0f;
        std::string s = "test";

        ser.write(oss, n);
        ser.write(oss, msg);
        ser.write(oss, s);
        ASSERT_TRUE(oss.good());

        std::istringstream iss(oss.str());
        int32_t rn = 0;
        SimpleMsg rmsg;
        std::string rs;
        ser.read(iss, rn);
        ser.read(iss, rmsg);
        ser.read(iss, rs);
        ASSERT_TRUE(!iss.fail());
        ASSERT_TRUE(rn == 42);
        ASSERT_TRUE(rmsg.id == 7 && rmsg.value == 3.0f);
        ASSERT_TRUE(rs == "test");
    }
}

// ---------------------------------------------------------------------------
// Safety and Bug Fix Tests (from code review)
// ---------------------------------------------------------------------------

/// Verify that reading a zero-length string correctly clears the target string.
static void EmptyStringBugFixTest()
{
    serialize ser;

    // std::string
    {
        std::string src = "";
        std::string dst = "previous data";
        
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        
        ASSERT_TRUE(dst.empty());
    }

    // std::wstring
    {
        std::wstring src = L"";
        std::wstring dst = L"previous data";
        
        std::ostringstream oss;
        ser.write(oss, src);
        std::istringstream iss(oss.str());
        ser.read(iss, dst);
        
        ASSERT_TRUE(dst.empty());
    }
}

/// A custom stream buffer that forces tellp/tellg/seekp to return error (-1).
class NonSeekableBuffer : public std::stringbuf
{
protected:
    pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
    pos_type seekpos(pos_type, std::ios_base::openmode) override { return pos_type(off_type(-1)); }
};

/// Verify that serializing an object into a non-seekable stream triggers an error.
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

// ---------------------------------------------------------------------------
// dmq::x* types (allocator/xtypes.h): fixed-block allocator with
// MSG_SERIALIZE_ALLOCATOR=ON, global heap otherwise
// ---------------------------------------------------------------------------

#ifdef DMQ_ALLOCATOR
/// Allocations made so far by every xallocator block-size pool up to 8 KB.
static uint32_t PoolAllocations()
{
    uint32_t total = 0;
    for (size_t block = 16; block <= 8192; block <<= 1)
    {
        const size_t overhead = XALLOC_BLOCK_HEADER_SIZE + XALLOC_BLOCK_FOOTER_SIZE;
        if (block < overhead)
            continue;
        total += xallocator_get_allocator(block - overhead)->GetAllocations();
    }
    return total;
}
#endif

/// A message with x* fields, itself allocated with XALLOCATOR.
struct XMsg : public serialize::I
{
    XALLOCATOR
public:
    dmq::xstring name;
    dmq::xlist<dmq::xstring> tags;
    dmq::xmap<int32_t, dmq::xstring> labels;
    dmq::xset<int32_t> ids;
    std::vector<int32_t, dmq::stl_allocator<int32_t>> values;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, name);
        ms.write(os, tags);
        ms.write(os, labels);
        ms.write(os, ids);
        return ms.write(os, values);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, name);
        ms.read(is, tags);
        ms.read(is, labels);
        ms.read(is, ids);
        return ms.read(is, values);
    }
};

static void AllocatorTypeTests()
{
    serialize ser;
#ifdef DMQ_ALLOCATOR
    const uint32_t before = PoolAllocations();
#endif

    // Round trip of every x* type, through a dmq::xstringstream
    {
        XMsg* src = new XMsg();
        src->name = "a name longer than any small-string buffer";
        src->tags = { "red", "a tag longer than any small-string buffer" };
        src->labels = { { 1, "one" }, { 2, "two" } };
        src->ids = { 3, 1, 2 };
        src->values = { 7, 8, 9 };

        dmq::xstringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        ser.write(ss, *src);
        ASSERT_TRUE(ss.good());

        XMsg* dst = new XMsg();
        ser.read(ss, *dst);
        ASSERT_TRUE(!ss.fail());
        ASSERT_TRUE(dst->name == src->name);
        ASSERT_TRUE(dst->tags == src->tags);
        ASSERT_TRUE(dst->labels == src->labels);
        ASSERT_TRUE(dst->ids == src->ids);
        ASSERT_TRUE(dst->values == src->values);
        delete src;
        delete dst;
    }

    // Nested x* containers
    {
        const dmq::xlist<dmq::xlist<int32_t>> src{ { 1, 2 }, {}, { 3 } };
        dmq::xlist<dmq::xlist<int32_t>> dst;
        dmq::xstringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        ser.write(ss, src);
        ser.read(ss, dst);
        ASSERT_TRUE(!ss.fail() && dst == src);
    }

    // The allocator doesn't change the wire format: x* types written, std::
    // types read back (and the other way)
    {
        const dmq::xlist<dmq::xstring> xsrc{ "p", "q" };
        const dmq::xmap<int32_t, dmq::xstring> xmsrc{ { 1, "m" } };
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        ser.write(ss, xsrc);
        ser.write(ss, xmsrc);
        std::list<std::string> dst;
        std::map<int32_t, std::string> mdst;
        ser.read(ss, dst);
        ser.read(ss, mdst);
        ASSERT_TRUE(!ss.fail());
        ASSERT_TRUE(dst == (std::list<std::string>{ "p", "q" }));
        ASSERT_TRUE(mdst == (std::map<int32_t, std::string>{ { 1, "m" } }));

        std::stringstream ss2(std::ios::in | std::ios::out | std::ios::binary);
        ser.write(ss2, dst);
        dmq::xlist<dmq::xstring> xdst;
        ser.read(ss2, xdst);
        ASSERT_TRUE(!ss2.fail() && xdst == xsrc);
    }

#ifdef DMQ_ALLOCATOR
    // The x* types and XALLOCATOR really used the fixed-block pools
    ASSERT_TRUE(PoolAllocations() > before);
#endif
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void SerializeTests()
{
    PrimitiveTests();
    StringTests();
    CharArrayTests();
    UserDefinedObjectTests();
    VersioningTests();
    ContainerValueTests();
    ContainerStringTests();
    ConstContainerTests();
    NestedContainerTests();
    ArrayTests();
    ContainerPointerTests();
    EndiannessTests();
    ErrorTests();
    MultiObjectStreamTests();
    EmptyStringBugFixTest();
    NonSeekableStreamTest();
    AllocatorTypeTests();
#ifdef DMQ_ALLOCATOR
    std::printf("All serialize tests passed (fixed-block allocator).\n");
#else
    std::printf("All serialize tests passed (global heap).\n");
#endif
}
