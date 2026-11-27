/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

#include "dab_common.hpp"

namespace dab {

/* ---- CRC-16 and firecode ---- */

namespace {

constexpr uint16_t kCcittPoly = 0x1021;
constexpr uint16_t kFirecodePoly = 0x782F;

uint16_t crc_bitwise(const uint8_t* data, size_t length, uint16_t crc, uint16_t poly) {
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ poly) : static_cast<uint16_t>(crc << 1);
    }
    return crc;
}

}  // namespace

uint16_t crc16_ccitt(const uint8_t* data, size_t length) {
    return crc_bitwise(data, length, 0xFFFF, kCcittPoly);
}

bool fib_crc_ok(const uint8_t fib[32]) {
    const uint16_t computed = static_cast<uint16_t>(crc16_ccitt(fib, kFibPayloadBytes) ^ 0xFFFF);
    const uint16_t stored = static_cast<uint16_t>((fib[30] << 8) | fib[31]);
    return computed == stored;
}

void fib_crc_stamp(uint8_t fib[32]) {
    const uint16_t computed = static_cast<uint16_t>(crc16_ccitt(fib, kFibPayloadBytes) ^ 0xFFFF);
    fib[30] = static_cast<uint8_t>(computed >> 8);
    fib[31] = static_cast<uint8_t>(computed & 0xFF);
}

bool firecode_ok(const uint8_t superframe_head[11]) {
    const uint16_t computed = crc_bitwise(&superframe_head[2], 9, 0x0000, kFirecodePoly);
    const uint16_t stored = static_cast<uint16_t>((superframe_head[0] << 8) | superframe_head[1]);
    return computed == stored;
}

void firecode_stamp(uint8_t superframe_head[11]) {
    const uint16_t computed = crc_bitwise(&superframe_head[2], 9, 0x0000, kFirecodePoly);
    superframe_head[0] = static_cast<uint8_t>(computed >> 8);
    superframe_head[1] = static_cast<uint8_t>(computed & 0xFF);
}

/* ---- Protection profiles and depuncturing ---- */

namespace {

constexpr uint16_t kEepACuPerUnit[4] = {12, 8, 6, 4};
constexpr uint16_t kEepBCuPerUnit[4] = {27, 21, 18, 15};

constexpr uint32_t kEepAUnitBitrate = 8000;
constexpr uint32_t kEepBUnitBitrate = 32000;

}  // namespace

uint16_t eep_cu_per_unit(const ProtectionProfile& profile) {
    if (profile.level < 1 || profile.level > 4)
        return 0;
    const size_t idx = profile.level - 1;
    return profile.option_b ? kEepBCuPerUnit[idx] : kEepACuPerUnit[idx];
}

uint32_t eep_bitrate(uint16_t size_cu, const ProtectionProfile& profile) {
    const uint16_t per_unit = eep_cu_per_unit(profile);
    if (per_unit == 0 || size_cu == 0 || (size_cu % per_unit) != 0)
        return 0;

    const uint32_t units = size_cu / per_unit;
    return units * (profile.option_b ? kEepBUnitBitrate : kEepAUnitBitrate);
}

uint32_t eep_subchannel_bits(uint16_t size_cu, const ProtectionProfile& profile) {
    const uint32_t bitrate = eep_bitrate(size_cu, profile);
    if (bitrate == 0)
        return 0;

    return (bitrate * kFrameDurationMs) / (1000 * kCifsPerFrame);
}

/* EN 300 401 table 29, stored not generated: the standard upgrades the eight
 * groups in bit-reversed order (0, 4, 2, 6, 1, 5, 3, 7), which a simple
 * generator gets wrong for 15 of 24 while every weight still checks out. */
uint32_t puncturing_vector(uint8_t index) {
    if (index < 1 || index > 24)
        return 0;

    static constexpr uint32_t kPuncturing[24] = {
        0xC8888888u,
        0xC888C888u,
        0xC8C8C888u,
        0xC8C8C8C8u,
        0xCCC8C8C8u,
        0xCCC8CCC8u,
        0xCCCCCCC8u,
        0xCCCCCCCCu,
        0xECCCCCCCu,
        0xECCCECCCu,
        0xECECECCCu,
        0xECECECECu,
        0xEEECECECu,
        0xEEECEEECu,
        0xEEEEEEECu,
        0xEEEEEEEEu,
        0xFEEEEEEEu,
        0xFEEEFEEEu,
        0xFEFEFEEEu,
        0xFEFEFEFEu,
        0xFFFEFEFEu,
        0xFFFEFFFEu,
        0xFFFFFFFEu,
        0xFFFFFFFFu,
    };
    return kPuncturing[index - 1];
}

uint8_t puncturing_weight(uint8_t index) {
    const uint32_t v = puncturing_vector(index);
    uint8_t weight = 0;
    for (int bit = 0; bit < 32; ++bit)
        weight = static_cast<uint8_t>(weight + ((v >> bit) & 1));
    return weight;
}

size_t depuncture(const int8_t* punctured,
                  size_t punctured_length,
                  uint32_t pi_vector,
                  int8_t* out,
                  size_t out_capacity) {
    if (punctured == nullptr || out == nullptr || pi_vector == 0)
        return 0;

    size_t taken = 0;
    size_t written = 0;

    while (taken < punctured_length && written < out_capacity) {
        const size_t bit_in_vector = written % 32;
        const bool kept = ((pi_vector >> (31 - bit_in_vector)) & 1) != 0;
        out[written++] = kept ? punctured[taken++] : 0;
    }

    /* Pad a partial group with erasures so the decoder sees whole symbols. */
    while ((written % 4) != 0 && written < out_capacity)
        out[written++] = 0;

    return written;
}

} /* namespace dab */
