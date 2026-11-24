/*
 * Copyright (C) 2026 OE1LBS
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "horus_v3.hpp"

#include <cstring>

namespace ui::external_app::horus_rx {

namespace {

// Sorted (by character code) permitted alphabets, as UPER indexes them.
constexpr char kCallsignAlphabet[] = "-/0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
constexpr char kNameAlphabet[] = "-0123456789abcdefghijklmnopqrstuvwxyz";
constexpr char kStrAlphabet[] = " +-./0123456789=ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

class BitReader {
   public:
    BitReader(const uint8_t* p, size_t n)
        : p_{p}, nbits_{n * 8} {}

    bool ok() const { return ok_; }
    size_t pos() const { return pos_; }

    uint32_t bits(int n) {
        uint32_t v = 0;
        for (int i = 0; i < n; i++) {
            if (pos_ >= nbits_) {
                ok_ = false;
                return 0;
            }
            v = (v << 1) | ((p_[pos_ / 8] >> (7 - (pos_ % 8))) & 1u);
            pos_++;
        }
        return v;
    }
    bool bit() { return bits(1) != 0; }

    // Constrained whole number in lo..hi.
    int32_t constrained(int32_t lo, int32_t hi) {
        const uint32_t range = (uint32_t)(hi - lo);
        int n = 0;
        while (n < 32 && (range >> n) != 0) n++;
        const int32_t v = lo + (int32_t)bits(n);
        if (v > hi) ok_ = false;
        return v;
    }

    // Unaligned PER general length determinant (forms up to 16383).
    uint32_t length() {
        if (!bit()) return bits(7);
        if (!bit()) return bits(14);
        ok_ = false;  // fragmented form: never used by Horus
        return 0;
    }

    // Unconstrained INTEGER: length in octets, then two's complement.
    int32_t unconstrained() {
        const uint32_t len = length();
        if (len == 0 || len > 4) {
            ok_ = false;
            return 0;
        }
        uint32_t v = bits(8 * len);
        if (len < 4 && (v & (1u << (8 * len - 1)))) v |= ~0u << (8 * len);
        return (int32_t)v;
    }

    void skip(size_t n) { pos_ += n; }

   private:
    const uint8_t* p_;
    size_t nbits_;
    size_t pos_ = 0;
    bool ok_ = true;
};

void read_indexed_string(BitReader& r, const char* alphabet, int bits_per_char, int len, char* out, size_t out_size) {
    const int alen = (int)strlen(alphabet);
    size_t o = 0;
    for (int i = 0; i < len; i++) {
        const uint32_t idx = r.bits(bits_per_char);
        const char c = (idx < (uint32_t)alen) ? alphabet[idx] : '?';
        if (o + 1 < out_size) out[o++] = c;
    }
    out[o] = '\0';
}

// REAL contents (X.690 binary or special forms) into mant * 2^exp.
void read_real(BitReader& r, ExtraValue& v) {
    const uint32_t len = r.length();
    v.mant = 0;
    v.exp = 0;
    if (len == 0) return;  // +0
    const uint32_t first = r.bits(8);
    if (!(first & 0x80)) {  // decimal or special values: not representable here
        r.skip(8 * (len - 1));
        return;
    }
    const bool neg = first & 0x40;
    const int base_shift = ((first >> 4) & 3) == 1 ? 3 : (((first >> 4) & 3) == 2 ? 4 : 1);
    const int scale = (first >> 2) & 3;
    const int elen_code = first & 3;
    uint32_t used = 1;
    int elen = elen_code + 1;
    if (elen_code == 3) {
        elen = (int)r.bits(8);
        used++;
    }
    if (elen > 4) {
        r.skip(8 * (len - used));
        return;
    }
    int32_t e = (int32_t)r.bits(8 * elen);
    if (elen < 4 && (e & (1 << (8 * elen - 1)))) e |= ~0u << (8 * elen);
    used += elen;
    // value = mantissa * 2^(e * log2(base) + scale); keep up to 31 mantissa bits
    int32_t e2 = e * base_shift + scale;
    uint32_t m = 0;
    for (; used < len; used++) {
        const uint32_t b = r.bits(8);
        if (m >> 23) {
            e2 += 8;  // dropped a low byte
            continue;
        }
        m = (m << 8) | b;
    }
    v.mant = neg ? -(int32_t)m : (int32_t)m;
    v.exp = (int16_t)e2;
}

void decode_extra(BitReader& r, ExtraSensor& s) {
    memset(&s, 0, sizeof(s));
    const bool has_name = r.bit();
    const bool has_values = r.bit();
    if (has_name) {
        const int len = r.constrained(1, 20);
        read_indexed_string(r, kNameAlphabet, 6, len, s.name, sizeof(s.name));
    }
    if (!has_values) return;
    const uint32_t choice = r.bits(2);
    switch (choice) {
        case 0: {  // horusStr: 7-bit index into the sorted alphabet (as asn1tools encodes it)
            s.kind = ExtraKind::Str;
            const int len = r.constrained(0, 255);
            read_indexed_string(r, kStrAlphabet, 7, len, s.str, sizeof(s.str));
            break;
        }
        case 1:
            s.kind = ExtraKind::Int;
            s.count = (uint8_t)r.constrained(1, 4);
            for (int i = 0; i < s.count; i++) s.v[i].mant = r.unconstrained();
            break;
        case 2:
            s.kind = ExtraKind::Real;
            s.count = (uint8_t)r.constrained(1, 4);
            for (int i = 0; i < s.count; i++) read_real(r, s.v[i]);
            break;
        default:
            s.kind = ExtraKind::Bool;
            s.count = 1;
            s.v[0].mant = (int32_t)r.bits(8);
            break;
    }
}

bool decode_v3(const uint8_t* data, size_t size, Telemetry& t) {
    BitReader r{data + 2, size - 2};  // skip the CRC
    const bool ext = r.bit();
    bool opt[11];
    for (auto& o : opt) o = r.bit();

    const int cs_len = r.constrained(1, 15);
    read_indexed_string(r, kCallsignAlphabet, 6, cs_len, t.callsign, sizeof(t.callsign));
    t.sequence = (uint16_t)r.constrained(0, 65535);
    t.time_of_day = r.constrained(-1, 86400);
    t.lat_e5 = r.constrained(-9000000, 9000000);
    t.lon_e5 = r.constrained(-18000000, 18000000);
    t.alt_m = r.constrained(-1000, 50000);

    if (opt[0]) {
        const int n = r.constrained(1, 4);
        for (int i = 0; i < n; i++) {
            ExtraSensor tmp;
            decode_extra(r, tmp);
            if (t.extra_count < kMaxExtra) t.extra[t.extra_count++] = tmp;
        }
    }
    if ((t.has_speed = opt[1])) t.speed_kmh = (uint16_t)r.constrained(0, 512);
    if ((t.has_sats = opt[2])) t.sats = (uint8_t)r.constrained(0, 31);
    if ((t.has_ascent = opt[3])) t.ascent_cms = (int16_t)r.constrained(-32767, 32767);
    if ((t.has_pressure = opt[4])) t.pressure_x10 = (uint16_t)r.constrained(0, 12000);
    if (opt[5]) {
        bool p[4];
        for (auto& b : p) b = r.bit();
        for (int i = 0; i < 4; i++)
            if ((t.has_temp[i] = p[i])) t.temp_x10[i] = (int16_t)r.constrained(-1023, 1023);
    }
    if ((t.has_humidity = opt[6])) t.humidity = (uint8_t)r.constrained(0, 100);
    if (opt[7]) {
        bool p[4];
        for (auto& b : p) b = r.bit();
        for (int i = 0; i < 4; i++)
            if ((t.has_mv[i] = p[i])) t.mv[i] = (uint16_t)r.constrained(0, 16383);
    }
    if (opt[8]) {  // counts: parsed and skipped
        const int n = r.constrained(1, 8);
        for (int i = 0; i < n; i++) r.unconstrained();
    }
    if (opt[9]) r.constrained(0, 5);  // gnssPowerSaveState
    if (opt[10]) {                    // customData
        const int n = r.constrained(0, 255);
        r.skip(8 * n);
    }
    (void)ext;  // extension additions (e.g. "via") are not needed for display
    return r.ok();
}

int32_t float_e5(const uint8_t* p) {
    // little-endian IEEE754 float to degrees * 1e5 (v1/v2 only; soft float on the M0)
    float f;
    memcpy(&f, p, 4);
    return (int32_t)(f * 100000.0f);
}

}  // namespace

bool decode_payload(const uint8_t* d, size_t size, uint8_t version, Telemetry& t) {
    memset(&t, 0, sizeof(t));
    t.version = version;
    t.time_of_day = -1;

    if (version == 3) return decode_v3(d, size, t);

    if (version == 1 && size == 22) {
        t.payload_id = d[0];
        t.sequence = (uint16_t)(d[1] | (d[2] << 8));
        t.time_of_day = d[3] * 3600 + d[4] * 60 + d[5];
        t.lat_e5 = float_e5(d + 6);
        t.lon_e5 = float_e5(d + 10);
        t.alt_m = d[14] | (d[15] << 8);
        t.has_speed = true;
        t.speed_kmh = d[16];
        t.has_sats = true;
        t.sats = d[17];
        t.has_temp[0] = true;
        t.temp_x10[0] = (int16_t)((int8_t)d[18] * 10);
        t.has_mv[0] = true;
        t.mv[0] = (uint16_t)(d[19] * 5000u / 255u);
        return true;
    }
    if (version == 2 && size == 32) {
        t.payload_id = (uint16_t)(d[0] | (d[1] << 8));
        t.sequence = (uint16_t)(d[2] | (d[3] << 8));
        t.time_of_day = d[4] * 3600 + d[5] * 60 + d[6];
        t.lat_e5 = float_e5(d + 7);
        t.lon_e5 = float_e5(d + 11);
        t.alt_m = d[15] | (d[16] << 8);
        t.has_speed = true;
        t.speed_kmh = d[17];
        t.has_sats = true;
        t.sats = d[18];
        t.has_temp[0] = true;
        t.temp_x10[0] = (int16_t)((int8_t)d[19] * 10);
        t.has_mv[0] = true;
        t.mv[0] = (uint16_t)(d[20] * 5000u / 255u);
        return true;
    }
    return false;
}

}  // namespace ui::external_app::horus_rx
