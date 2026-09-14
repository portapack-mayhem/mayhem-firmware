/*
 * Copyright (C) 2026 Alexey Verhogladov
 *
 * This file is part of PortaPack.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mesh_protocol.hpp"
#include <cstring>
#include <algorithm>

namespace meshtastic {

// ---- DecodedData -----------------------------------------------------------

// The port numbers Meshtastic actually uses. A frame that decrypted to noise usually
// yields a plausible-looking varint here, and accepting it was how a corrupted packet
// became a "new node" in the list and got re-flooded to the mesh.
// Is this one of the port numbers Meshtastic has actually assigned?
//
// This is the test for "did the decryption work". A packet decrypted with the wrong key
// is rubbish, but rubbish often parses as a well-formed protobuf: the lengths happen to
// agree and the fields read out. What it cannot fake is the port number, which is drawn
// from a sparse set of assigned values - so an arbitrary one is almost certainly noise.
// Without this check every failed decryption looked like a valid packet.
//
// The assigned values fall into runs, which is all this needs to know. Named anchors,
// for orientation: 1 text, 3 position, 4 nodeinfo, 5 routing, 67 telemetry,
// 70 traceroute, 71 neighbourinfo. 11 and 75 are deliberately not in the set.
//
// NB: PortNum is a uint8_t, so a port number above 255 cannot survive the cast that
// produced `p` - 256 (private) arrives as 0 and 257 as 1. The two cases below are kept
// because the set is the protocol's, not ours, but they are unreachable as things stand.
static bool known_portnum(PortNum p) {
    const uint32_t v = static_cast<uint32_t>(p);
    return v <= 10 || v == 12 ||
           (v >= 32 && v <= 35) ||
           (v >= 64 && v <= 74) ||
           (v >= 76 && v <= 78) ||
           v == 256 || v == 257;
}

bool DecodedData::decode(const uint8_t* buf, size_t len, DecodedData& out) {
    out = {};
    size_t i = 0;
    bool saw_portnum = false;
    bool truncated = false;
    while (i < len) {
        uint64_t tag_raw;
        size_t consumed = decode_varint(buf + i, len - i, tag_raw);
        if (!consumed) break;
        i += consumed;

        const uint8_t field_num = static_cast<uint8_t>(tag_raw >> 3);
        const uint8_t wire_type = static_cast<uint8_t>(tag_raw & 0x07);

        if (wire_type == 0) {  // varint
            uint64_t val;
            consumed = decode_varint(buf + i, len - i, val);
            if (!consumed) break;
            i += consumed;
            if (field_num == 1) {
                out.portnum = static_cast<PortNum>(val);
                saw_portnum = true;
            }
            if (field_num == 3) out.want_response = (val != 0);
            if (field_num == 5) out.source = static_cast<uint32_t>(val);
        } else if (wire_type == 2) {  // length-delimited
            uint64_t field_len;
            consumed = decode_varint(buf + i, len - i, field_len);
            if (!consumed) break;
            i += consumed;
            // A long packet truncated by the M4 ring leaves this final field short of
            // its declared length: copy the bytes we actually have (a partial NodeInfo
            // still yields the peer name, since long_name is an early field). For a
            // normal full packet field_len <= avail, so behaviour is unchanged.
            const size_t avail = len - i;
            const size_t take = (field_len > avail) ? avail : static_cast<size_t>(field_len);
            if (field_num == 2 && take <= sizeof(out.payload)) {
                memcpy(out.payload, buf + i, take);
                out.payload_len = static_cast<uint8_t>(take);
            }
            if (field_len > avail) {
                truncated = true;
                break;
            }  // nothing decodable after
            i += static_cast<size_t>(field_len);
        } else if (wire_type == 5) {  // fixed32 (dest=4, source=5, request_id=6, ...)
            if (len - i < 4) break;
            const uint32_t v = buf[i] | (buf[i + 1] << 8) | (buf[i + 2] << 16) | (static_cast<uint32_t>(buf[i + 3]) << 24);
            i += 4;
            if (field_num == 5) out.source = v;      // real Meshtastic source is fixed32
            if (field_num == 6) out.request_id = v;  // ACK/NAK target (delivery status)
        } else {
            break;  // unknown wire type -> stop
        }
    }
    // Accept only what parsed as a real Data message: a port number we recognise, and
    // every byte accounted for - unless the packet was cut short by the receive ring,
    // which is a truncation we deliberately support (a partial NodeInfo still names its
    // sender). Without this every decryption failure looked like a valid packet.
    return saw_portnum && known_portnum(out.portnum) && (truncated || i >= len);
}

size_t DecodedData::encode(uint8_t* buf, size_t max_len) const {
    size_t i = 0;
    // field 1 (portnum): tag = (1<<3)|0 = 0x08
    if (i + 2 >= max_len) return 0;
    buf[i++] = 0x08;
    i += encode_varint(buf + i, static_cast<uint64_t>(portnum));
    // field 2 (payload): tag = (2<<3)|2 = 0x12
    if (payload_len > 0 && i + 2 + payload_len < max_len) {
        buf[i++] = 0x12;
        i += encode_varint(buf + i, payload_len);
        memcpy(buf + i, payload, payload_len);
        i += payload_len;
    }
    // field 3 (want_response): tag = 0x18
    if (want_response && i + 2 < max_len) {
        buf[i++] = 0x18;
        buf[i++] = 0x01;
    }
    // field 6 (request_id): fixed32, tag = (6<<3)|5 = 0x35.  Set on a ROUTING ACK
    // so the original sender can match it to the message it sent.
    if (request_id != 0 && i + 5 <= max_len) {
        buf[i++] = 0x35;
        buf[i++] = static_cast<uint8_t>(request_id);
        buf[i++] = static_cast<uint8_t>(request_id >> 8);
        buf[i++] = static_cast<uint8_t>(request_id >> 16);
        buf[i++] = static_cast<uint8_t>(request_id >> 24);
    }
    // field 9 (bitfield): varint, tag = (9<<3)|0 = 0x48.  bit 0 = ok_to_mqtt.
    if (ok_to_mqtt && i + 2 <= max_len) {
        buf[i++] = 0x48;
        buf[i++] = 0x01;
    }
    return i;
}

// ---- PositionData ----------------------------------------------------------
// Meshtastic Position protobuf (mesh.proto, wire types matter):
//   1: latitude_i   (sfixed32, degrees * 1e7)  - wire type 5
//   2: longitude_i  (sfixed32, degrees * 1e7)  - wire type 5
//   3: altitude     (int32 varint, meters)
//   4: time         (fixed32, unix timestamp)  - wire type 5
//  19: sats_in_view (uint32 varint)

static uint32_t read_le32(const uint8_t* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

// Used by more than one decoder now, so it sits with read_le32 rather than beside
// the first one that happened to need it.
static float read_float_le(const uint8_t* p) {
    uint32_t b = read_le32(p);
    float f;
    memcpy(&f, &b, sizeof(f));
    return f;
}

static void write_le32(uint8_t* p, uint32_t v) {
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

bool NeighborInfoData::decode(const uint8_t* buf, size_t len, NeighborInfoData& out, uint32_t our_id) {
    out = {};
    size_t i = 0;
    while (i < len) {
        uint64_t tag;
        size_t c = decode_varint(buf + i, len - i, tag);
        if (!c) break;
        i += c;
        const uint8_t fn = static_cast<uint8_t>(tag >> 3), wt = static_cast<uint8_t>(tag & 7);
        if (wt == 0) {
            uint64_t v;
            c = decode_varint(buf + i, len - i, v);
            if (!c) break;
            i += c;
            if (fn == 1)
                out.node_id = static_cast<uint32_t>(v);
            else if (fn == 2)
                out.last_sent_by_id = static_cast<uint32_t>(v);
            else if (fn == 3)
                out.interval_secs = static_cast<uint32_t>(v);
        } else if (wt == 2) {
            uint64_t sl;
            c = decode_varint(buf + i, len - i, sl);
            if (!c) break;
            i += c;
            if (sl > len - i) break;
            if (fn == 4) {  // one Neighbor
                out.count++;
                const uint8_t* nb = buf + i;
                size_t j = 0, n = static_cast<size_t>(sl);
                uint32_t id = 0;
                float snr = 0.0f;
                bool have_snr = false;
                while (j < n) {
                    uint64_t nt;
                    size_t nc = decode_varint(nb + j, n - j, nt);
                    if (!nc) break;
                    j += nc;
                    const uint8_t nfn = static_cast<uint8_t>(nt >> 3),
                                  nwt = static_cast<uint8_t>(nt & 7);
                    if (nwt == 0) {
                        uint64_t nv;
                        nc = decode_varint(nb + j, n - j, nv);
                        if (!nc) break;
                        j += nc;
                        if (nfn == 1) id = static_cast<uint32_t>(nv);
                    } else if (nwt == 5) {
                        if (n - j < 4) break;
                        if (nfn == 2) {
                            snr = read_float_le(nb + j);
                            have_snr = true;
                        }
                        j += 4;
                    } else if (nwt == 2) {
                        uint64_t xl;
                        nc = decode_varint(nb + j, n - j, xl);
                        if (!nc) break;
                        j += nc;
                        if (xl > n - j) break;
                        j += static_cast<size_t>(xl);
                    } else
                        break;
                }
                // The entry that names us is the interesting one: it says what this
                // node hears of US.
                if (id && id == our_id && have_snr) {
                    out.has_us = true;
                    out.snr_to_us = snr;
                }
            }
            i += static_cast<size_t>(sl);
        } else if (wt == 5) {
            if (len - i < 4) break;
            i += 4;
        } else if (wt == 1) {
            if (len - i < 8) break;
            i += 8;
        } else
            break;
    }
    return out.node_id != 0 || out.count > 0;
}

bool PositionData::decode(const uint8_t* buf, size_t len, PositionData& out) {
    out = {};
    size_t i = 0;
    while (i < len) {
        uint64_t tag_raw;
        size_t consumed = decode_varint(buf + i, len - i, tag_raw);
        if (!consumed) break;
        i += consumed;
        const uint8_t field_num = static_cast<uint8_t>(tag_raw >> 3);
        const uint8_t wire_type = static_cast<uint8_t>(tag_raw & 0x07);
        if (wire_type == 0) {
            uint64_t val;
            consumed = decode_varint(buf + i, len - i, val);
            if (!consumed) break;
            i += consumed;
            if (field_num == 3) {
                out.altitude_m = static_cast<int32_t>(val);
            } else if (field_num == 19) {
                out.satellites_in_view = static_cast<uint8_t>(val);
            }
        } else if (wire_type == 5) {
            if (len - i < 4) break;
            const uint32_t val = read_le32(buf + i);
            i += 4;
            if (field_num == 1) {
                out.latitude = static_cast<int32_t>(val) * 1e-7;
            } else if (field_num == 2) {
                out.longitude = static_cast<int32_t>(val) * 1e-7;
            } else if (field_num == 4) {
                out.timestamp = val;
            }
        } else if (wire_type == 2) {
            uint64_t field_len;
            consumed = decode_varint(buf + i, len - i, field_len);
            if (!consumed) break;
            i += consumed;
            if (field_len > len - i) break;  // overflow-safe skip of an unknown field
            i += field_len;
        } else if (wire_type == 1) {
            if (len - i < 8) break;  // skip unknown 64-bit field
            i += 8;
        } else {
            break;
        }
    }
    out.valid = (out.latitude != 0.0 || out.longitude != 0.0);
    return out.valid;
}

size_t PositionData::encode(uint8_t* buf, size_t max_len) const {
    if (max_len < 22) return 0;  // worst case below
    size_t i = 0;
    // field 1: latitude_i (sfixed32)
    buf[i++] = 0x0D;
    write_le32(buf + i, static_cast<uint32_t>(static_cast<int32_t>(latitude * 1e7)));
    i += 4;
    // field 2: longitude_i (sfixed32)
    buf[i++] = 0x15;
    write_le32(buf + i, static_cast<uint32_t>(static_cast<int32_t>(longitude * 1e7)));
    i += 4;
    // field 3: altitude (int32 varint; negatives sign-extend to 64-bit per proto)
    if (altitude_m != 0) {
        buf[i++] = 0x18;
        i += encode_varint(buf + i, static_cast<uint64_t>(static_cast<int64_t>(altitude_m)));
    }
    // field 4: time (fixed32)
    if (timestamp != 0) {
        buf[i++] = 0x25;
        write_le32(buf + i, timestamp);
        i += 4;
    }
    // field 19: sats_in_view (two-byte tag: (19<<3)|0 = 0x98 0x01)
    if (satellites_in_view != 0) {
        buf[i++] = 0x98;
        buf[i++] = 0x01;
        i += encode_varint(buf + i, satellites_in_view);
    }
    return i;
}

// ---- TelemetryData ---------------------------------------------------------
// Meshtastic Telemetry protobuf: time(1,fixed32), device_metrics(2),
// environment_metrics(3). Sub-messages are length-delimited.
// Append the user-supplied metrics belonging to this variant. Everything in
// telemetry.proto that the device has no sensor for goes out this way, so the wire
// format supports the whole message set without a widget per field.
static size_t encode_extras(uint8_t* buf, size_t max_len, TelemetryData::Variant variant, const TelemetryData::ExtraMetric* extras, size_t count) {
    size_t n = 0;
    for (size_t k = 0; k < count; k++) {
        const auto& x = extras[k];
        if (x.variant != variant) continue;
        if (n + 6 > max_len) break;
        if (x.is_float) {
            buf[n++] = static_cast<uint8_t>((x.field << 3) | 5);
            n += encode_float(buf + n, x.value);
        } else {
            buf[n++] = static_cast<uint8_t>(x.field << 3);
            n += encode_varint(buf + n, static_cast<uint32_t>(x.value));
        }
    }
    return n;
}

size_t TelemetryData::encode(uint8_t* buf, size_t max_len, Variant variant, const ExtraMetric* extras, size_t extra_count) const {
    size_t i = 0;
    // field 1: time (fixed32)
    if (timestamp != 0) {
        buf[i++] = 0x0D;
        i += encode_fixed32(buf + i, timestamp);
    }
    if (variant == DEVICE) {
        // field 2: device_metrics (battery/voltage are gated by the user's Off toggles)
        uint8_t d[48];
        size_t n = 0;
        if (send_battery) {
            d[n++] = 0x08;  // battery_level (uint32)
            n += encode_varint(d + n, battery_level);
        }
        if (send_voltage) {
            d[n++] = 0x15;  // voltage (float)
            n += encode_float(d + n, voltage);
        }
        if (send_util) {
            d[n++] = 0x1D;  // channel_utilization (float, 3)
            n += encode_float(d + n, channel_utilization < 0 ? 0.0f : channel_utilization);
            d[n++] = 0x25;  // air_util_tx (float, field 4)
            n += encode_float(d + n, air_util_tx < 0 ? 0.0f : air_util_tx);
        }
        if (send_uptime) {
            d[n++] = 0x28;  // uptime_seconds (uint32, field 5)
            n += encode_varint(d + n, uptime_seconds);
        }
        n += encode_extras(d + n, sizeof(d) - n, DEVICE, extras, extra_count);
        buf[i++] = 0x12;  // field 2, len-delimited
        i += encode_varint(buf + i, n);
        memcpy(buf + i, d, n);
        i += n;
    } else if (variant == ENVIRONMENT) {
        // field 3: environment_metrics. Twenty of the Sensors entries land here, and
        // with the three real readings in front of them the worst case is 108 bytes -
        // more than the 96 this buffer used to hold. encode_extras stops cleanly when
        // it runs out, so nothing overflowed, but the last few metrics the user had
        // ticked were dropped without a word. Sized for the whole table now.
        uint8_t e[176];
        size_t n = 0;
        if (send_env_base) {
            e[n++] = 0x0D;
            n += encode_float(e + n, temperature);  // field 1
            e[n++] = 0x15;
            n += encode_float(e + n, relative_humidity);  // field 2
            e[n++] = 0x1D;
            n += encode_float(e + n, barometric_pressure);  // field 3
        }
        n += encode_extras(e + n, sizeof(e) - n, ENVIRONMENT, extras, extra_count);
        buf[i++] = 0x1A;  // field 3, len-delimited
        i += encode_varint(buf + i, n);
        memcpy(buf + i, e, n);
        i += n;
    } else if (variant == AIR_QUALITY) {
        // field 4: air_quality_metrics. Field 2 = pm25_standard, field 3 =
        // pm100_standard (PM10), field 13 = co2; all plain varints.
        // Every air-quality figure now comes from the user's own table, so nothing is
        // written here unconditionally - two sources for the same field would have put
        // it on the wire twice.
        uint8_t a[64];
        size_t n = encode_extras(a, sizeof(a), AIR_QUALITY, extras, extra_count);
        if (n) {
            buf[i++] = 0x22;  // field 4, len-delimited
            i += encode_varint(buf + i, n);
            memcpy(buf + i, a, n);
            i += n;
        }
    } else if (variant == LOCAL_STATS) {
        // field 6: local_stats. Meshtastic never puts this on the air by itself - the
        // telemetry module hands it to an attached phone and otherwise sends it only in
        // answer to a request (DeviceTelemetryModule::allocReply). Fields the device
        // genuinely does not measure are left out rather than sent as zero, so the
        // phone shows a blank instead of a confident lie.
        uint8_t d[64];
        size_t n = 0;
        if (send_uptime) {
            d[n++] = 0x08;
            n += encode_varint(d + n, stat_uptime);
        }  // 1
        if (channel_utilization >= 0.0f) {
            d[n++] = 0x15;
            n += encode_float(d + n, channel_utilization);  // 2
        }
        if (air_util_tx >= 0.0f) {
            d[n++] = 0x1D;
            n += encode_float(d + n, air_util_tx);  // 3
        }
        d[n++] = 0x20;
        n += encode_varint(d + n, packets_tx);  // 4
        d[n++] = 0x28;
        n += encode_varint(d + n, packets_rx);  // 5
        d[n++] = 0x30;
        n += encode_varint(d + n, packets_rx_bad);  // 6
        d[n++] = 0x38;
        n += encode_varint(d + n, nodes_online);  // 7
        d[n++] = 0x40;
        n += encode_varint(d + n, nodes_total);  // 8
        d[n++] = 0x48;
        n += encode_varint(d + n, rx_dupe);  // 9
        d[n++] = 0x50;
        n += encode_varint(d + n, tx_relay);  // 10
        d[n++] = 0x58;
        n += encode_varint(d + n, tx_relay_canceled);  // 11
        if (heap_total) {
            d[n++] = 0x60;
            n += encode_varint(d + n, heap_total);  // 12
            d[n++] = 0x68;
            n += encode_varint(d + n, heap_free);  // 13
        }
        d[n++] = 0x70;
        n += encode_varint(d + n, tx_dropped);  // 14
        buf[i++] = 0x32;                        // field 6, len-delimited
        i += encode_varint(buf + i, n);
        memcpy(buf + i, d, n);
        i += n;
    } else if (variant == POWER || variant == HEALTH) {
        // Nothing on this device measures either, so both are built entirely from the
        // user's own values - a solar/battery rig or a wearable someone is emulating.
        uint8_t m[64];
        const size_t n = encode_extras(m, sizeof(m), variant, extras, extra_count);
        if (n) {
            buf[i++] = (variant == POWER) ? 0x2A : 0x3A;  // field 5 / field 7
            i += encode_varint(buf + i, n);
            memcpy(buf + i, m, n);
            i += n;
        }
    }
    (void)max_len;
    return i;
}

bool TelemetryData::decode(const uint8_t* buf, size_t len, TelemetryData& out) {
    out = {};
    size_t i = 0;
    while (i < len) {
        uint64_t tag;
        size_t c = decode_varint(buf + i, len - i, tag);
        if (!c) break;
        i += c;
        const uint8_t fn = tag >> 3, wt = tag & 7;
        if (wt == 0) {  // varint
            uint64_t v;
            c = decode_varint(buf + i, len - i, v);
            if (!c) break;
            i += c;
            if (fn == 1) out.timestamp = static_cast<uint32_t>(v);
        } else if (wt == 5) {  // fixed32
            if (len - i < 4) break;
            if (fn == 1) out.timestamp = read_le32(buf + i);
            i += 4;
        } else if (wt == 2) {  // sub-message
            uint64_t sl;
            c = decode_varint(buf + i, len - i, sl);
            if (!c) break;
            i += c;
            if (sl > len - i) break;
            const uint8_t* s = buf + i;
            size_t n = sl, j = 0;
            while (j < n) {
                uint64_t st;
                size_t sc = decode_varint(s + j, n - j, st);
                if (!sc) break;
                j += sc;
                const uint8_t sfn = st >> 3, swt = st & 7;
                if (swt == 0) {
                    uint64_t sv;
                    sc = decode_varint(s + j, n - j, sv);
                    if (!sc) break;
                    j += sc;
                    const uint32_t u = static_cast<uint32_t>(sv);
                    if (fn == 2) {  // device metrics
                        if (sfn == 1)
                            out.battery_level = static_cast<uint8_t>(sv);
                        else if (sfn == 5)
                            out.uptime_seconds = u;
                    } else if (fn == 3) {  // environment metrics
                        if (sfn == 7)
                            out.iaq = u;
                        else if (sfn == 13)
                            out.wind_direction = u;
                        else if (sfn == 21)
                            out.soil_moisture = u;
                    } else if (fn == 4) {  // air quality
                        // Names follow the proto: pm10_standard is PM1.0 and
                        // pm100_standard is PM10. Our pm25/pm10 members are the two
                        // the phone app shows (PM2.5 and PM10).
                        switch (sfn) {
                            case 1:
                                out.pm1_std = static_cast<uint16_t>(u);
                                break;
                            case 2:
                                out.pm25 = static_cast<uint16_t>(u);
                                break;
                            case 3:
                                out.pm10 = static_cast<uint16_t>(u);
                                break;
                            case 4:
                                out.pm10_env = static_cast<uint16_t>(u);
                                break;
                            case 5:
                                out.pm25_env = static_cast<uint16_t>(u);
                                break;
                            case 6:
                                out.pm100_env = static_cast<uint16_t>(u);
                                break;
                            case 7:
                                out.particles_03 = u;
                                break;
                            case 8:
                                out.particles_05 = u;
                                break;
                            case 9:
                                out.particles_10 = u;
                                break;
                            case 10:
                                out.particles_25 = u;
                                break;
                            case 11:
                                out.particles_50 = u;
                                break;
                            case 12:
                                out.particles_100 = u;
                                break;
                            case 13:
                                out.co2 = static_cast<uint16_t>(u);
                                break;
                            case 19:
                                out.pm40_std = static_cast<uint16_t>(u);
                                break;
                            case 20:
                                out.particles_40 = u;
                                break;
                            default:
                                break;
                        }
                    } else if (fn == 5) {  // power metrics (varints: none)
                    } else if (fn == 6) {  // local stats
                        switch (sfn) {
                            case 1:
                                out.stat_uptime = u;
                                break;
                            case 4:
                                out.packets_tx = u;
                                break;
                            case 5:
                                out.packets_rx = u;
                                break;
                            case 6:
                                out.packets_rx_bad = u;
                                break;
                            case 7:
                                out.nodes_online = u;
                                break;
                            case 8:
                                out.nodes_total = u;
                                break;
                            case 9:
                                out.rx_dupe = u;
                                break;
                            case 10:
                                out.tx_relay = u;
                                break;
                            case 11:
                                out.tx_relay_canceled = u;
                                break;
                            case 12:
                                out.heap_total = u;
                                break;
                            case 13:
                                out.heap_free = u;
                                break;
                            case 14:
                                out.tx_dropped = u;
                                break;
                            case 15:
                                out.noise_floor = static_cast<int32_t>(
                                    (sv & 1) ? -static_cast<int64_t>((sv + 1) >> 1)
                                             : static_cast<int64_t>(sv >> 1));
                                out.has_noise = true;
                                break;
                            default:
                                break;
                        }
                    } else if (fn == 7) {  // health metrics
                        if (sfn == 1)
                            out.heart_bpm = u;
                        else if (sfn == 2)
                            out.spo2 = u;
                    } else if (fn == 8) {  // host metrics (Linux nodes)
                        switch (sfn) {
                            case 1:
                                out.host_uptime = u;
                                break;
                            case 2:
                                out.freemem = u;
                                break;
                            case 3:
                                out.diskfree1 = u;
                                break;
                            case 4:
                                out.diskfree2 = u;
                                break;
                            case 5:
                                out.diskfree3 = u;
                                break;
                            case 6:
                                out.load1 = u;
                                break;
                            case 7:
                                out.load5 = u;
                                break;
                            case 8:
                                out.load15 = u;
                                break;
                            default:
                                break;
                        }
                    } else if (fn == 9) {  // traffic management stats
                        switch (sfn) {
                            case 1:
                                out.packets_inspected = u;
                                break;
                            case 2:
                                out.position_dedup_drops = u;
                                break;
                            case 3:
                                out.nodeinfo_cache_hits = u;
                                break;
                            case 4:
                                out.rate_limit_drops = u;
                                break;
                            case 5:
                                out.unknown_packet_drops = u;
                                break;
                            case 6:
                                out.hop_exhausted = u;
                                break;
                            case 7:
                                out.router_hops_preserved = u;
                                break;
                            default:
                                break;
                        }
                    }
                } else if (swt == 5) {
                    if (n - j < 4) break;
                    const float f = read_float_le(s + j);
                    if (fn == 2) {
                        if (sfn == 2)
                            out.voltage = f;
                        else if (sfn == 3)
                            out.channel_utilization = f;
                        else if (sfn == 4)
                            out.air_util_tx = f;
                    } else if (fn == 3) {
                        switch (sfn) {
                            case 1:
                                out.temperature = f;
                                break;
                            case 2:
                                out.relative_humidity = f;
                                break;
                            case 3:
                                out.barometric_pressure = f;
                                break;
                            case 4:
                                out.gas_resistance = f;
                                break;
                            case 5:
                                out.env_voltage = f;
                                break;
                            case 6:
                                out.env_current = f;
                                break;
                            case 8:
                                out.distance = f;
                                break;
                            case 9:
                                out.lux = f;
                                break;
                            case 10:
                                out.white_lux = f;
                                break;
                            case 11:
                                out.ir_lux = f;
                                break;
                            case 12:
                                out.uv_lux = f;
                                break;
                            case 14:
                                out.wind_speed = f;
                                break;
                            case 15:
                                out.weight = f;
                                break;
                            case 16:
                                out.wind_gust = f;
                                break;
                            case 17:
                                out.wind_lull = f;
                                break;
                            case 18:
                                out.radiation = f;
                                break;
                            case 19:
                                out.rainfall_1h = f;
                                break;
                            case 20:
                                out.rainfall_24h = f;
                                break;
                            case 22:
                                out.soil_temperature = f;
                                break;
                            case 23:
                                out.one_wire_temperature = f;
                                break;
                            default:
                                break;
                        }
                    } else if (fn == 4) {
                        switch (sfn) {
                            case 14:
                                out.co2_temperature = f;
                                break;
                            case 15:
                                out.co2_humidity = f;
                                break;
                            case 16:
                                out.formaldehyde = f;
                                break;
                            case 17:
                                out.form_humidity = f;
                                break;
                            case 18:
                                out.form_temperature = f;
                                break;
                            case 21:
                                out.pm_temperature = f;
                                break;
                            case 22:
                                out.pm_humidity = f;
                                break;
                            case 23:
                                out.pm_voc_idx = f;
                                break;
                            case 24:
                                out.pm_nox_idx = f;
                                break;
                            case 25:
                                out.particles_tps = f;
                                break;
                            default:
                                break;
                        }
                    } else if (fn == 5) {  // power: v/i per channel
                        if (sfn >= 1 && sfn <= 16) {
                            const uint8_t ch = static_cast<uint8_t>((sfn - 1) / 2);
                            if ((sfn & 1) != 0)
                                out.ch_voltage[ch] = f;
                            else
                                out.ch_current[ch] = f;
                        }
                    } else if (fn == 6) {
                        if (sfn == 2)
                            out.channel_utilization = f;
                        else if (sfn == 3)
                            out.air_util_tx = f;
                    } else if (fn == 7) {
                        if (sfn == 3) out.body_temperature = f;
                    }
                    j += 4;
                } else if (swt == 2) {
                    uint64_t l;
                    sc = decode_varint(s + j, n - j, l);
                    if (!sc) break;
                    j += sc;
                    if (l > n - j) break;
                    j += l;
                } else
                    break;
            }
            if (fn == 2)
                out.has_device = true;
            else if (fn == 3)
                out.has_env = true;
            else if (fn == 4)
                out.has_air = true;
            else if (fn == 5)
                out.has_power = true;
            else if (fn == 6)
                out.has_stats = true;
            else if (fn == 7)
                out.has_health = true;
            else if (fn == 8)
                out.has_host = true;
            else if (fn == 9)
                out.has_traffic = true;
            i += sl;
        } else
            break;
    }
    return out.has_device || out.has_env || out.has_air || out.has_stats ||
           out.has_power || out.has_health || out.has_host || out.has_traffic;
}

// ---- NodeInfoData ----------------------------------------------------------
// Meshtastic User protobuf (mesh.proto):
// field 1: id (string "!hexid" - NOT the numeric node id)
// field 2: long_name (string)
// field 3: short_name (string)
// field 4: macaddr (bytes, deprecated)
// field 5: hw_model (enum varint - which board the peer runs)
// field 6: is_licensed (bool) - not used here
// field 7: role (enum varint - client, router, sensor, tracker, ...)
// field 9: is_unmessagable (bool) - nobody is reading messages on this node
// The numeric node id comes from the packet header, so field 1 is skipped.

bool NodeInfoData::decode(const uint8_t* buf, size_t len, NodeInfoData& out) {
    out = {};
    size_t i = 0;
    while (i < len) {
        uint64_t tag_raw;
        size_t consumed = decode_varint(buf + i, len - i, tag_raw);
        if (!consumed) break;
        i += consumed;
        const uint8_t field_num = static_cast<uint8_t>(tag_raw >> 3);
        const uint8_t wire_type = static_cast<uint8_t>(tag_raw & 0x07);
        if (wire_type == 0) {
            uint64_t val;
            consumed = decode_varint(buf + i, len - i, val);
            if (!consumed) break;
            i += consumed;
            if (field_num == 1)
                out.node_id = static_cast<uint32_t>(val);
            else if (field_num == 5)
                out.hw_model = static_cast<uint8_t>(val);
            // Role is field SEVEN. We encoded it there all along but read it from six,
            // which is is_licensed - so every peer came out looking like a plain client.
            else if (field_num == 7)
                out.role = static_cast<uint8_t>(val);
            else if (field_num == 9)
                out.unmessagable = (val != 0);
        } else if (wire_type == 2) {
            uint64_t field_len;
            consumed = decode_varint(buf + i, len - i, field_len);
            if (!consumed) break;
            i += consumed;
            if (field_len > len - i) break;  // overflow-safe (i < len here)
            if (field_num == 2 && field_len < sizeof(out.long_name)) {
                memcpy(out.long_name, buf + i, field_len);
                out.long_name[field_len] = '\0';
            } else if (field_num == 3 && field_len < sizeof(out.short_name)) {
                memcpy(out.short_name, buf + i, field_len);
                out.short_name[field_len] = '\0';
            } else if (field_num == 8 && field_len == sizeof(out.public_key)) {
                memcpy(out.public_key, buf + i, sizeof(out.public_key));
                out.has_pubkey = true;
            }
            i += field_len;
        } else {
            break;
        }
    }
    return true;
}

// ---- Channel sharing URL ---------------------------------------------------

static void b64url_append(std::string& out, const uint8_t* data, size_t len) {
    static const char* A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        const uint32_t v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += A[(v >> 18) & 63];
        out += A[(v >> 12) & 63];
        out += A[(v >> 6) & 63];
        out += A[v & 63];
    }
    if (i < len) {  // 1 or 2 bytes left; Meshtastic drops the padding
        const uint32_t v = (data[i] << 16) | ((i + 1 < len ? data[i + 1] : 0) << 8);
        out += A[(v >> 18) & 63];
        out += A[(v >> 12) & 63];
        if (i + 1 < len) out += A[(v >> 6) & 63];
    }
}

// One ChannelSettings entry, appended to buf. Returns the new length.
static size_t append_channel(uint8_t* buf, size_t n, const char* name, const uint8_t* psk, size_t psk_len) {
    // ChannelSettings: field 2 = psk (bytes), field 3 = name (string).
    if (psk && psk_len && psk_len <= 32) {
        buf[n++] = 0x12;
        buf[n++] = static_cast<uint8_t>(psk_len);
        memcpy(buf + n, psk, psk_len);
        n += psk_len;
    }
    const size_t name_len = name ? std::min(strlen(name), size_t{12}) : 0;
    if (name_len) {
        buf[n++] = 0x1A;
        buf[n++] = static_cast<uint8_t>(name_len);
        memcpy(buf + n, name, name_len);
        n += name_len;
    }
    return n;
}

std::string channel_share_url(const ChannelShare& s) {
    // ChannelSet: field 1 = settings (repeated ChannelSettings), field 2 = lora_config.
    // The repeated entries land on channel indices 0, 1, 2... in the order they appear,
    // so a shared secondary must be preceded by the primary it lives beside.
    uint8_t set[192];
    size_t k = 0;
    uint8_t ch[64];

    if (s.primary_name) {
        const size_t n = append_channel(ch, 0, s.primary_name, s.primary_psk, s.primary_psk_len);
        set[k++] = 0x0A;
        k += encode_varint(set + k, n);
        memcpy(set + k, ch, n);
        k += n;
    }
    {
        const size_t n = append_channel(ch, 0, s.name, s.psk, s.psk_len);
        set[k++] = 0x0A;
        k += encode_varint(set + k, n);
        memcpy(set + k, ch, n);
        k += n;
    }

    // LoRaConfig: 1 = use_preset, 2 = modem_preset, 3 = bandwidth (kHz),
    // 4 = spread_factor, 5 = coding_rate, 7 = region, 11 = channel_num.
    uint8_t lc[32];
    size_t m = 0;
    lc[m++] = 0x08;
    lc[m++] = 1;
    lc[m++] = 0x10;
    m += encode_varint(lc + m, s.modem_preset);
    if (s.bw_hz) {
        lc[m++] = 0x18;
        m += encode_varint(lc + m, s.bw_hz / 1000);
    }
    if (s.sf) {
        lc[m++] = 0x20;
        m += encode_varint(lc + m, s.sf);
    }
    if (s.cr) {
        lc[m++] = 0x28;
        m += encode_varint(lc + m, s.cr);
    }
    lc[m++] = 0x38;
    m += encode_varint(lc + m, s.region_code);
    if (s.channel_num) {
        lc[m++] = 0x58;
        m += encode_varint(lc + m, s.channel_num);
    }

    set[k++] = 0x12;
    k += encode_varint(set + k, m);
    memcpy(set + k, lc, m);
    k += m;

    std::string url = "https://meshtastic.org/e/#";
    b64url_append(url, set, k);
    return url;
}

std::string base64_encode(const uint8_t* data, size_t len) {
    static const char* A = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        const uint32_t v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += A[(v >> 18) & 63];
        out += A[(v >> 12) & 63];
        out += A[(v >> 6) & 63];
        out += A[v & 63];
    }
    if (i < len) {
        const uint32_t v = (data[i] << 16) | ((i + 1 < len ? data[i + 1] : 0) << 8);
        out += A[(v >> 18) & 63];
        out += A[(v >> 12) & 63];
        out += (i + 1 < len) ? A[(v >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

std::string contact_share_url(uint32_t node_id, const char* long_name, const char* short_name, const uint8_t* pubkey, bool has_pubkey) {
    // User: 1 = id ("!hex"), 2 = long_name, 3 = short_name, 8 = public_key.
    uint8_t u[96];
    size_t n = 0;
    u[n++] = 0x0A;
    u[n++] = 9;
    u[n++] = '!';
    for (int k = 28; k >= 0; k -= 4)
        u[n++] = "0123456789abcdef"[(node_id >> k) & 0xF];
    const size_t ln = long_name ? std::min(strlen(long_name), size_t{16}) : 0;
    if (ln) {
        u[n++] = 0x12;
        u[n++] = static_cast<uint8_t>(ln);
        memcpy(u + n, long_name, ln);
        n += ln;
    }
    const size_t sn = short_name ? std::min(strlen(short_name), size_t{4}) : 0;
    if (sn) {
        u[n++] = 0x1A;
        u[n++] = static_cast<uint8_t>(sn);
        memcpy(u + n, short_name, sn);
        n += sn;
    }
    if (has_pubkey && pubkey) {
        u[n++] = 0x42;
        u[n++] = 32;
        memcpy(u + n, pubkey, 32);
        n += 32;
    }

    // SharedContact: 1 = node_num (uint32), 2 = user.
    uint8_t c[128];
    size_t k = 0;
    c[k++] = 0x08;
    k += encode_varint(c + k, node_id);
    c[k++] = 0x12;
    k += encode_varint(c + k, n);
    memcpy(c + k, u, n);
    k += n;

    std::string url = "https://meshtastic.org/v/#";
    b64url_append(url, c, k);
    return url;
}

}  // namespace meshtastic
