/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Meshtastic wire protocol structures.
 * Reference: https://meshtastic.org/docs/overview/mesh-algo
 * and meshtastic/firmware source (meshtastic/mesh/generated/meshtastic/*.pb.h)
 */

#ifndef __MESH_PROTOCOL_H__
#define __MESH_PROTOCOL_H__

#include <cstdint>
#include <cstring>
#include <array>
#include <string>

namespace meshtastic {

// ---- LoRa PHY header (16 bytes, always unencrypted) ----------------------
//
// [0-3]   to        (destination node ID, LE uint32; 0xFFFFFFFF = broadcast)
// [4-7]   from      (sender node ID, LE uint32)
// [8-11]  packet_id (LE uint32)
// [12]    flags     (hop_limit:3 | want_ack:1 | via_mqtt:1 | hop_start:3)
// [13]    channel   (hash of channel name; 0x08 = LongFast default)
// [14]    next_hop  (low byte of the intended next hop, 0 = any)
// [15]    relay_node(low byte of the node that relayed this frame)
// [16+]   payload   (encrypted MeshPacket Data protobuf)

static constexpr size_t PKT_HEADER_SIZE = 16;  // dest4+src4+id4+flags1+chanHash1+nextHop1+relay1
static constexpr size_t PKT_MAX_SIZE = 255;
static constexpr uint32_t BROADCAST_ADDR = 0xFFFFFFFF;
// channel_hash = xorHash(channelName) ^ xorHash(psk).  Default channel name is the
// modem-preset name -> ShortTurbo = 0x0E (measured on-air + decrypt-verified), LongFast = 0x08.
static constexpr uint8_t DEFAULT_CHANNEL_HASH = 0x0E;  // hash("ShortTurbo")

struct PacketHeader {
    uint32_t to;
    uint32_t from;
    uint32_t packet_id;
    uint8_t flags;  // hop_limit[2:0] | want_ack[3] | via_mqtt[4] | hop_start[7:5]
    uint8_t channel_hash;
    uint8_t next_hop{0};  // (current Meshtastic 16-byte header)
    uint8_t relay_node{0};

    uint8_t hop_limit() const { return flags & 0x07; }
    bool want_ack() const { return (flags >> 3) & 0x01; }
    bool via_mqtt() const { return (flags >> 4) & 0x01; }
    uint8_t hop_start() const { return (flags >> 5) & 0x07; }

    // Real Meshtastic LoRa header (16 B): dest(4 LE) src(4 LE) id(4 LE) flags(1)
    // channel_hash(1) next_hop(1) relay_node(1).  (Older code had flags/channel_hash
    // swapped and omitted next_hop/relay -> 14 B, which mis-aligned the payload by 2.)
    static PacketHeader from_bytes(const uint8_t* p) {
        PacketHeader h;
        h.to = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
        h.from = p[4] | (p[5] << 8) | (p[6] << 16) | (p[7] << 24);
        h.packet_id = p[8] | (p[9] << 8) | (p[10] << 16) | (p[11] << 24);
        h.flags = p[12];
        h.channel_hash = p[13];
        h.next_hop = p[14];
        h.relay_node = p[15];
        return h;
    }

    void to_bytes(uint8_t* p) const {
        p[0] = to;
        p[1] = to >> 8;
        p[2] = to >> 16;
        p[3] = to >> 24;
        p[4] = from;
        p[5] = from >> 8;
        p[6] = from >> 16;
        p[7] = from >> 24;
        p[8] = packet_id;
        p[9] = packet_id >> 8;
        p[10] = packet_id >> 16;
        p[11] = packet_id >> 24;
        p[12] = flags;
        p[13] = channel_hash;
        p[14] = next_hop;
        p[15] = relay_node;
    }
};

// ---- Portnum (application-layer port, Meshtastic protobuf PortNum) -------
enum class PortNum : uint8_t {
    UNKNOWN = 0,
    TEXT_MESSAGE = 1,
    REMOTE_HARDWARE = 2,
    POSITION = 3,
    NODEINFO = 4,
    ROUTING = 5,
    ADMIN = 6,          // real Meshtastic ADMIN_APP (was 67 - clashed with TELEMETRY)
    TELEMETRY = 67,     // TELEMETRY_APP: device + environment metrics
    TRACEROUTE = 70,    // TRACEROUTE_APP: RouteDiscovery, the path a packet took
    NEIGHBORINFO = 71,  // NEIGHBORINFO_APP: who a node hears directly, and how well
    SIMULATOR = 255,
};

// ---- NeighborInfo (portnum 71) --------------------------------------------
// A node's list of DIRECT neighbours - the ones whose packets reach it without a
// single hop - and the signal it hears each of them at. Together they draw the mesh's
// real topology, which is the one thing signal strength alone cannot tell you: a link
// is often good in one direction and poor in the other.
//   NeighborInfo { node_id=1, last_sent_by_id=2, node_broadcast_interval_secs=3,
//                  repeated Neighbor neighbors=4 }
//   Neighbor     { node_id=1, snr=2 (float) }
// last_rx_time and the per-neighbour interval are deliberately NOT put on the air -
// see collectNeighborInfo() in Meshtastic's NeighborInfoModule.
struct NeighborInfoData {
    uint32_t node_id{0};          // who is reporting
    uint32_t last_sent_by_id{0};  // who put this copy on the air
    uint32_t interval_secs{0};    // how often the reporter broadcasts this
    uint8_t count{0};             // how many neighbours it listed
    // The reporter's own view of US, when we appear in its list. This is the asymmetric
    // half of the link: what it hears of us, which our own RSSI cannot show.
    bool has_us{false};
    float snr_to_us{0.0f};

    static bool decode(const uint8_t* buf, size_t len, NeighborInfoData& out, uint32_t our_id);
};

// ---- Minimal hand-encoded protobuf helpers --------------------------------
// Only the fields we need, encoded in a compact representation.
// Field layout for Data message (field IDs match Meshtastic .proto):
//   1: portnum (varint)
//   2: payload (bytes)
//   3: want_response (bool)
//   5: source (uint32)

struct DecodedData {
    PortNum portnum{PortNum::UNKNOWN};
    uint8_t payload[PKT_MAX_SIZE - PKT_HEADER_SIZE - 4]{};
    uint8_t payload_len{0};
    bool want_response{false};
    uint32_t source{0};
    // Data.request_id (field 6, fixed32): a ROUTING ACK/NAK echoes the packet_id
    // it acknowledges here.  This is how a sender matches an incoming ACK to the
    // message it sent (delivery status).
    uint32_t request_id{0};
    // Data.bitfield (field 9) bit 0 = ok_to_mqtt: sender opts in to being bridged
    // to MQTT by a gateway.
    bool ok_to_mqtt{false};

    static bool decode(const uint8_t* buf, size_t len, DecodedData& out);
    size_t encode(uint8_t* buf, size_t max_len) const;
};

// ---- Position message (portnum=3) ----------------------------------------
struct PositionData {
    double latitude{0.0};
    double longitude{0.0};
    int32_t altitude_m{0};
    uint32_t timestamp{0};
    uint8_t satellites_in_view{0};
    bool valid{false};

    static bool decode(const uint8_t* buf, size_t len, PositionData& out);
    size_t encode(uint8_t* buf, size_t max_len) const;
};

// ---- NodeInfo message (portnum=4) ----------------------------------------
struct NodeInfoData {
    uint32_t node_id{0};
    char long_name[41]{};      // max 40 chars + null
    char short_name[5]{};      // max 4 chars + null
    uint8_t hw_model{0};       // mesh.proto HardwareModel enum (0 = unset)
    uint8_t public_key[32]{};  // Curve25519 pubkey (field 8), for PKC DMs
    bool has_pubkey{false};
    uint8_t role{0};  // DeviceConfig.Role the node advertises (field 7)
    // "Messaging: unmonitored" (field 9). Meshtastic sets it for the roles nobody sits
    // in front of - router, router late, sensor, tracker, TAK tracker - so other
    // clients stop offering to write to them. Advisory, not enforced.
    bool unmessagable{false};

    static bool decode(const uint8_t* buf, size_t len, NodeInfoData& out);
};

// ---- Routing message (portnum=5) -----------------------------------------
enum class RoutingError : uint8_t {
    NONE = 0,
    NO_ROUTE = 1,
    GOT_NAK = 2,
    TIMEOUT = 3,
    NO_INTERFACE = 4,
    MAX_RETRANSMIT = 5,
};

struct RoutingData {
    uint32_t request_id{0};
    RoutingError error{RoutingError::NONE};
    bool is_ack{false};  // true = ACK, false = NAK
};

// ---- Telemetry message (portnum=67) -------------------------------------
// Meshtastic Telemetry: field1=time(fixed32), field2=DeviceMetrics,
// field3=EnvironmentMetrics.
struct TelemetryData {
    uint32_t timestamp{0};  // unix UTC

    // DeviceMetrics (has_device)
    uint8_t battery_level{0};  // percent 0-100 (101 = plugged in / no battery)
    float voltage{0.0f};       // volts
    uint32_t uptime_seconds{0};
    bool has_device{false};
    // Per-field TX gates (user "Off" in the telemetry config skips just that metric).
    bool send_battery{true};
    bool send_voltage{true};
    bool send_util{false};   // channel_utilization / air_util_tx are opt-in
    bool send_uptime{true};  // ...and uptime can be withheld or invented too
    // Temperature/humidity/pressure are only worth sending when they are a reading or
    // a value the user entered; an extra metric alone must not drag three zeroes along.
    bool send_env_base{true};

    // EnvironmentMetrics (has_env)
    float temperature{0.0f};          // celsius
    float relative_humidity{0.0f};    // percent
    float barometric_pressure{0.0f};  // hPa
    bool has_env{false};

    // DeviceMetrics extras a peer may report.
    float channel_utilization{-1.0f};
    float air_util_tx{-1.0f};

    // EnvironmentMetrics extras (fields 4, 7, 9).
    float gas_resistance{-1.0f};
    uint32_t iaq{0};
    float lux{-1.0f};

    // LocalStats (variant 6) - what "Request local stats" asks a node for.
    uint32_t stat_uptime{0};
    uint32_t packets_tx{0}, packets_rx{0}, packets_rx_bad{0};
    uint32_t nodes_online{0}, nodes_total{0};
    uint32_t rx_dupe{0}, tx_relay{0};
    uint32_t heap_free{0}, heap_total{0};
    bool has_stats{false};

    // AirQualityMetrics (has_air). The proto calls PM1.0 "pm10" and PM10 "pm100";
    // we carry the two the phone app shows plus CO2.
    uint16_t pm25{0};  // ug/m3, pm25_standard
    uint16_t pm10{0};  // ug/m3, pm100_standard
    uint16_t co2{0};   // ppm
    bool has_air{false};

    // The rest of telemetry.proto, decoded so nothing a peer sends is thrown away.
    // Sentinels: -1 for a quantity that cannot be negative, -999 for a temperature.
    // EnvironmentMetrics
    float distance{-1.0f};  // mm, e.g. water level
    float white_lux{-1.0f}, ir_lux{-1.0f}, uv_lux{-1.0f};
    uint32_t wind_direction{400};  // degrees (400 = absent)
    float wind_speed{-1.0f}, wind_gust{-1.0f}, wind_lull{-1.0f};
    float weight{-1.0f};     // kg
    float radiation{-1.0f};  // uR/h
    float rainfall_1h{-1.0f}, rainfall_24h{-1.0f};
    uint32_t soil_moisture{101};  // percent (101 = absent)
    float soil_temperature{-999.0f};
    float env_voltage{-1.0f}, env_current{-1.0f};
    float one_wire_temperature{-999.0f};

    // PowerMetrics: eight voltage/current channels (solar, battery, load).
    float ch_voltage[8]{-1, -1, -1, -1, -1, -1, -1, -1};
    float ch_current[8]{-1, -1, -1, -1, -1, -1, -1, -1};
    bool has_power{false};

    // AirQualityMetrics in full: both scales, particle counts and the sensor extras.
    uint16_t pm1_std{0}, pm10_env{0}, pm25_env{0}, pm100_env{0}, pm40_std{0};
    uint32_t particles_03{0}, particles_05{0}, particles_10{0}, particles_25{0},
        particles_40{0}, particles_50{0}, particles_100{0};
    float particles_tps{-1.0f};
    float co2_temperature{-999.0f}, co2_humidity{-1.0f};
    float formaldehyde{-1.0f}, form_temperature{-999.0f}, form_humidity{-1.0f};
    float pm_temperature{-999.0f}, pm_humidity{-1.0f};
    float pm_voc_idx{-1.0f}, pm_nox_idx{-1.0f};

    // LocalStats, remaining counters.
    uint32_t tx_relay_canceled{0}, tx_dropped{0};
    int32_t noise_floor{0};
    bool has_noise{false};

    // HealthMetrics (wearables).
    uint32_t heart_bpm{0}, spo2{0};
    float body_temperature{-999.0f};
    bool has_health{false};

    // HostMetrics - only a Linux node (meshtasticd) reports these.
    uint32_t host_uptime{0}, freemem{0}, diskfree1{0}, diskfree2{0}, diskfree3{0};
    uint32_t load1{0}, load5{0}, load15{0};
    bool has_host{false};

    // TrafficManagementStats - what a router's filters dropped.
    uint32_t packets_inspected{0}, position_dedup_drops{0}, nodeinfo_cache_hits{0},
        rate_limit_drops{0}, unknown_packet_drops{0}, hop_exhausted{0},
        router_hops_preserved{0};
    bool has_traffic{false};

    // Telemetry.variant is a protobuf oneof: exactly ONE metrics type per packet
    // (a receiver keeps only the last one seen).
    enum Variant : uint8_t { DEVICE = 0,
                             ENVIRONMENT = 1,
                             AIR_QUALITY = 2,
                             POWER = 3,
                             HEALTH = 4,
                             LOCAL_STATS = 5 };

    // A metric the device has no sensor for, filled in by the user and appended to the
    // variant it belongs to. One table entry beats a widget per field: telemetry.proto
    // has over sixty of them.
    struct ExtraMetric {
        uint8_t variant;  // Variant this field belongs to
        uint8_t field;    // protobuf field number inside that sub-message
        bool is_float;    // float (wire type 5) or varint
        float value;
    };

    size_t encode(uint8_t* buf, size_t max_len, Variant variant, const ExtraMetric* extras = nullptr, size_t extra_count = 0) const;

    // Parse a received Telemetry payload (fills whichever metrics are present).
    static bool decode(const uint8_t* buf, size_t len, TelemetryData& out);
};

// ---- Full decoded Meshtastic packet --------------------------------------
struct MeshPacket {
    PacketHeader header{};
    DecodedData data{};
    int8_t rx_rssi{0};
    float rx_snr{0.0f};
    bool encrypted{false};
    bool decoded{false};
    uint8_t channel_index{0};  // which of our configured channels it arrived on

    std::string text_payload() const {
        if (data.portnum == PortNum::TEXT_MESSAGE) {
            return std::string(reinterpret_cast<const char*>(data.payload), data.payload_len);
        }
        return {};
    }
};

// ---- Protobuf fixed-width helpers -----------------------------------------
// Little-endian 32-bit fields (protobuf wire type 5): fixed32/sfixed32/float.
inline size_t encode_fixed32(uint8_t* buf, uint32_t v) {
    buf[0] = v;
    buf[1] = v >> 8;
    buf[2] = v >> 16;
    buf[3] = v >> 24;
    return 4;
}
inline size_t encode_float(uint8_t* buf, float f) {
    uint32_t bits;
    __builtin_memcpy(&bits, &f, sizeof(bits));
    return encode_fixed32(buf, bits);
}

// ---- Protobuf varint helpers (minimal) -----------------------------------
inline size_t encode_varint(uint8_t* buf, uint64_t value) {
    size_t i = 0;
    while (value > 0x7F) {
        buf[i++] = static_cast<uint8_t>((value & 0x7F) | 0x80);
        value >>= 7;
    }
    buf[i++] = static_cast<uint8_t>(value);
    return i;
}

// Build the standard Meshtastic channel-sharing URL:
//   https://meshtastic.org/e/#<base64url(ChannelSet)>
// ChannelSet (apponly.proto) = the channel's settings plus the modem preset and region,
// which is exactly what the phone app encodes in its QR code. Scanning it joins the
// channel with the real key, so no one has to read a hex string off a screen.
// psk_len 1 means "well known default key" (the value 1 = the public channel key);
// 16 or 32 bytes is a real key.
//
// One structure rather than a row of arguments, because getting any of these wrong is
// silent: the far end joins a channel it cannot hear on.
struct ChannelShare {
    const char* name{nullptr};
    const uint8_t* psk{nullptr};
    size_t psk_len{0};
    uint8_t modem_preset{0};
    uint8_t region_code{0};
    // The preset's own parameters, written out explicitly. use_preset is set, so a node
    // ignores them - but it still STORES them, and leaving them out stored zeros: the
    // phone app announced "Bandwidth 250 -> 0, Spread Factor 11 -> 0" on every scan.
    uint8_t sf{0};
    uint32_t bw_hz{0};
    uint8_t cr{0};
    // LoRaConfig.channel_num, 1-based (0 = let the far end work it out). This is what
    // pins the frequency. Without it a node derives the slot from the name of ITS
    // primary channel - and a shared channel becomes the primary when it is imported,
    // so the far end retunes to a different frequency and hears nothing at all.
    uint32_t channel_num{0};
    // When sharing a SECONDARY channel, the primary has to travel with it and go first:
    // ChannelSet entries land on channel indices 0, 1, 2... in order. Send only the
    // secondary and it is installed as the primary, which is the same retune again.
    const char* primary_name{nullptr};
    const uint8_t* primary_psk{nullptr};
    size_t primary_psk_len{0};
};
std::string channel_share_url(const ChannelShare& s);

// Build the standard Meshtastic contact-sharing URL for one node:
//   https://meshtastic.org/v/#<base64url(SharedContact)>
// Scanning it adds the node to a phone's contacts together with its public key, which
// is what makes an encrypted direct message to it possible without typing anything.
// Standard base64 (with padding), how every Meshtastic tool prints a public key.
std::string base64_encode(const uint8_t* data, size_t len);

std::string contact_share_url(uint32_t node_id, const char* long_name, const char* short_name, const uint8_t* pubkey, bool has_pubkey);

inline size_t decode_varint(const uint8_t* buf, size_t len, uint64_t& out) {
    out = 0;
    for (size_t i = 0; i < len && i < 10; i++) {
        out |= (static_cast<uint64_t>(buf[i] & 0x7F)) << (7 * i);
        if (!(buf[i] & 0x80)) return i + 1;
    }
    return 0;
}

}  // namespace meshtastic

#endif /* __MESH_PROTOCOL_H__ */
