/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mesh_router.hpp"
#include "mesh_pki.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace meshtastic {

void MeshRouter::set_local_node(uint32_t node_id, const uint8_t* psk, size_t psk_len) {
    local_node_id_ = node_id;
    // Channel 0 (primary) uses this key; its hash is set via set_channel_hash().
    set_channel(0, channels_[0].hash, psk, psk_len, true);
}

void MeshRouter::set_pki(const uint8_t priv[32], const uint8_t pub[32], bool enabled) {
    memcpy(pki_priv_, priv, 32);
    memcpy(pki_pub_, pub, 32);
    pki_enabled_ = enabled;
}

// Meshtastic PKC nonce, byte for byte as CryptoEngine::initNonce builds it:
//   memcpy(nonce, &packetId, 8); memcpy(nonce + 8, &fromNode, 4);
//   if (extraNonce) memcpy(nonce + 4, &extraNonce, 4);
// So the 32-bit extra nonce lands at offset 4 - inside the high half of the 64-bit
// packet id, not at the end - and byte 12 stays zero. (Whatever the intent was, this
// is the wire format stock firmware uses; anything else fails its CCM tag check, which
// is why real nodes silently ignored our direct messages.)
static void pki_build_nonce(uint8_t nonce[13], uint32_t packet_id, uint32_t from_node, uint32_t extra) {
    memset(nonce, 0, 13);
    nonce[0] = packet_id & 0xFF;
    nonce[1] = (packet_id >> 8) & 0xFF;
    nonce[2] = (packet_id >> 16) & 0xFF;
    nonce[3] = (packet_id >> 24) & 0xFF;
    if (extra) {
        nonce[4] = extra & 0xFF;
        nonce[5] = (extra >> 8) & 0xFF;
        nonce[6] = (extra >> 16) & 0xFF;
        nonce[7] = (extra >> 24) & 0xFF;
    }
    nonce[8] = from_node & 0xFF;
    nonce[9] = (from_node >> 8) & 0xFF;
    nonce[10] = (from_node >> 16) & 0xFF;
    nonce[11] = (from_node >> 24) & 0xFF;
}

void MeshRouter::set_channel(size_t idx, uint8_t hash, const uint8_t* psk, size_t psk_len, bool enabled) {
    if (idx >= MAX_CHANNELS) return;
    ChannelKey& c = channels_[idx];
    c.hash = hash;
    c.psk_len = (psk_len > sizeof(c.psk)) ? static_cast<uint8_t>(sizeof(c.psk))
                                          : static_cast<uint8_t>(psk_len);
    if (psk && c.psk_len) memcpy(c.psk, psk, c.psk_len);
    c.enabled = enabled;
}

void MeshRouter::use_channel_key(size_t idx) {
    if (idx >= MAX_CHANNELS) return;
    crypto_.set_key(channels_[idx].psk, channels_[idx].psk_len);
}

int MeshRouter::channel_for_hash(uint8_t hash) const {
    for (size_t i = 0; i < MAX_CHANNELS; i++)
        if (channels_[i].enabled && channels_[i].hash == hash) return static_cast<int>(i);
    return -1;
}

uint32_t MeshRouter::next_packet_id() {
    return ++packet_counter_;
}

bool MeshRouter::is_duplicate(uint32_t from, uint32_t packet_id) {
    for (const auto& e : seen_) {
        if (e.valid && e.from == from && e.packet_id == packet_id) return true;
    }
    return false;
}

void MeshRouter::mark_seen(uint32_t from, uint32_t packet_id) {
    seen_[seen_head_] = {from, packet_id, true};
    seen_head_ = (seen_head_ + 1) % SEEN_CACHE_SIZE;
}

bool MeshRouter::on_raw_rx(const uint8_t* raw, size_t len, int8_t rssi, float snr, uint32_t uptime_ticks) {
    last_uptime_ticks_ = uptime_ticks;  // the decrypt path below has no clock of its own
    counters_.rx++;
    if (len < PKT_HEADER_SIZE || len > PKT_MAX_SIZE) {
        counters_.rx_bad++;
        return false;
    }

    MeshPacket pkt;
    pkt.header = PacketHeader::from_bytes(raw);
    pkt.rx_rssi = rssi;
    pkt.rx_snr = snr;

    // Channel filter: accept a packet only if its channel hash matches one of our
    // configured (enabled) channels. This also rejects non-Meshtastic frames (e.g.
    // a raw test beacon), which must never reach the decoder / rebroadcaster -
    // parsing/rebroadcasting garbage crashed M0 (Guru Meditation).
    // A PKC direct message to us rides "channel 0" (hash byte == 0). Accept it even
    // though hash 0 normally matches no configured channel; decrypt it with our key.
    const bool pkc = pki_enabled_ && pkt.header.channel_hash == 0 &&
                     pkt.header.to == local_node_id_;
    // A PKC direct message between two OTHER nodes is encrypted with their shared key,
    // so we cannot read a byte of it - and it carries channel-hash 0, which matches no
    // channel of ours, so the filter below threw it away. That made this node a black
    // hole in the mesh: every private message whose path ran through us died here.
    // Passing it on needs no decryption at all, only the hop count decremented, which
    // is what relay_only does. Narrow on purpose - hash 0, addressed to one node that
    // is not us - so ordinary noise is still rejected rather than re-broadcast.
    const bool relay_only = !pkc && pkt.header.channel_hash == 0 &&
                            pkt.header.to != BROADCAST_ADDR &&
                            pkt.header.to != local_node_id_;

    int ch = -1;
    if (pkc) {
        pkt.channel_index = 0;  // PKC direct messages ride the primary channel slot
    } else if (!relay_only) {
        ch = channel_for_hash(pkt.header.channel_hash);
        if (ch < 0) {
            last_bad_hash_ = pkt.header.channel_hash;
            last_bad_from_ = pkt.header.from;
            bad_hash_n_++;
            return false;
        }
        pkt.channel_index = static_cast<uint8_t>(ch);
    }

    // Drop packets sent by ourselves
    if (pkt.header.from == local_node_id_) return false;

    // LoRaConfig.ignore_mqtt governs MQTT *routing*: a packet that reached us through
    // an MQTT gateway must not be re-flooded and is not surfaced in chat - but we still
    // learn its sender (name, public key, position, signal). Dropping it outright also
    // threw away NodeInfo, so peers' public keys were never learned and PKC direct
    // messages from them could never be decrypted.
    const bool mqtt_ignored = ignore_mqtt_ && pkt.header.via_mqtt();

    // Deduplication
    if (is_duplicate(pkt.header.from, pkt.header.packet_id)) {
        counters_.rx_dupe++;
        return false;
    }
    mark_seen(pkt.header.from, pkt.header.packet_id);

    // Nothing here to decode or show - just carry it one hop further for its owner.
    // The caller applies the node's role (a muted or hidden client repeats nothing)
    // before it actually keys the radio, so that check does not belong here.
    if (relay_only)
        return !mqtt_ignored && pkt.header.hop_limit() > 0;

    const uint8_t* payload = raw + PKT_HEADER_SIZE;
    const size_t pay_len = len - PKT_HEADER_SIZE;

    if (pay_len > 0) {
        if (pkc) {
            pki_decrypt_decode(pkt, payload, pay_len);  // AES-256-CCM with sender's pubkey
        } else {
            use_channel_key(static_cast<size_t>(ch));  // decrypt with the matched channel's key
            decrypt_decode(pkt, payload, pay_len);
        }
        if (pkt.decoded)
            dispatch(pkt, !mqtt_ignored);
        else
            counters_.rx_bad++;  // decrypted to something that is not a Data
    }

    // Everything below trusts the frame, so nothing below runs for one we could not
    // decode. A single bit slipping through the demodulator lands in the header as
    // often as in the payload, and a corrupted sender id used to be entered in the node
    // list as a new neighbour and then re-flooded to the mesh - inventing a node that
    // does not exist, for every node in earshot. A frame whose payload decrypts and
    // parses is one whose header survived too.
    if (!pkt.decoded) return false;

    // Update node DB with signal info. Hops travelled = hop_start - hop_limit
    // (0 for a direct neighbour). The old code used MAX_HOP_LIMIT as the baseline,
    // so a direct packet from a node whose hop-limit is 3 read as 7-3 = 4 hops.
    const uint8_t hs = pkt.header.hop_start(), hl = pkt.header.hop_limit();
    node_db_.update_signal(pkt.header.from, rssi, snr,
                           (hs >= hl) ? (hs - hl) : 0,
                           uptime_ticks);
    // A node we can hear but cannot name has never sent us its NodeInfo, so we have
    // neither its name nor its public key. Ask. It is the same request the phone app
    // calls "exchange user info", and it is what fills the node list with names.
    note_unknown_node(pkt.header.from, uptime_ticks);

    // Should we re-broadcast? Only if hop_limit > 0 and not unicast-to-us
    if (mqtt_ignored) return false;
    if (pkt.header.hop_limit() == 0) return false;
    if (pkt.header.to == local_node_id_) return false;  // unicast to us, no rebroadcast
    return true;
}

bool MeshRouter::decrypt_decode(MeshPacket& pkt, const uint8_t* encrypted, size_t enc_len) {
    uint8_t buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    const size_t n = std::min(enc_len, sizeof(buf));
    memcpy(buf, encrypted, n);

    if (crypto_.has_key()) {
        crypto_.crypt(buf, n, pkt.header.packet_id, pkt.header.from);
        pkt.encrypted = true;
    }

    if (!DecodedData::decode(buf, n, pkt.data)) return false;
    pkt.decoded = true;
    return true;
}

bool MeshRouter::pki_decrypt_decode(MeshPacket& pkt, const uint8_t* payload, size_t len) {
    // Layout: [ciphertext][8-byte tag][4-byte extraNonce]. Need at least a tag,
    // the extra nonce and one plaintext byte.
    if (len < 8 + 4 + 1) return false;
    const NodeEntry* peer = node_db_.find(pkt.header.from);
    if (!peer || !peer->has_pubkey) {
        // Someone is talking to us privately and we have no key to open it with. Say so
        // to the mesh rather than dropping it in silence - see note_unknown_node().
        note_unknown_node(pkt.header.from, last_uptime_ticks_, true);
        return false;
    }

    const size_t ct_len = len - 12;
    const uint8_t* tag = payload + ct_len;
    const uint32_t extra = payload[ct_len + 8] | (payload[ct_len + 9] << 8) |
                           (payload[ct_len + 10] << 16) | (payload[ct_len + 11] << 24);

    uint8_t shared[32], key[32];
    pki::x25519(shared, pki_priv_, peer->public_key);
    pki::sha256(shared, 32, key);
    uint8_t nonce[13];
    pki_build_nonce(nonce, pkt.header.packet_id, pkt.header.from, extra);

    uint8_t buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    if (ct_len > sizeof(buf)) return false;
    if (!pki::aes_ccm_decrypt(key, nonce, 13, payload, ct_len, tag, buf)) return false;
    if (!DecodedData::decode(buf, ct_len, pkt.data)) return false;
    pkt.encrypted = true;
    pkt.decoded = true;
    return true;
}

void MeshRouter::dispatch(const MeshPacket& pkt, bool deliver) {
    // Update node DB from content. This half runs even for packets we only observe
    // (deliver == false, e.g. ignore_mqtt): learning who is out there and their
    // public keys is independent of whether the payload is shown to the user.
    if (pkt.data.portnum == PortNum::NODEINFO) {
        NodeInfoData ni;
        if (NodeInfoData::decode(pkt.data.payload, pkt.data.payload_len, ni)) {
            node_db_.update_node(pkt.header.from, ni.long_name, ni.short_name);
            node_db_.set_identity(pkt.header.from, ni.hw_model, ni.role);
            if (NodeEntry* e = node_db_.find(pkt.header.from)) e->unmessagable = ni.unmessagable;
            if (ni.has_pubkey) node_db_.set_pubkey(pkt.header.from, ni.public_key);
        }
    } else if (pkt.data.portnum == PortNum::NEIGHBORINFO) {
        NeighborInfoData ni;
        if (NeighborInfoData::decode(pkt.data.payload, pkt.data.payload_len, ni,
                                     local_node_id_)) {
            // Ignore the dummy packet Meshtastic guards against: a single neighbour
            // with node id 0 and no signal.
            if (!(ni.count == 1 && ni.node_id == 0)) {
                NodeEntry* e = node_db_.find(pkt.header.from);
                if (e) {
                    e->nbr_count = ni.count;
                    e->nbr_hears_us = ni.has_us;
                    if (ni.has_us) e->nbr_snr_to_us = ni.snr_to_us;
                }
            }
        }
    } else if (pkt.data.portnum == PortNum::POSITION) {
        PositionData pos;
        if (PositionData::decode(pkt.data.payload, pkt.data.payload_len, pos)) {
            node_db_.update_position(pkt.header.from, pos);
        }
    } else if (pkt.data.portnum == PortNum::TELEMETRY && !pkt.data.want_response) {
        // Answers only. A request is an empty copy of the variant it asks for, so it
        // decodes into a perfectly valid reading of all zeroes - and storing that wiped
        // the asking node's real figures the instant it asked us for ours. Seen on the
        // air: a Heltec broadcasting 101% and 4.36 V showed as 0% and 0.0 V on its card
        // from the moment it requested our metrics. want_response is what tells the
        // question from the answer.
        //
        // Static, not a local: the full telemetry record covers every message in
        // telemetry.proto and is far too large for this stack (the M0's main process
        // gets 4 KB, and this runs deep inside packet handling).
        static TelemetryData tel;
        tel = TelemetryData{};
        if (TelemetryData::decode(pkt.data.payload, pkt.data.payload_len, tel))
            node_db_.update_telemetry(pkt.header.from, tel);
    }

    if (deliver && on_packet_) on_packet_(pkt);
}

void MeshRouter::write_header(uint8_t* buf, uint32_t to, uint32_t packet_id, uint8_t hop_limit, bool want_ack, uint8_t channel_hash) const {
    PacketHeader h;
    h.to = to;
    h.from = local_node_id_;
    h.packet_id = packet_id;
    h.flags = (hop_limit & 0x07) | (want_ack ? 0x08 : 0x00) | (hop_limit << 5);
    h.channel_hash = channel_hash;
    // Originator marks itself as the first "relayer" (RadioInterface.cpp does
    // the same) so relay-vs-origin detection by other nodes works.
    h.relay_node = static_cast<uint8_t>(local_node_id_ & 0xFF);
    h.to_bytes(buf);
}

void MeshRouter::make_relay(uint8_t* raw) const {
    // flags byte [12]: hop_limit lives in bits 0-2; hop_start (bits 5-7),
    // want_ack and via_mqtt must survive untouched.
    const uint8_t flags = raw[12];
    const uint8_t hops = flags & 0x07;
    raw[12] = (flags & ~0x07) | (hops ? (hops - 1) : 0);
    raw[15] = static_cast<uint8_t>(local_node_id_ & 0xFF);  // relay_node
}

// ---- TX builders -----------------------------------------------------------

// Data.bitfield bit 0 (ok_to_mqtt) rides on EVERY packet we originate, not just text.
// An MQTT gateway drops another node's packet that lacks it whenever the broker is a
// public one - "DontMqttMeBro" in Meshtastic's MQTT.cpp - and NodeInfo and position are
// precisely the packets a map needs, so without the bit we never appeared on one.
static size_t build_data_payload(uint8_t* buf, size_t max_len, PortNum portnum, const uint8_t* payload, size_t payload_len, bool want_response = false, bool ok_to_mqtt = false, uint32_t request_id = 0) {
    DecodedData d;
    d.portnum = portnum;
    d.want_response = want_response;
    d.ok_to_mqtt = ok_to_mqtt;
    d.request_id = request_id;
    memcpy(d.payload, payload, std::min(payload_len, sizeof(d.payload)));
    d.payload_len = static_cast<uint8_t>(std::min(payload_len, sizeof(d.payload)));
    return d.encode(buf, max_len);
}

// PKC: a direct message to a node whose public key we know is encrypted with
// AES-256-CCM and rides "channel 0" (hash byte 0). Payload = [ct][tag8][extra4].
// Every unicast we send has to take this path once the peer holds our public key -
// Meshtastic refuses a merely channel-encrypted direct message from such a sender,
// which for a long time silently swallowed our echoes and read receipts alike.
size_t MeshRouter::build_pki_packet(uint8_t* out_buf, size_t max_len, uint32_t dest, uint32_t pid, bool want_ack, const uint8_t* data_buf, size_t data_len) {
    const NodeEntry* peer = (dest != BROADCAST_ADDR) ? node_db_.find(dest) : nullptr;
    if (!pki_enabled_ || !peer || !peer->has_pubkey ||
        PKT_HEADER_SIZE + data_len + 12 > max_len)
        return 0;

    uint8_t shared[32], key[32];
    pki::x25519(shared, pki_priv_, peer->public_key);
    pki::sha256(shared, 32, key);
    const uint32_t extra = pid * 2654435761u;  // per-message nonce salt
    uint8_t nonce[13];
    pki_build_nonce(nonce, pid, local_node_id_, extra);

    write_header(out_buf, dest, pid, hop_limit_, want_ack, 0);
    uint8_t* out = out_buf + PKT_HEADER_SIZE;
    uint8_t tag[8];
    pki::aes_ccm_encrypt(key, nonce, 13, data_buf, data_len, out, tag);
    memcpy(out + data_len, tag, 8);
    out[data_len + 8] = extra & 0xFF;
    out[data_len + 9] = (extra >> 8) & 0xFF;
    out[data_len + 10] = (extra >> 16) & 0xFF;
    out[data_len + 11] = (extra >> 24) & 0xFF;
    return PKT_HEADER_SIZE + data_len + 12;
}

size_t MeshRouter::build_text_tx(uint8_t* out_buf, size_t max_len, const char* text, size_t text_len, uint32_t dest, bool want_ack) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    const uint32_t pid = next_packet_id();

    // Plaintext Data protobuf (portnum=TEXT + message) - encrypted below.
    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf),
                                         PortNum::TEXT_MESSAGE,
                                         reinterpret_cast<const uint8_t*>(text),
                                         text_len, want_ack, ok_to_mqtt_);

    if (const size_t n = build_pki_packet(out_buf, max_len, dest, pid, want_ack,
                                          data_buf, data_len))
        return n;

    // Otherwise: shared-channel encryption on the currently-selected channel.
    write_header(out_buf, dest, pid, hop_limit_, want_ack, channels_[active_ch_].hash);
    use_channel_key(active_ch_);
    if (crypto_.has_key()) {
        crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    }
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

size_t MeshRouter::build_position_tx(uint8_t* out_buf, size_t max_len, const PositionData& pos) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    const uint32_t pid = next_packet_id();
    write_header(out_buf, BROADCAST_ADDR, pid, hop_limit_, false, channels_[0].hash);
    use_channel_key(0);  // position/telemetry/nodeinfo always ride the primary channel

    uint8_t pos_buf[64];
    size_t pos_len = pos.encode(pos_buf, sizeof(pos_buf));

    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf),
                                         PortNum::POSITION, pos_buf, pos_len,
                                         false, ok_to_mqtt_);
    if (crypto_.has_key()) {
        crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    }
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

size_t MeshRouter::build_telemetry_tx(uint8_t* out_buf, size_t max_len, const TelemetryData& tel, TelemetryData::Variant variant, const TelemetryData::ExtraMetric* extras, size_t extra_count, uint32_t dest, uint32_t request_id) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    const uint32_t pid = next_packet_id();

    uint8_t tel_buf[128];
    size_t tel_len = tel.encode(tel_buf, sizeof(tel_buf), variant, extras, extra_count);

    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf),
                                         PortNum::TELEMETRY, tel_buf, tel_len,
                                         false, ok_to_mqtt_, request_id);

    // Answering one node rather than the mesh is a unicast, and a unicast to a peer
    // that holds our public key must be PKC or it is refused - the same no-downgrade
    // rule that swallowed the stats request. A broadcast still rides the channel key.
    if (const size_t n = build_pki_packet(out_buf, max_len, dest, pid, false,
                                          data_buf, data_len))
        return n;

    write_header(out_buf, dest, pid, hop_limit_, false, channels_[0].hash);
    use_channel_key(0);  // position/telemetry/nodeinfo always ride the primary channel
    if (crypto_.has_key()) {
        crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    }
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

size_t MeshRouter::build_nodeinfo_tx(uint8_t* out_buf, size_t max_len, const char* long_name, const char* short_name, bool want_response, uint32_t dest) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    const uint32_t pid = next_packet_id();
    write_header(out_buf, dest, pid, hop_limit_, false, channels_[0].hash);
    use_channel_key(0);  // position/telemetry/nodeinfo always ride the primary channel

    // Hand-encode User protobuf (mesh.proto): field1=id(string "!hexid"),
    // field2=long_name(str), field3=short_name(str), field5=hw_model(enum).
    // Field 1 as a varint num (old code) is a wire-type mismatch that makes
    // real firmware drop the whole NODEINFO.
    uint8_t ni_buf[128];  // room for the optional 34-byte public_key field
    size_t ni_len = 0;
    // field 1: id string "!xxxxxxxx" (lowercase hex, like stock firmware)
    ni_buf[ni_len++] = 0x0A;
    ni_buf[ni_len++] = 9;
    ni_buf[ni_len++] = '!';
    for (int k = 28; k >= 0; k -= 4)
        ni_buf[ni_len++] = "0123456789abcdef"[(local_node_id_ >> k) & 0xF];
    // field 2: long_name
    if (long_name && long_name[0]) {
        const size_t slen = std::min(strlen(long_name), size_t{40});
        ni_buf[ni_len++] = 0x12;
        ni_len += encode_varint(ni_buf + ni_len, slen);
        memcpy(ni_buf + ni_len, long_name, slen);
        ni_len += slen;
    }
    // field 3: short_name
    if (short_name && short_name[0]) {
        const size_t slen = std::min(strlen(short_name), size_t{4});
        ni_buf[ni_len++] = 0x1A;
        ni_len += encode_varint(ni_buf + ni_len, slen);
        memcpy(ni_buf + ni_len, short_name, slen);
        ni_len += slen;
    }
    // field 5: hw_model - 255 (PRIVATE_HW) by default, or a spoofed common node
    // so peers show us as e.g. a Heltec V3 / T-Beam.
    ni_buf[ni_len++] = 0x28;
    ni_len += encode_varint(ni_buf + ni_len, hw_model_);
    // field 7: role (DeviceConfig.Role). Omitted for CLIENT, which is the proto
    // default; without this field a peer always displays us as a client.
    if (role_) {
        ni_buf[ni_len++] = 0x38;
        ni_len += encode_varint(ni_buf + ni_len, role_);
    }
    // field 9: is_unmessagable. Set for exactly the roles Meshtastic sets it for in
    // installRoleDefaults() - router (2), tracker (5), sensor (6), TAK tracker (10) and
    // router late (11) - the ones with nobody watching a screen. It tells other clients
    // not to offer to write to us; it does not stop anyone who tries.
    if (role_ == 2 || role_ == 5 || role_ == 6 || role_ == 10 || role_ == 11) {
        ni_buf[ni_len++] = 0x48;  // field 9, varint
        ni_buf[ni_len++] = 1;
    }
    // field 8: public_key (32-byte Curve25519) - advertised only when PKC is on, so
    // peers can encrypt direct messages to us (they show a green padlock).
    if (pki_enabled_) {
        ni_buf[ni_len++] = 0x42;  // field 8, wire type 2 (length-delimited)
        ni_buf[ni_len++] = 32;
        memcpy(ni_buf + ni_len, pki_pub_, 32);
        ni_len += 32;
    }

    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf),
                                         PortNum::NODEINFO, ni_buf, ni_len,
                                         want_response, ok_to_mqtt_);
    if (crypto_.has_key()) {
        crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    }
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

size_t MeshRouter::build_neighborinfo_tx(uint8_t* out_buf, size_t max_len, uint32_t interval_secs) {
    if (max_len < PKT_HEADER_SIZE) return 0;

    // NeighborInfo { node_id=1, last_sent_by_id=2, node_broadcast_interval_secs=3,
    //                repeated Neighbor neighbors=4 }
    uint8_t ni[160];
    size_t n = 0;
    ni[n++] = 0x08;
    n += encode_varint(ni + n, local_node_id_);
    ni[n++] = 0x10;
    n += encode_varint(ni + n, local_node_id_);  // we put it on the air
    ni[n++] = 0x18;
    n += encode_varint(ni + n, interval_secs);

    // A neighbour is one we heard without a hop. Anything we have not heard from in
    // twice its broadcast interval has gone, exactly as cleanUpNeighbors() decides it.
    size_t count = 0;
    for (size_t i = 0; i < node_db_.count() && count < MAX_NEIGHBORS; i++) {
        const NodeEntry* e = node_db_.at(i);
        if (!e || !e->active || e->node_id == local_node_id_) continue;
        if (e->hop_count != 0) continue;
        if (last_uptime_ticks_ > e->last_seen_uptime &&
            (last_uptime_ticks_ - e->last_seen_uptime) > interval_secs * 2u * 60u)
            continue;  // ~60 ticks a second
        // Neighbor { node_id=1 (varint), snr=2 (float) }. last_rx_time and the
        // per-neighbour interval stay at home - see collectNeighborInfo().
        uint8_t nb[16];
        size_t m = 0;
        nb[m++] = 0x08;
        m += encode_varint(nb + m, e->node_id);
        nb[m++] = 0x15;
        float snr = e->last_snr;
        memcpy(nb + m, &snr, 4);
        m += 4;
        if (n + 2 + m > sizeof(ni)) break;
        ni[n++] = 0x22;
        n += encode_varint(ni + n, m);
        memcpy(ni + n, nb, m);
        n += m;
        count++;
    }
    if (count == 0) return 0;  // nothing to say; Meshtastic stays quiet too

    const uint32_t pid = next_packet_id();
    write_header(out_buf, BROADCAST_ADDR, pid, hop_limit_, false, channels_[0].hash);
    use_channel_key(0);
    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf),
                                         PortNum::NEIGHBORINFO, ni, n,
                                         false, ok_to_mqtt_);
    if (crypto_.has_key()) crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

size_t MeshRouter::build_stats_request_tx(uint8_t* out_buf, size_t max_len, uint32_t dest, bool want_stats) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    const uint32_t pid = next_packet_id();

    // Telemetry.variant is a oneof, and a request is an EMPTY copy of the variant it
    // wants: field 6 for LocalStats, field 2 for the device metrics a phone shows.
    // The variant has to be there. This used to send a bare empty Telemetry when asking
    // for metrics, and a stock node answers only when the request decodes with the
    // device metrics variant set, so nothing ever came back. Measured against a Heltec
    // V4: it decrypted and decoded the request - its own log named the port and the
    // want_response flag - and still said nothing until the two bytes were present.
    const uint8_t tel_stats[2] = {0x32, 0x00};  // local_stats {}
    const uint8_t tel_dev[2] = {0x12, 0x00};    // device_metrics {}
    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf), PortNum::TELEMETRY,
                                         want_stats ? tel_stats : tel_dev, 2, true);

    // Through PKC when the peer's key is known, exactly as a text message goes. This
    // request is a unicast, and a node that holds our public key drops any unicast from
    // us that carries only channel encryption - the same no-downgrade rule that once
    // swallowed our messages and read receipts. Without this the request never reached
    // the far node's telemetry module at all, so nothing ever came back.
    if (const size_t n = build_pki_packet(out_buf, max_len, dest, pid, false,
                                          data_buf, data_len))
        return n;

    write_header(out_buf, dest, pid, hop_limit_, false, channels_[0].hash);
    use_channel_key(0);
    if (crypto_.has_key()) crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

// "Trace" on the node card: ask a node which path reaches it. The payload is an empty
// RouteDiscovery - every node along the way appends itself, so there is nothing for us
// to put in it. Unicast, and so PKC-first for the same reason the telemetry request is:
// a node holding our public key drops a channel-encrypted unicast unread.
size_t MeshRouter::build_traceroute_request_tx(uint8_t* out_buf, size_t max_len, uint32_t dest) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    const uint32_t pid = next_packet_id();

    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = build_data_payload(data_buf, sizeof(data_buf), PortNum::TRACEROUTE,
                                         nullptr, 0, true);

    if (const size_t n = build_pki_packet(out_buf, max_len, dest, pid, false,
                                          data_buf, data_len))
        return n;

    write_header(out_buf, dest, pid, hop_limit_, false, channels_[0].hash);
    use_channel_key(0);
    if (crypto_.has_key()) crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

// A protobuf int32 is not zigzag-coded: a negative one is sign-extended to 64 bits
// and comes out ten bytes long. SNRs below zero are the normal case here, so this has
// to be right or the phone reads a nonsense hop.
static size_t put_varint_i32(uint8_t* p, int32_t v) {
    uint64_t u = static_cast<uint64_t>(static_cast<int64_t>(v));
    size_t i = 0;
    while (u >= 0x80) {
        p[i++] = static_cast<uint8_t>(u) | 0x80;
        u >>= 7;
    }
    p[i++] = static_cast<uint8_t>(u);
    return i;
}

// TraceRoute (portnum 70): someone asked which path reaches us, and the reply is the
// RouteDiscovery they sent with our own leg appended. This follows Meshtastic's
// TraceRouteModule::alterReceivedProtobuf, which runs before the reply is sent:
//
//   insertUnknownHops()   - the destination cannot name the relays that carried the
//                           request, so it pads the route with one 0xFFFFFFFF entry
//                           and one INT8_MIN ("unknown") SNR per hop taken;
//   appendMyIDandSNR()    - and then appends only its SNR, not its ID: the node that
//                           asked already knows which node it addressed.
//
// The appended fields are written unpacked while a stock node packs them. Both are
// valid protobuf for a repeated field and every decoder merges the two forms, which
// is what lets this be a copy of the incoming bytes plus a short tail.
size_t MeshRouter::build_traceroute_tx(uint8_t* out_buf, size_t max_len, uint32_t dest, uint32_t request_id, const uint8_t* route, size_t route_len, float snr, uint8_t hops_taken, size_t channel_idx) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    if (channel_idx >= MAX_CHANNELS) channel_idx = 0;
    const uint32_t pid = next_packet_id();

    uint8_t payload[256];
    // A route is capped at 8 nodes, so the tail below can never need more than
    // 8*(5+10) bytes; leaving that much room means the copy cannot be cut mid-field.
    size_t n = std::min(route_len, sizeof(payload) - 128);
    if (route_len) memcpy(payload, route, n);

    if (hops_taken > 7) hops_taken = 7;
    for (uint8_t i = 0; i < hops_taken; i++) {
        payload[n++] = 0x0D;  // field 1 (route), fixed32
        payload[n++] = 0xFF;
        payload[n++] = 0xFF;  // 0xFFFFFFFF = a hop we cannot name
        payload[n++] = 0xFF;
        payload[n++] = 0xFF;
        payload[n++] = 0x10;                     // field 2 (snr_towards), varint
        n += put_varint_i32(payload + n, -128);  // INT8_MIN = SNR unknown
    }

    int32_t q4 = static_cast<int32_t>(lroundf(snr * 4.0f));
    if (q4 > 127) q4 = 127;
    if (q4 < -127) q4 = -127;  // -128 is spoken for: it means "unknown"
    payload[n++] = 0x10;
    n += put_varint_i32(payload + n, q4);

    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    const size_t data_len = build_data_payload(data_buf, sizeof(data_buf),
                                               PortNum::TRACEROUTE, payload, n,
                                               false, ok_to_mqtt_, request_id);
    if (!data_len) return 0;
    // Like a receipt, this is a unicast: a peer holding our public key rejects one that
    // is merely channel-encrypted, so take the PKC path whenever it is open.
    if (const size_t len = build_pki_packet(out_buf, max_len, dest, pid, false,
                                            data_buf, data_len))
        return len;
    write_header(out_buf, dest, pid, hop_limit_, false, channels_[channel_idx].hash);
    use_channel_key(channel_idx);
    uint8_t enc[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    memcpy(enc, data_buf, data_len);
    if (crypto_.has_key()) crypto_.crypt(enc, data_len, pid, local_node_id_);
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, enc, data_len);
    return PKT_HEADER_SIZE + data_len;
}

size_t MeshRouter::build_ack_tx(uint8_t* out_buf, size_t max_len, uint32_t dest, uint32_t request_id, size_t channel_idx) {
    if (max_len < PKT_HEADER_SIZE) return 0;
    if (channel_idx >= MAX_CHANNELS) channel_idx = 0;
    const uint32_t pid = next_packet_id();

    // Real Meshtastic ACK = a ROUTING Data where Data.request_id (fixed32 field 6)
    // echoes the acknowledged packet's id; the Routing payload carries error_reason
    // (NONE for an ACK), which serialises to empty, so we leave the payload empty.
    // (The old format put request_id inside the Routing protobuf - a real node's
    // sender never matched it, so delivery status never turned green.)
    DecodedData d;
    d.portnum = PortNum::ROUTING;
    d.request_id = request_id;
    d.payload_len = 0;

    uint8_t data_buf[PKT_MAX_SIZE - PKT_HEADER_SIZE];
    size_t data_len = d.encode(data_buf, sizeof(data_buf));
    // A receipt is a direct message like any other, so it needs the same end-to-end
    // encryption; a channel-encrypted one never reaches a peer that knows our key.
    if (const size_t n = build_pki_packet(out_buf, max_len, dest, pid, false,
                                          data_buf, data_len))
        return n;
    write_header(out_buf, dest, pid, hop_limit_, false, channels_[channel_idx].hash);
    use_channel_key(channel_idx);
    if (crypto_.has_key()) {
        crypto_.crypt(data_buf, data_len, pid, local_node_id_);
    }
    if (PKT_HEADER_SIZE + data_len > max_len) return 0;
    memcpy(out_buf + PKT_HEADER_SIZE, data_buf, data_len);
    return PKT_HEADER_SIZE + data_len;
}

}  // namespace meshtastic
