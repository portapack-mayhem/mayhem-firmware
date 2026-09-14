// Host test for partial-packet decode (peer NAME recovery).
//
// A NodeInfo packet is ~110-123 LoRa symbols — longer than the M4 RX ring, so the
// baseband emits a TRUNCATED payload (the tail is cut). AES-CTR is a stream cipher
// and long_name/short_name are early fields, so DecodedData::decode must copy the
// bytes it has (not bail on the short final field) and NodeInfoData::decode must
// still recover the name. This test builds a real NodeInfo Data protobuf, cuts the
// tail, and checks the name survives.
//
// Build:
//   c++ -std=c++17 -w -I../../firmware/application/apps/mesh \
//       -o test_mesh_partial test_mesh_partial.cpp \
//       ../../firmware/application/apps/mesh/mesh_protocol.cpp
#include <cstdio>
#include <cstring>
#include <vector>
#include "mesh_protocol.hpp"

using namespace meshtastic;

static int fails = 0;
#define CHECK(cond, name)                              \
    do {                                               \
        const bool ok = (cond);                        \
        if (!ok) fails++;                              \
        printf("%-4s %s\n", ok ? "OK" : "FAIL", name); \
    } while (0)

// Append a length-delimited protobuf field (wire type 2).
static void put_ld(std::vector<uint8_t>& v, uint8_t field, const char* s, size_t n) {
    v.push_back(static_cast<uint8_t>((field << 3) | 2));
    v.push_back(static_cast<uint8_t>(n));
    for (size_t k = 0; k < n; ++k) v.push_back(static_cast<uint8_t>(s[k]));
}

int main() {
    // ── Build a Meshtastic User protobuf (the NodeInfo payload) ──────────────────
    // field 1 id, field 2 long_name, field 3 short_name, field 4 macaddr, field 5 hw.
    std::vector<uint8_t> user;
    put_ld(user, 1, "!dc63a709", 9);
    put_ld(user, 2, "PortaPack H4M", 13);
    put_ld(user, 3, "PP4M", 4);
    put_ld(user, 4, "\x01\x02\x03\x04\x05\x06", 6);       // macaddr (late field)
    user.push_back((5 << 3) | 0); user.push_back(255);    // hw_model varint (latest)

    // ── Wrap it in a Data protobuf: portnum=NODEINFO(4) + payload=User ───────────
    std::vector<uint8_t> data;
    data.push_back((1 << 3) | 0); data.push_back(4);      // portnum = NODEINFO
    data.push_back((2 << 3) | 2);                         // payload tag
    data.push_back(static_cast<uint8_t>(user.size()));    // payload length (FULL)
    for (uint8_t b : user) data.push_back(b);

    // (1) Full packet: name + short name decode as before.
    {
        DecodedData d;
        CHECK(DecodedData::decode(data.data(), data.size(), d) &&
              d.portnum == PortNum::NODEINFO && d.payload_len == user.size(),
              "full Data decodes, payload complete");
        NodeInfoData ni;
        CHECK(NodeInfoData::decode(d.payload, d.payload_len, ni) &&
              std::strcmp(ni.long_name, "PortaPack H4M") == 0 &&
              std::strcmp(ni.short_name, "PP4M") == 0,
              "full NodeInfo: long+short name");
    }

    // (2) Truncated tail (M4 ring cut): drop macaddr+hw_model. The Data.payload length
    //     prefix still claims the full User size, but only the head is present.
    //     Cut so long_name+short_name are intact but the payload is short of declared.
    {
        const size_t keep = 4 /*Data hdr*/ + 9 + 2 /*id*/ + 13 + 2 /*long*/ + 4 + 2 /*short*/;
        std::vector<uint8_t> cut(data.begin(), data.begin() + keep);
        DecodedData d;
        CHECK(DecodedData::decode(cut.data(), cut.size(), d) &&
              d.portnum == PortNum::NODEINFO && d.payload_len > 0,
              "truncated Data: payload copied (not dropped)");
        NodeInfoData ni;
        CHECK(NodeInfoData::decode(d.payload, d.payload_len, ni) &&
              std::strcmp(ni.long_name, "PortaPack H4M") == 0 &&
              std::strcmp(ni.short_name, "PP4M") == 0,
              "truncated NodeInfo: name still recovered");
    }

    // (3) A normal short packet (POSITION-sized) is never affected: field fits.
    {
        std::vector<uint8_t> shortd;
        shortd.push_back((1 << 3) | 0); shortd.push_back(3);   // portnum POSITION
        put_ld(shortd, 2, "\x0d\x00\x00\x00\x00", 5);          // small payload
        DecodedData d;
        CHECK(DecodedData::decode(shortd.data(), shortd.size(), d) &&
              d.payload_len == 5,
              "normal short packet unchanged");
    }

    printf("\n%s\n", fails ? "FAILURES" : "ALL PASS");
    return fails;
}
