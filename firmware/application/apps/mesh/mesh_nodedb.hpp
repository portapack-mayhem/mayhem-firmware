/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __MESH_NODEDB_H__
#define __MESH_NODEDB_H__

#include "mesh_protocol.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace meshtastic {

// The whole node table lives in the app object, which sits in a 43 kB heap shared with
// everything else the app allocates. Each entry costs 216 bytes, and the number is
// chosen against the memory left rather than against ambition.
//
// Twelve became ten for a measured reason. The map view needs 3736 bytes in one piece;
// after a few screens have been opened and closed the heap holds around four kilobytes
// but in two dozen fragments, so the request falls through to the unallocated core -
// which stood at 3472. Two entries are 432 bytes, and they turn a map that refuses to
// open into one that does. A handheld that can name ten neighbours and show them on a
// map beats one that can name twelve and cannot.
static constexpr size_t MAX_NODES = 10;

struct NodeEntry {
    uint32_t node_id{0};
    char long_name[41]{};
    char short_name[5]{};
    PositionData position{};
    int8_t last_rssi{0};
    float last_snr{0.0f};
    uint32_t last_seen_uptime{0};  // system uptime ticks at last packet
    uint8_t hop_count{0};
    // What this node last told the mesh about its own neighbours (portnum 71): how
    // many it hears directly, and - when we are one of them - the signal it hears US
    // at. That last figure is the half of the link our own receiver cannot measure.
    bool unmessagable{false};  // the node says nobody reads messages on it
    uint8_t nbr_count{0};
    bool nbr_hears_us{false};
    float nbr_snr_to_us{0.0f};
    bool active{false};
    bool has_position{false};
    float battery_level{-1.0f};  // -1 = unknown
    float voltage{-1.0f};        // volts, -1 = unknown
    float temperature{-999.0f};  // C, -999 = unknown
    float humidity{-1.0f};       // %, -1 = unknown
    float pressure{-1.0f};       // hPa, -1 = unknown
    uint8_t hw_model{0};         // which board the node runs (mesh.proto HardwareModel)
    uint8_t role{0};             // and in what role it advertises itself
    uint8_t public_key[32]{};    // Curve25519 pubkey (NodeInfo field 8, for PKC DMs)
    bool has_pubkey{false};
    // A direct-message thread exists with this node (we messaged it, or it messaged
    // us). Threads are listed next to the channels so a private conversation can be
    // reopened without hunting through the node list.
    bool dm_thread{false};

    // Colour the chat marks this node's messages with, chosen in its detail page.
    // 0 = none, 1..7 index a small fixed palette.
    uint8_t colour{0};

    // When we first heard this node (uptime ticks), shown in its detail page.
    uint32_t first_seen_uptime{0};
    // What the node has actually told us about itself, so the list can show an icon
    // per capability instead of columns that are blank for most nodes.
    bool has_telemetry{false};  // any device metrics (battery / voltage)
    bool has_env_data{false};   // temperature / humidity / pressure
    bool has_air_data{false};   // particulate matter / CO2
    bool has_stats{false};      // LocalStats from a "request stats" reply

    // DeviceMetrics extras
    float channel_utilization{-1.0f};
    float air_util_tx{-1.0f};
    uint32_t uptime_seconds{0};
    // EnvironmentMetrics extras
    float gas_resistance{-1.0f};
    float lux{-1.0f};
    uint32_t iaq{0};
    // AirQualityMetrics
    uint16_t pm25{0}, pm10{0}, co2{0};
};

// LocalStats is a snapshot a node sends when asked, and only the most recent answer is
// of any interest - keeping eleven counters for every node in the table is not worth
// the RAM.
struct LocalStats {
    uint32_t node_id{0};  // who answered (0 = nothing asked yet)
    uint32_t uptime{0};
    uint32_t packets_tx{0}, packets_rx{0}, packets_rx_bad{0};
    uint32_t nodes_online{0}, nodes_total{0};
    uint32_t tx_relay{0}, rx_dupe{0};
    uint32_t heap_free{0}, heap_total{0};
};

class NodeDB {
   public:
    NodeDB() { clear(); }

    void clear();

    // Update or insert a node. Returns pointer to the entry.
    NodeEntry* update_node(uint32_t node_id,
                           const char* long_name = nullptr,
                           const char* short_name = nullptr);

    NodeEntry* find(uint32_t node_id);
    const NodeEntry* find(uint32_t node_id) const;

    // Update position for a node (creates entry if not present).
    void update_position(uint32_t node_id, const PositionData& pos);

    // Store a node's telemetry (device metrics: battery/voltage; environment
    // metrics: temp/humidity/pressure) - whichever the packet carried.
    void update_telemetry(uint32_t node_id, const TelemetryData& tel);

    // Update signal info from a received packet.
    void update_signal(uint32_t node_id, int8_t rssi, float snr, uint8_t hops, uint32_t uptime);

    // Store a node's Curve25519 public key (NodeInfo field 8), enabling PKC DMs.
    void set_pubkey(uint32_t node_id, const uint8_t pub[32]);

    // What the node said it is: board model and role, both from its NodeInfo.
    void set_identity(uint32_t node_id, uint8_t hw_model, uint8_t role);

    // Remember that a direct-message thread exists with this node (creates the
    // entry if we have never heard from it).
    void mark_dm(uint32_t node_id);

    // Most recent "Request local stats" reply, whoever sent it.
    const LocalStats& last_stats() const { return last_stats_; }

    // A short journal of answered traceroutes. Six entries shared across the whole
    // mesh rather than a history per node: a route is worth keeping mainly to see it
    // change, and six covers that at 96 bytes, where a per-node history would cost
    // more than the map reservation this app already fights for. Hops are kept as
    // their low sixteen bits, which is all any screen here shows of a node number.
    struct RouteLog {
        uint32_t node_id{0};   // who was traced
        uint32_t when{0};      // uptime ticks when the answer came back
        uint16_t hops[3]{};    // up to three intermediate nodes
        uint8_t hop_count{0};  // how many there really were, three or not
    };
    static constexpr size_t MAX_ROUTES = 6;
    void add_route(uint32_t node_id, uint32_t when, const uint16_t* hops, uint8_t count);
    const RouteLog& route(size_t i) const { return routes_[i % MAX_ROUTES]; }
    size_t route_count() const { return route_count_; }

    // The last telemetry packet in full, and who sent it. Every field in
    // telemetry.proto is kept here rather than per node: one record costs half a
    // kilobyte, sixteen of them would not fit in the memory this app has.
    const TelemetryData& last_telemetry() const { return last_telemetry_; }
    uint32_t last_telemetry_node() const { return last_telemetry_node_; }

    size_t count() const;
    NodeEntry* at(size_t idx);
    const NodeEntry* at(size_t idx) const;

    // Iterate over active entries.
    size_t active_count() const;

   private:
    std::array<NodeEntry, MAX_NODES> nodes_{};
    LocalStats last_stats_{};
    // Newest first, so a page can print them in the order they happened without
    // walking backwards through a ring.
    std::array<RouteLog, MAX_ROUTES> routes_{};
    size_t route_count_{0};
    TelemetryData last_telemetry_{};
    uint32_t last_telemetry_node_{0};
    size_t count_{0};

    NodeEntry* insert(uint32_t node_id);
};

}  // namespace meshtastic

#endif /* __MESH_NODEDB_H__ */
