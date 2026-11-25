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

#include "dab_crc.hpp"
#include "dab_constants.hpp"

namespace dab {

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

} /* namespace dab */
