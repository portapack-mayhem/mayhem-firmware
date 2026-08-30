/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Meshtastic flooding mesh router.
 * Implements: deduplication, hop counting, Save & Forward (SD card queue).
 */

#ifndef __MESH_ROUTER_H__
#define __MESH_ROUTER_H__

#include "mesh_protocol.hpp"
#include "mesh_crypto.hpp"
#include "mesh_nodedb.hpp"

#include <array>
#include <functional>
#include <cstdint>

namespace meshtastic {

// Maximum hop limit per Meshtastic spec
static constexpr uint8_t MAX_HOP_LIMIT = 7;
static constexpr uint8_t DEFAULT_HOP_LIMIT = 3;

// Seen-packet deduplication cache (circular buffer of recent packet IDs)
static constexpr size_t SEEN_CACHE_SIZE = 64;

struct SeenEntry {
    uint32_t from{0};
    uint32_t packet_id{0};
    bool valid{false};
};

// Callback for packets that are for us (or broadcast) and fully decoded.
using PacketCallback = std::function<void(const MeshPacket&)>;

// Up to this many parallel channels (index 0 = primary/default, 1.. = custom
// encrypted channels). All share the RF frequency; the 1-byte channel hash +
// per-channel AES key separate them, exactly like real Meshtastic.
static constexpr size_t MAX_CHANNELS = 11;  // primary + 10 custom (see NUM_CUSTOM in ui)

struct ChannelKey {
    uint8_t hash{0};
    uint8_t psk[32]{};
    uint8_t psk_len{0};
    bool enabled{false};
};

class MeshRouter {
   public:
    MeshRouter(NodeDB& db)
        : node_db_(db) {}

    // Set the local node ID and our AES key.
    void set_local_node(uint32_t node_id, const uint8_t* psk, size_t psk_len);

    // Process a raw LoRa PHY payload received from the baseband.
    // Returns true if the packet should be re-broadcast (flooded).
    // Calls on_rx_packet callback for packets delivered to this node.
    bool on_raw_rx(const uint8_t* raw, size_t len, int8_t rssi, float snr, uint32_t uptime_ticks);

    // Build and return a raw LoRa payload for a TX text message.
    // Returns number of bytes written to out_buf (0 on error).
    size_t build_text_tx(uint8_t* out_buf, size_t max_len, const char* text, size_t text_len, uint32_t dest = BROADCAST_ADDR, bool want_ack = false);

    // Build a POSITION packet from our current position.
    size_t build_position_tx(uint8_t* out_buf, size_t max_len, const PositionData& pos);

    // Build a NODEINFO packet advertising this node. With want_response set and a
    // unicast destination it doubles as a request: the peer answers with its own
    // NodeInfo, which is the only way to learn its public key - and a node that
    // publishes one refuses direct messages that are merely channel-encrypted.
    // Meshtastic refuses a direct message that carries only channel encryption when it
    // already knows the sender's public key - a guard against silently downgrading to a
    // weaker cipher. We publish ours, so every peer expects PKC from us, and without its
    // key we cannot oblige. Asking costs one unicast NodeInfo with want_response set.
    // ~60 s at the ~60 Hz tick the app counts in.
    static constexpr uint32_t KEY_REQ_INTERVAL = 3600;

    void request_key_from(uint32_t dest) {
        if (!dest || dest == BROADCAST_ADDR || !pki_enabled_) return;
        const NodeEntry* e = node_db_.find(dest);
        if (e && e->has_pubkey) return;
        key_req_dest_ = dest;
    }
    // We heard from someone we cannot identify - no NodeInfo, so no name and, worse, no
    // public key. Meshtastic sends a node's NodeInfo only every few hours (three, by
    // default), so after a restart we can sit deaf to a peer's encrypted direct messages
    // for that long: they arrive, fail the decrypt for want of the key, and are dropped
    // without so much as an acknowledgement - which is what the far end reports as
    // "max retransmit". Asking costs one broadcast; the answer carries the key.
    // Rate-limited, because "we do not know this node" is true of every packet it sends.
    // need_key = we just failed to open something this node encrypted to us, so it
    // demonstrably HAS a key and asking again is worth it. Otherwise a name is enough:
    // a peer with PKC switched off publishes a NodeInfo without a key, and asking after
    // one every minute for ever would be us talking to ourselves on a shared channel.
    void note_unknown_node(uint32_t from, uint32_t uptime_ticks, bool need_key = false) {
        if (!from || from == BROADCAST_ADDR || from == local_node_id_) return;
        const NodeEntry* e = node_db_.find(from);
        if (e && (need_key ? e->has_pubkey : e->long_name[0])) return;  // nothing to ask for
        if (key_req_tick_ && uptime_ticks - key_req_tick_ < KEY_REQ_INTERVAL) return;
        key_req_tick_ = uptime_ticks ? uptime_ticks : 1;
        key_req_dest_ = from;
    }

    uint32_t take_key_request() {
        const uint32_t d = key_req_dest_;
        key_req_dest_ = 0;
        return d;
    }

    size_t build_nodeinfo_tx(uint8_t* out_buf, size_t max_len, const char* long_name, const char* short_name, bool want_response = false, uint32_t dest = BROADCAST_ADDR);

    // Our direct neighbours and what we hear each of them at (portnum 71). A neighbour
    // is a node whose packet reached us without a hop, which is how Meshtastic decides
    // it too. interval_secs is how often we send this, and it is what the far end uses
    // to decide when to forget us. Returns 0 when we have no neighbours to report -
    // Meshtastic does not put an empty list on the air either.
    static constexpr size_t MAX_NEIGHBORS = 10;  // MAX_NUM_NEIGHBORS in NeighborInfoModule
    size_t build_neighborinfo_tx(uint8_t* out_buf, size_t max_len, uint32_t interval_secs);

    // Build a TELEMETRY packet. Telemetry.variant is a oneof, so one metrics type
    // per packet: device, environment or air quality.
    size_t build_telemetry_tx(uint8_t* out_buf, size_t max_len, const TelemetryData& tel, TelemetryData::Variant variant, const TelemetryData::ExtraMetric* extras = nullptr, size_t extra_count = 0, uint32_t dest = BROADCAST_ADDR,
                              // Packet id of the request this answers, echoed back so
                              // the asker can match it. Meshtastic's own tools wait for
                              // exactly that and ignore a reply without it.
                              uint32_t request_id = 0);

    // Ask a node for its LocalStats (the phone app's "Request local stats"): an empty
    // Telemetry carrying the local_stats variant, sent with want_response so the node
    // answers with its counters.
    // want_stats picks what is asked for: the router's own statistics, or the device
    // metrics (battery, voltage, airtime) a phone shows for a node. An empty Telemetry
    // with want_response is the plain "tell me about yourself" question.
    size_t build_stats_request_tx(uint8_t* out_buf, size_t max_len, uint32_t dest, bool want_stats = true);

    // Build an ACK routing packet in response to a received packet (on the same
    // channel it arrived on).
    // TraceRoute (portnum 70). Answers a RouteDiscovery request addressed to us:
    // `route` is the payload exactly as it arrived, `snr` what we measured of the
    // request, `hops_taken` how many relays it crossed to reach us.
    size_t build_traceroute_request_tx(uint8_t* out_buf, size_t max_len, uint32_t dest);
    size_t build_traceroute_tx(uint8_t* out_buf, size_t max_len, uint32_t dest, uint32_t request_id, const uint8_t* route, size_t route_len, float snr, uint8_t hops_taken, size_t channel_idx = 0);

    size_t build_ack_tx(uint8_t* out_buf, size_t max_len, uint32_t dest, uint32_t request_id, size_t channel_idx = 0);

    // Rewrite a received raw frame in place for rebroadcast: hop_limit-1 and
    // relay_node = our low node-id byte, like real relayers do - the sender
    // recognizes the echo as a relay (implicit ACK) and stops retransmitting.
    void make_relay(uint8_t* raw) const;

    void set_on_packet(PacketCallback cb) { on_packet_ = cb; }

    // Primary-channel hash used in TX headers and as the RX channel filter.
    // (Compat shim: sets channel 0's hash.)
    void set_channel_hash(uint8_t h) {
        channels_[0].hash = h;
        channels_[0].enabled = true;
    }
    // Hop limit applied to every packet we originate (0 -> default).
    void set_hop_limit(uint8_t h) { hop_limit_ = h ? h : DEFAULT_HOP_LIMIT; }
    // Set the ok_to_mqtt bit on our text messages (gateway may bridge to MQTT).
    void set_ok_to_mqtt(bool v) { ok_to_mqtt_ = v; }
    // LoRaConfig.ignore_mqtt: when set, packets flagged via_mqtt are neither
    // re-flooded nor delivered to the UI. They are still parsed into the node DB
    // (names, public keys, positions) - ignoring MQTT traffic must not blind us
    // to who is on the mesh.
    void set_ignore_mqtt(bool v) { ignore_mqtt_ = v; }
    // HardwareModel we advertise in NodeInfo (255 = PRIVATE_HW; or spoof a common node).
    void set_hw_model(uint8_t m) { hw_model_ = m; }
    // DeviceConfig.Role advertised in NodeInfo. Without it every peer files us under
    // CLIENT, whatever the app shows locally.
    void set_role(uint8_t r) { role_ = r; }

    // Configure our PKC identity keypair (Curve25519). When enabled, we advertise the
    // public key in NodeInfo (peers show a green padlock) and outgoing direct messages
    // to a node whose public key we know are encrypted with AES-256-CCM (channel 0).
    void set_pki(const uint8_t priv[32], const uint8_t pub[32], bool enabled);
    bool pki_enabled() const { return pki_enabled_; }

    // Configure a channel slot (idx 0 = primary). psk of length 0 clears the key.
    void set_channel(size_t idx, uint8_t hash, const uint8_t* psk, size_t psk_len, bool enabled);
    // Which channel new TX packets go out on.
    void set_active_channel(size_t idx) {
        if (idx < MAX_CHANNELS) active_ch_ = idx;
    }
    size_t active_channel() const { return active_ch_; }

    // Seed the packet-id counter (call once at startup with an RTC-derived
    // value): real Meshtastic dedup caches (from, id) pairs, so restarting
    // from id=1 every boot gets fresh packets silently dropped as duplicates.
    void seed_packet_id(uint32_t seed) { packet_counter_ = seed; }

    // The counters behind LocalStats (Telemetry variant 6). Meshtastic answers a
    // request for these over the radio and otherwise hands them only to an attached
    // phone - they are never broadcast, so they cost no airtime until someone asks.
    struct Counters {
        uint32_t rx{0};          // frames handed up by the demodulator
        uint32_t rx_bad{0};      // of those, malformed or undecodable
        uint32_t rx_dupe{0};     // already seen, dropped by the flood cache
        uint32_t tx{0};          // frames accepted for transmission
        uint32_t tx_relay{0};    // of those, someone else's carried one hop further
        uint32_t tx_dropped{0};  // could not be sent: the radio was already busy
    };
    const Counters& counters() const { return counters_; }
    // The channel hash of the last frame turned away by the filter, and how many were.
    // A one-byte mismatch here drops every packet before decryption and leaves no other
    // trace at all - it cost two days across SHORT_FAST and BW125 before anyone thought
    // to look at the byte itself.
    uint8_t last_bad_hash() const { return last_bad_hash_; }
    // ...and who that frame claimed to come from. The header search only ever validates
    // the first four bytes (the destination), so a sender that reads correctly puts the
    // corruption after byte 8 - and a garbled one puts it in the very first payload
    // block. Two hex digits that split the search in half.
    uint32_t last_bad_from() const { return last_bad_from_; }
    uint32_t bad_hash_count() const { return bad_hash_n_; }
    uint8_t primary_hash() const { return channels_[0].hash; }
    void note_tx(bool relayed = false) {
        counters_.tx++;
        if (relayed) counters_.tx_relay++;
    }
    void note_tx_dropped() { counters_.tx_dropped++; }
    // Counted where the relay is built, since queue_tx cannot tell whose packet it is.
    void note_relayed() { counters_.tx_relay++; }

    uint32_t local_node_id() const { return local_node_id_; }
    uint32_t next_packet_id();

   private:
    // A peer we owe a key request. Any view can raise it; the app drains it from its
    // timer, where there is stack to spare for building the packet.
    uint32_t key_req_dest_{0};
    uint32_t key_req_tick_{0};       // when we last asked, so we ask at most now and then
    uint32_t last_uptime_ticks_{0};  // the clock the decrypt path has no argument for
    // End-to-end encrypt an already-built Data protobuf when the destination's public
    // key is known. Returns the packet length, or 0 when PKC does not apply and the
    // caller should fall back to channel encryption.
    size_t build_pki_packet(uint8_t* out_buf, size_t max_len, uint32_t dest, uint32_t pid, bool want_ack, const uint8_t* data_buf, size_t data_len);
    NodeDB& node_db_;
    uint32_t local_node_id_{0xDEADBEEF};
    uint32_t packet_counter_{0};
    Counters counters_{};
    uint8_t last_bad_hash_{0};
    uint32_t last_bad_from_{0};
    uint32_t bad_hash_n_{0};
    uint8_t hop_limit_{DEFAULT_HOP_LIMIT};
    bool ok_to_mqtt_{false};
    bool ignore_mqtt_{false};
    uint8_t hw_model_{255};  // PRIVATE_HW
    uint8_t role_{0};        // DeviceConfig.Role, 0 = CLIENT
    std::array<ChannelKey, MAX_CHANNELS> channels_{};
    size_t active_ch_{0};
    MeshCrypto crypto_{};

    uint8_t pki_priv_[32]{};  // our Curve25519 private key
    uint8_t pki_pub_[32]{};   // our Curve25519 public key
    bool pki_enabled_{false};

    // Load crypto_ with a channel's key (re-keyed per TX build / RX decrypt).
    void use_channel_key(size_t idx);
    // Find the enabled channel whose hash matches; -1 if none.
    int channel_for_hash(uint8_t hash) const;

    std::array<SeenEntry, SEEN_CACHE_SIZE> seen_{};
    size_t seen_head_{0};

    PacketCallback on_packet_{};

    bool is_duplicate(uint32_t from, uint32_t packet_id);
    void mark_seen(uint32_t from, uint32_t packet_id);

    // Attempt to decrypt and decode the Data payload.
    bool decrypt_decode(MeshPacket& pkt, const uint8_t* encrypted, size_t enc_len);
    // Attempt to decrypt a PKC (AES-256-CCM) direct message addressed to us.
    // Payload layout: [ciphertext][8-byte tag][4-byte extraNonce]. Needs the
    // sender's public key from the node DB; returns false if we don't have it
    // or the tag fails to verify.
    bool pki_decrypt_decode(MeshPacket& pkt, const uint8_t* payload, size_t len);

    // Dispatch decoded packet to node_db and (when deliver) the UI callback.
    void dispatch(const MeshPacket& pkt, bool deliver = true);

    // Build the 14-byte PHY header into buf.
    void write_header(uint8_t* buf, uint32_t to, uint32_t packet_id, uint8_t hop_limit, bool want_ack, uint8_t channel_hash) const;
};

}  // namespace meshtastic

#endif /* __MESH_ROUTER_H__ */
