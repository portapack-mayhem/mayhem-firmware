// Host test for the Meshtastic Telemetry codec (device + environment metrics).
// Telemetry.variant is a protobuf oneof → one metrics type per packet, so we
// encode/decode each variant separately and check the round-trip.
//
// Build:
//   c++ -std=c++17 -w -I../../firmware/application/apps/mesh \
//       -o test_mesh_telemetry test_mesh_telemetry.cpp \
//       ../../firmware/application/apps/mesh/mesh_protocol.cpp
// Cross-check the wire bytes against the real protobuf classes:
//   python3 -c "from meshtastic.protobuf import telemetry_pb2; ..."  (see repo notes)
#include <cstdio>
#include <cmath>
#include "mesh_protocol.hpp"

using namespace meshtastic;

static int fails = 0;
#define CHECK(cond, name)                                 \
    do {                                                  \
        const bool ok = (cond);                           \
        if (!ok) fails++;                                 \
        printf("%-4s %s\n", ok ? "OK" : "FAIL", name);    \
    } while (0)

int main() {
    TelemetryData t;
    t.timestamp = 1783600000;
    t.battery_level = 87;  t.voltage = 4.05f;  t.uptime_seconds = 1234;  t.has_device = true;
    t.temperature = 23.5f; t.relative_humidity = 45.0f; t.barometric_pressure = 1013.2f; t.has_env = true;

    uint8_t dev[128], env[128];
    const size_t dn = t.encode(dev, sizeof(dev), TelemetryData::DEVICE);
    const size_t en = t.encode(env, sizeof(env), TelemetryData::ENVIRONMENT);

    TelemetryData rd, re;
    CHECK(TelemetryData::decode(dev, dn, rd) && rd.has_device && !rd.has_env,
          "device packet decodes as device-only");
    // The decoder extracts what the app displays (battery, voltage) + time;
    // uptime_seconds is encode-only (peers don't need our uptime), so it is
    // intentionally not read back here.
    CHECK(rd.timestamp == 1783600000 && rd.battery_level == 87 &&
          std::fabs(rd.voltage - 4.05f) < 0.01f,
          "device metrics round-trip (battery/voltage/time)");

    CHECK(TelemetryData::decode(env, en, re) && re.has_env && !re.has_device,
          "environment packet decodes as env-only");
    CHECK(std::fabs(re.temperature - 23.5f) < 0.01f &&
          std::fabs(re.relative_humidity - 45.0f) < 0.01f &&
          std::fabs(re.barometric_pressure - 1013.2f) < 0.1f,
          "environment metrics round-trip");

    // A real node's DeviceMetrics carries extra fields (channel_util=3,
    // air_util_tx=4) our encoder omits; the decoder must skip them and still
    // pull battery. Bytes captured from a Heltec V4 on-air.
    const uint8_t heltec[] = {
        0x0d,0xc0,0xa5,0x4e,0x6a,0x12,0x14,0x08,0x65,0x15,0x96,0x43,0x8b,0x40,
        0x1d,0x00,0x00,0x00,0x00,0x25,0xa5,0x4f,0x7a,0x3c,0x28,0xfa,0x1f};
    TelemetryData rh;
    CHECK(TelemetryData::decode(heltec, sizeof(heltec), rh) && rh.has_device &&
          rh.battery_level == 101 && std::fabs(rh.voltage - 4.352f) < 0.01f,
          "real Heltec device metrics (skips unknown fields)");

    printf("\n%s\n", fails ? "FAILURES" : "ALL PASS");
    return fails;
}
