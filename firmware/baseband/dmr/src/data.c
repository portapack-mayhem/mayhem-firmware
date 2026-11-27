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

#include <dmr/data.h>
#include <string.h>

static unsigned int popcount32(uint32_t value) {
    unsigned int count = 0;

    while (value) {
        value &= value - 1u;
        ++count;
    }

    return count;
}

static uint32_t slot_codeword(unsigned int value) {
    static const uint32_t rows[8] = {
        0x800f6u, 0x40d99u, 0x206cdu, 0x10367u,
        0x08dc6u, 0x04a97u, 0x0293eu, 0x018ebu};

    uint32_t word = 0;

    for (unsigned int i = 0; i < 8u; ++i)
        if ((value >> (7u - i)) & 1u)
            word ^= rows[i];
    return word;
}

int dmr_slot_type_decode(uint32_t received, unsigned int* color_code, unsigned int* data_type, unsigned int* corrected_bits) {
    if (received > 0xfffffu || !color_code || !data_type)
        return 0;

    unsigned int best_value = 0, best_distance = 21u;

    for (unsigned int value = 0; value < 256u; ++value) {
        const unsigned int distance = popcount32(
            received ^ slot_codeword(value));

        if (distance < best_distance) {
            best_distance = distance;
            best_value = value;
        }
    }

    if (best_distance > 3u)
        return 0;

    *color_code = best_value >> 4;
    *data_type = best_value & 15u;

    if (corrected_bits)
        *corrected_bits = best_distance;

    return 1;
}

static uint8_t get_bit(const uint8_t* bytes, unsigned int bit) {
    return (bytes[bit / 8u] >> (7u - bit % 8u)) & 1u;
}

static void set_bit(uint8_t* bytes, unsigned int bit, uint8_t value) {
    const uint8_t mask = (uint8_t)(0x80u >> (bit % 8u));

    if (value)
        bytes[bit / 8u] |= mask;
    else
        bytes[bit / 8u] &= (uint8_t)~mask;
}

int dmr_bptc_196_96_decode(const uint8_t encoded[25],
                           uint8_t information[12]) {
    if (!encoded || !information)
        return 0;

    uint8_t matrix[196] = {0};

    for (unsigned int i = 0; i < 196u; ++i)
        matrix[i] = get_bit(encoded, (i * 181u) % 196u);

    memset(information, 0, 12u);

    unsigned int output = 0;

    for (unsigned int i = 4u; i < 12u; ++i)
        set_bit(information, output++, matrix[i]);

    for (unsigned int row = 1u; row < 9u; ++row)
        for (unsigned int col = 0; col < 11u; ++col)
            set_bit(information, output++, matrix[row * 15u + 1u + col]);

    return 1;
}

static uint8_t gf_multiply(uint8_t a, uint8_t b) {
    uint8_t result = 0;

    while (b) {
        if (b & 1u) result ^= a;

        const int high = a & 0x80u;

        a <<= 1;

        if (high) a ^= 0x1du;

        b >>= 1;
    }
    return result;
}

static void rs_12_9(const uint8_t lc[9], uint8_t parity[3]) {
    static const uint8_t coefficients[9][3] = {
        {0x1c, 0xbc, 0xfd}, {0x89, 0x31, 0x08}, {0xad, 0x41, 0x36}, {0x7d, 0x71, 0x16}, {0xf3, 0xa6, 0x3a}, {0x08, 0x83, 0x7b}, {0x3f, 0x6f, 0x02}, {0x6c, 0x0d, 0xa7}, {0x0e, 0x38, 0x40}};

    memset(parity, 0, 3u);

    for (unsigned int i = 0; i < 9u; ++i)
        for (unsigned int p = 0; p < 3u; ++p)
            parity[p] ^= gf_multiply(lc[i], coefficients[i][p]);
}

static int extract_data_fields(const dmr_burst_t* burst, uint8_t encoded[25], unsigned int* color_code, unsigned int* data_type) {
    uint32_t slot = 0;

    for (unsigned int i = 0; i < 10u; ++i)
        slot = (slot << 1) | dmr_burst_get_bit(burst->data, 98u + i);

    for (unsigned int i = 0; i < 10u; ++i)
        slot = (slot << 1) | dmr_burst_get_bit(burst->data, 156u + i);

    if (!dmr_slot_type_decode(slot, color_code, data_type, NULL))
        return 0;

    memset(encoded, 0, 25u);

    for (unsigned int i = 0; i < 98u; ++i)
        set_bit(encoded, i, dmr_burst_get_bit(burst->data, i));

    for (unsigned int i = 0; i < 98u; ++i)
        set_bit(encoded, 98u + i,
                dmr_burst_get_bit(burst->data, 166u + i));
    return 1;
}

int dmr_full_lc_decode(const dmr_burst_t* burst, uint8_t lc[9], unsigned int* color_code, unsigned int* data_type) {
    if (!burst || !lc || !color_code || !data_type)
        return 0;

    uint8_t encoded[25], information[12], expected[3];

    if (!extract_data_fields(burst, encoded, color_code, data_type) ||
        (*data_type != DMR_DATA_TYPE_VOICE_LC_HEADER &&
         *data_type != DMR_DATA_TYPE_TERMINATOR_LC) ||
        !dmr_bptc_196_96_decode(encoded, information))
        return 0;

    memcpy(lc, information, 9u);
    rs_12_9(lc, expected);

    const uint8_t mask = *data_type == DMR_DATA_TYPE_VOICE_LC_HEADER
                             ? 0x96u
                             : 0x99u;

    for (unsigned int i = 0; i < 3u; ++i)
        if ((uint8_t)(information[9u + i] ^ mask) != expected[i])
            return 0;

    return 1;
}
