/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#include "ambe_fec.h"
#include <string.h>

static const uint8_t a_pos[24] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 1, 5, 9, 13, 17, 21};
static const uint8_t b_pos[23] = {25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42};
static const uint8_t c_pos[25] = {46, 50, 54, 58, 62, 66, 70, 3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43, 47, 51, 55, 59, 63, 67, 71};
static const uint16_t parity_rows[12] = {0x8eb, 0x93e, 0xa97, 0xdc6, 0x367, 0x6cd, 0xd99, 0x3da, 0x7b4, 0xf68, 0x63b, 0xc75};
static const uint16_t inverse_rows[12] = {0xc75, 0x49f, 0x93e, 0x6e3, 0xdc6, 0xf13, 0xab9, 0x1ed, 0x3da, 0x7b4, 0xf68, 0xa4f};

static uint16_t transform(uint16_t value, const uint16_t* rows) {
    uint16_t result = 0;

    for (unsigned i = 0; i < 12; ++i)
        if (value & (1u << i)) result ^= rows[i];

    return result;
}

uint32_t dmr_ambe_golay24(uint16_t data) {
    return ((uint32_t)(data & 4095u) << 12) | transform(data, parity_rows);
}

uint32_t dmr_ambe_scramble(uint16_t data) {
    uint32_t state = data * 16u, result = 0;

    for (unsigned i = 0; i < 24; ++i) {
        state = (173u * state + 13849u) & 65535u;
        result = (result << 1) | (state >> 15);
    }

    return result;
}

static int decode24(uint32_t received, uint16_t* data) {
    uint16_t d = received >> 12;
    uint16_t syndrome = (received & 4095u) ^ transform(d, parity_rows);

    if (__builtin_popcount(syndrome) <= 3) {
        *data = d;
        return 1;
    }

    for (unsigned i = 0; i < 12; ++i)
        if (__builtin_popcount(syndrome ^ parity_rows[i]) <= 2) {
            *data = d ^ (1u << i);
            return 1;
        }

    syndrome = transform(syndrome, inverse_rows);

    if (__builtin_popcount(syndrome) <= 3) {
        *data = d ^ syndrome;
        return 1;
    }

    for (unsigned i = 0; i < 12; ++i)
        if (__builtin_popcount(syndrome ^ inverse_rows[i]) <= 2) {
            *data = d ^ syndrome ^ inverse_rows[i];
            return 1;
        }

    return 0;
}

static uint32_t unpack(const uint8_t* frame, const uint8_t* positions, unsigned n) {
    uint32_t value = 0;

    for (unsigned i = 0; i < n; ++i)
        value = (value << 1) | ((frame[positions[i] / 8] >> (7 - positions[i] % 8)) & 1u);

    return value;
}

int dmr_ambe_decode(const uint8_t channel[9], uint8_t codec[7]) {
    uint32_t a = unpack(channel, a_pos, 24), b = unpack(channel, b_pos, 23);
    uint32_t c = unpack(channel, c_pos, 25);
    uint16_t da, db;

    if (!decode24(a, &da)) return -1;

    b ^= dmr_ambe_scramble(da) >> 1;

    if (!decode24(b << 1, &db) && !decode24((b << 1) | 1u, &db)) return -1;
    memset(codec, 0, 7);

    uint64_t bits = ((uint64_t)da << 37) | ((uint64_t)db << 25) | c;

    for (unsigned i = 0; i < 49; ++i)
        codec[i / 8] |= ((bits >> (48 - i)) & 1u) << (7 - i % 8);

    return __builtin_popcount(a ^ dmr_ambe_golay24(da)) +
           __builtin_popcount(b ^ (dmr_ambe_golay24(db) >> 1));
}
