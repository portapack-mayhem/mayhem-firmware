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

/*
 * Minimal unaligned-PER decoder for the Horus Binary v3 "Telemetry" type
 * (horusdemodlib/horusbinaryv3/HorusBinaryV3.asn1), plus the fixed v1/v2 layouts.
 * Integer only, no heap. Custom REAL values are kept as mantissa and binary exponent.
 */

#ifndef __HORUS_V3_H__
#define __HORUS_V3_H__

#include <cstdint>
#include <cstddef>

namespace ui::external_app::horus_rx {

constexpr int kMaxExtra = 4;
constexpr int kMaxValues = 4;

enum class ExtraKind : uint8_t {
    None = 0,
    Str,
    Int,
    Real,
    Bool
};

struct ExtraValue {
    int32_t mant;  // Int: value. Real: value = mant * 2^exp. Bool: bit flags.
    int16_t exp;
};

struct ExtraSensor {
    char name[21];
    ExtraKind kind;
    uint8_t count;
    ExtraValue v[kMaxValues];
    char str[24];  // Str values, truncated
};

struct Telemetry {
    uint8_t version;  // 1, 2 or 3
    char callsign[16];
    uint16_t payload_id;  // v1/v2 numeric payload ID
    uint16_t sequence;
    int32_t time_of_day;  // seconds, -1 = invalid
    int32_t lat_e5;       // degrees * 1e5
    int32_t lon_e5;
    int32_t alt_m;

    bool has_speed, has_sats, has_ascent, has_pressure, has_humidity;
    uint16_t speed_kmh;
    uint8_t sats;
    int16_t ascent_cms;
    uint16_t pressure_x10;
    uint8_t humidity;

    // tenths of degC: internal, external, custom1, custom2
    bool has_temp[4];
    int16_t temp_x10[4];

    // millivolts: battery, solar, custom1, custom2
    bool has_mv[4];
    uint16_t mv[4];

    uint8_t extra_count;
    ExtraSensor extra[kMaxExtra];
};

// Decode a CRC-checked payload (as delivered by the framer, CRC included).
// Returns false if the content does not parse.
bool decode_payload(const uint8_t* data, size_t size, uint8_t version, Telemetry& out);

}  // namespace ui::external_app::horus_rx

#endif /*__HORUS_V3_H__*/
