// MessageSerialize PlatformIO example: write a message to a binary stream,
// read it back, and check the copy. Runs on Arduino boards and the desktop
// (native).
//
// A message class derives from serialize::I and writes/reads each field.
// Here the bytes go to a std::stringstream; on a device they would go to a
// UART, socket or flash instead.
//
// @see https://github.com/endurodave/MessageSerialize

#include "msg_serialize.h"
#include <sstream>
#include <stdio.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

static void Log(const char* msg)
{
#ifdef ARDUINO
    Serial.println(msg);
#else
    printf("%s\n", msg);
#endif
}

// A nested message
class Position : public serialize::I
{
public:
    float latitude = 0;
    float longitude = 0;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, latitude);
        return ms.write(os, longitude);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, latitude);
        return ms.read(is, longitude);
    }
};

// The message sent between nodes
class SensorReport : public serialize::I
{
public:
    uint16_t id = 0;
    std::string name;
    Position position;
    std::array<int16_t, 4> samples{};               // fixed size: no heap
    std::vector<uint8_t> flags;
    std::map<std::string, int32_t> counters;

    std::ostream& write(serialize& ms, std::ostream& os) override
    {
        ms.write(os, id);
        ms.write(os, name);
        ms.write(os, position);
        ms.write(os, samples);
        ms.write(os, flags);
        return ms.write(os, counters);
    }
    std::istream& read(serialize& ms, std::istream& is) override
    {
        ms.read(is, id);
        ms.read(is, name);
        ms.read(is, position);
        ms.read(is, samples);
        ms.read(is, flags);
        return ms.read(is, counters);
    }
};

static bool Demo()
{
    char buf[96];
    serialize ms;

    SensorReport out;
    out.id = 42;
    out.name = "pump-3";
    out.position.latitude = 44.98f;
    out.position.longitude = -93.27f;
    out.samples = { { 100, -200, 300, -400 } };
    out.flags = { 1, 0, 1 };
    out.counters["starts"] = 12;
    out.counters["faults"] = 1;

    // Write the message
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ms.write(ss, out);
    const std::string bytes = ss.str();
    snprintf(buf, sizeof(buf), "Wrote SensorReport: %u bytes", (unsigned)bytes.size());
    Log(buf);

    // Read it back into a new object
    SensorReport in;
    ms.read(ss, in);
    bool ok = ss.good() && ms.getLastError() == serialize::ParsingError::NONE &&
        in.id == out.id && in.name == out.name &&
        in.position.latitude == out.position.latitude &&
        in.position.longitude == out.position.longitude &&
        in.samples == out.samples && in.flags == out.flags && in.counters == out.counters;
    snprintf(buf, sizeof(buf), "Read back: id %u, name \"%s\", starts %ld: %s",
        (unsigned)in.id, in.name.c_str(), (long)in.counters["starts"], ok ? "match" : "MISMATCH");
    Log(buf);

    // A message cut short in transit is reported as an error, not misread
    std::stringstream cut(bytes.substr(0, bytes.size() / 2),
        std::ios::in | std::ios::out | std::ios::binary);
    SensorReport partial;
    ms.clearLastError();
    ms.read(cut, partial);
    bool cutDetected = ms.getLastError() != serialize::ParsingError::NONE;
    Log(cutDetected ? "Truncated message: error reported" : "Truncated message: NOT detected");

    return ok && cutDetected;
}

#ifdef ARDUINO
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Log("MessageSerialize demo");
    Log(Demo() ? "MessageSerialize demo PASSED" : "MessageSerialize demo FAILED");
}

void loop()
{
}
#else
int main()
{
    Log("MessageSerialize demo");
    bool ok = Demo();
    Log(ok ? "MessageSerialize demo PASSED" : "MessageSerialize demo FAILED");
    return ok ? 0 : 1;
}
#endif
