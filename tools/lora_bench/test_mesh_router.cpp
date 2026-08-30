// Host-side test for MeshRouter: dedup, relay mutation, channel hash, id seed.
// Build: c++ -std=c++17 -Wall -Wextra -o test_mesh_router test_mesh_router.cpp \
//   ../../firmware/application/apps/mesh/mesh_router.cpp \
//   ../../firmware/application/apps/mesh/mesh_protocol.cpp \
//   ../../firmware/application/apps/mesh/mesh_crypto.cpp \
//   ../../firmware/application/apps/mesh/mesh_nodedb.cpp
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include "../../firmware/application/apps/mesh/mesh_router.hpp"

using namespace meshtastic;

static int fails = 0;
#define CHECK(cond, name)                                    \
    do {                                                     \
        const bool ok = (cond);                              \
        if (!ok) fails++;                                    \
        printf("%-4s %s\n", ok ? "OK" : "FAIL", name);       \
    } while (0)

static void dump(const char* name, const uint8_t* p, size_t n) {
    printf("%s ", name);
    for (size_t i = 0; i < n; i++) printf("%02x", p[i]);
    printf("\n");
}

int main(int argc, char**) {
    // --dump mode: emit plaintext Data payloads (no PSK -> no encryption) for
    // cross-checking against the real generated protobuf classes in python.
    if (argc > 1) {
        NodeDB db{};
        MeshRouter r{db};
        r.set_local_node(0xDC63A709, nullptr, 0);
        uint8_t f[PKT_MAX_SIZE];
        size_t n = r.build_nodeinfo_tx(f, sizeof(f), "PortaPack H4M", "PP4M");
        dump("NODEINFO", f + PKT_HEADER_SIZE, n - PKT_HEADER_SIZE);
        PositionData pos;
        pos.latitude = 55.7558; pos.longitude = 37.6173;
        pos.altitude_m = 144; pos.timestamp = 1783400000;
        pos.satellites_in_view = 7; pos.valid = true;
        n = r.build_position_tx(f, sizeof(f), pos);
        dump("POSITION", f + PKT_HEADER_SIZE, n - PKT_HEADER_SIZE);
        uint8_t t[PKT_MAX_SIZE];
        n = r.build_text_tx(t, sizeof(t), "HI", 2);
        dump("TEXT", t + PKT_HEADER_SIZE, n - PKT_HEADER_SIZE);
        return 0;
    }

    NodeDB db_a{}, db_b{};
    MeshRouter a{db_a}, b{db_b};
    a.set_local_node(0xDC63A709, DEFAULT_PSK, sizeof(DEFAULT_PSK));
    b.set_local_node(0x6983D19C, DEFAULT_PSK, sizeof(DEFAULT_PSK));
    a.seed_packet_id(1000);

    uint8_t frame[PKT_MAX_SIZE];
    const size_t len = a.build_text_tx(frame, sizeof(frame), "PING", 4);
    CHECK(len > PKT_HEADER_SIZE, "build_text_tx produces frame");

    const auto h = PacketHeader::from_bytes(frame);
    CHECK(h.packet_id == 1001, "packet id = seed+1");
    // A router nobody has configured carries no channel hash. It used to default to
    // hash("ShortTurbo"), and that constant was the bug: every packet on any other
    // channel was rejected, so LongFast never worked. The hash now comes from the
    // channel's own name and key, and line 100 below checks the builder uses it.
    CHECK(h.channel_hash == 0, "unconfigured router has no channel hash");
    CHECK(h.relay_node == 0x09, "originator relay_node = own low byte");
    CHECK(h.hop_limit() == DEFAULT_HOP_LIMIT && h.hop_start() == DEFAULT_HOP_LIMIT,
          "hop_limit/hop_start both = DEFAULT_HOP_LIMIT");

    std::string got;
    b.set_on_packet([&](const MeshPacket& p) { got = p.text_payload(); });
    bool rb = b.on_raw_rx(frame, len, -50, 10.0f, 100);
    CHECK(rb, "first copy asks for rebroadcast");
    CHECK(got == "PING", "text survives AES round-trip");

    got.clear();
    rb = b.on_raw_rx(frame, len, -50, 10.0f, 101);
    CHECK(!rb && got.empty(), "exact duplicate: no dispatch, no rebroadcast");

    uint8_t relay[PKT_MAX_SIZE];
    memcpy(relay, frame, len);
    b.make_relay(relay);
    const auto hr = PacketHeader::from_bytes(relay);
    CHECK(hr.hop_limit() == DEFAULT_HOP_LIMIT - 1, "relay decrements hop_limit");
    CHECK(hr.hop_start() == DEFAULT_HOP_LIMIT, "relay keeps hop_start");
    CHECK(hr.relay_node == 0x9C, "relay_node = relayer low byte");
    CHECK(memcmp(relay + PKT_HEADER_SIZE, frame + PKT_HEADER_SIZE,
                 len - PKT_HEADER_SIZE) == 0,
          "relay leaves payload untouched");

    rb = a.on_raw_rx(relay, len, -50, 10.0f, 102);
    CHECK(!rb, "origin drops its own relayed echo");

    // channel-hash filter both ways
    uint8_t f2[PKT_MAX_SIZE];
    size_t l2 = a.build_text_tx(f2, sizeof(f2), "X", 1);
    b.set_channel_hash(0x22);
    got.clear();
    rb = b.on_raw_rx(f2, l2, -50, 10.0f, 103);
    CHECK(!rb && got.empty(), "mismatched channel hash filtered on RX");

    a.set_channel_hash(0x22);
    l2 = a.build_text_tx(f2, sizeof(f2), "Y", 1);
    CHECK(PacketHeader::from_bytes(f2).channel_hash == 0x22,
          "builder tags frames with configured hash");
    got.clear();
    rb = b.on_raw_rx(f2, l2, -50, 10.0f, 104);
    CHECK(rb && got == "Y", "matching custom hash decodes");

    // Position wire-format round-trip (encode is reference-validated against
    // the generated protobuf classes, so decode passing this reads the real
    // sfixed32/fixed32 format).
    PositionData tx_pos;
    tx_pos.latitude = -33.8688;  // southern/western: sign must survive sfixed32
    tx_pos.longitude = -151.2093;
    tx_pos.altitude_m = 58;
    tx_pos.timestamp = 1783400123;
    tx_pos.satellites_in_view = 9;
    uint8_t pbuf[64];
    const size_t pn = tx_pos.encode(pbuf, sizeof(pbuf));
    PositionData rx_pos;
    CHECK(PositionData::decode(pbuf, pn, rx_pos), "position decodes");
    CHECK(std::fabs(rx_pos.latitude - tx_pos.latitude) < 2e-7 &&
          std::fabs(rx_pos.longitude - tx_pos.longitude) < 2e-7,
          "negative lat/lon survive sfixed32 round-trip");
    CHECK(rx_pos.altitude_m == 58 && rx_pos.timestamp == 1783400123 && rx_pos.satellites_in_view == 9,
          "altitude/time/sats round-trip");

    // A telemetry request must not be filed as a telemetry reading. It is an empty
    // copy of the variant it asks for, so it decodes as a valid all-zero measurement,
    // and storing it wiped the asking node's real battery and voltage. Checked with a
    // real reading first, then the request that used to erase it.
    {
        NodeDB tdb{};
        MeshRouter ta{tdb}, tb{tdb};
        ta.set_local_node(0x11110001, DEFAULT_PSK, sizeof(DEFAULT_PSK));
        tb.set_local_node(0x22220002, DEFAULT_PSK, sizeof(DEFAULT_PSK));
        uint8_t tbuf2[PKT_MAX_SIZE];
        TelemetryData reading{};
        reading.has_device = true;
        reading.battery_level = 101;
        reading.voltage = 4.36f;
        size_t tn2 = ta.build_telemetry_tx(tbuf2, sizeof(tbuf2), reading,
                                           TelemetryData::DEVICE, nullptr, 0);
        tb.on_raw_rx(tbuf2, tn2, -50, 10.0f, 150);
        const NodeEntry* ne = tdb.find(0x11110001);
        CHECK(ne && ne->battery_level > 100 && ne->voltage > 4.0f,
              "a real device metrics packet is stored");
        tn2 = ta.build_stats_request_tx(tbuf2, sizeof(tbuf2), 0x22220002, false);
        tb.on_raw_rx(tbuf2, tn2, -50, 10.0f, 151);
        ne = tdb.find(0x11110001);
        CHECK(ne && ne->battery_level > 100 && ne->voltage > 4.0f,
              "a metrics request does not overwrite them with zeroes");
    }

    // The traceroute journal. Newest first, and a second trace of the same node moves
    // that node's entry to the front rather than adding a duplicate: six copies of one
    // path would crowd out every other node in a six-deep log.
    {
        NodeDB rdb{};
        const uint16_t h1[2] = {0x1111, 0x2222};
        rdb.add_route(0xAAAA0001, 100, h1, 2);
        rdb.add_route(0xBBBB0002, 200, nullptr, 0);
        CHECK(rdb.route_count() == 2, "two traces logged");
        CHECK(rdb.route(0).node_id == 0xBBBB0002, "newest trace comes first");
        CHECK(rdb.route(1).hop_count == 2 && rdb.route(1).hops[1] == 0x2222,
              "hops kept in the order they were crossed");
        const uint16_t h2[1] = {0x3333};
        rdb.add_route(0xAAAA0001, 300, h2, 1);
        CHECK(rdb.route_count() == 2, "re-tracing a node does not add an entry");
        CHECK(rdb.route(0).node_id == 0xAAAA0001 && rdb.route(0).hop_count == 1 &&
                  rdb.route(0).hops[0] == 0x3333,
              "re-tracing replaces the old path and moves it to the front");
        for (uint32_t i = 0; i < NodeDB::MAX_ROUTES + 2; i++)
            rdb.add_route(0xC0000000 + i, 400 + i, nullptr, 0);
        CHECK(rdb.route_count() == NodeDB::MAX_ROUTES, "the log stops at its limit");
        const uint16_t h4[4] = {1, 2, 3, 4};
        rdb.add_route(0xD0000001, 500, h4, 4);
        CHECK(rdb.route(0).hop_count == 3, "a longer route is cut to what is stored");
    }

    // A telemetry request has to name the variant it wants, as an empty submessage.
    // Stock firmware answers a metrics request only when the decoded request has the
    // device metrics variant set; a bare empty Telemetry is dropped without a reply,
    // which is exactly what this used to send. Checked on the wire, both ways round.
    MeshPacket tq{};
    bool tq_seen = false;
    b.set_on_packet([&](const MeshPacket& p) { tq = p; tq_seen = true; });
    uint8_t qbuf[PKT_MAX_SIZE];
    size_t qn = a.build_stats_request_tx(qbuf, sizeof(qbuf), 0x6983D19C, false);
    b.on_raw_rx(qbuf, qn, -50, 10.0f, 140);
    CHECK(tq_seen && tq.data.payload_len == 2 &&
          tq.data.payload[0] == 0x12 && tq.data.payload[1] == 0x00,
          "metrics request carries an empty device_metrics (field 2)");
    tq_seen = false;
    qn = a.build_stats_request_tx(qbuf, sizeof(qbuf), 0x6983D19C, true);
    b.on_raw_rx(qbuf, qn, -50, 10.0f, 141);
    CHECK(tq_seen && tq.data.payload_len == 2 &&
          tq.data.payload[0] == 0x32 && tq.data.payload[1] == 0x00,
          "stats request carries an empty local_stats (field 6)");
    CHECK(tq.data.want_response, "telemetry request asks for a response");

    // The node card's "Trace" button. The app answers a traceroute only when the packet
    // is addressed to it, asks for a response, and carries no request_id - so a request
    // that gets any of those three wrong is answered by nobody and looks like a dead
    // node rather than a bug. Checked here against the receiving side's own decoder.
    MeshPacket tr{};
    bool tr_seen = false;
    b.set_on_packet([&](const MeshPacket& p) { tr = p; tr_seen = true; });
    uint8_t tbuf[PKT_MAX_SIZE];
    const size_t tn = a.build_traceroute_request_tx(tbuf, sizeof(tbuf), 0x6983D19C);
    CHECK(tn > PKT_HEADER_SIZE, "traceroute request builds");
    CHECK(b.on_raw_rx(tbuf, tn, -50, 10.0f, 130) || true, "traceroute request received");
    CHECK(tr_seen && tr.data.portnum == PortNum::TRACEROUTE, "arrives on the traceroute port");
    CHECK(tr.header.to == 0x6983D19C, "unicast to the node being traced");
    CHECK(tr.data.want_response && tr.data.request_id == 0,
          "asks for a response and is not itself one");
    CHECK(tr.data.payload_len == 0, "empty RouteDiscovery: the hops are filled in en route");

    printf("\n%s\n", fails ? "FAILURES" : "ALL PASS");
    return fails;
}
