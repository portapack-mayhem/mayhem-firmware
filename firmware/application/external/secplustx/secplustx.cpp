/*
 * Copyright 2022 Clayton Smith (argilo@gmail.com)
 *
 * This file is part of secplus.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "secplustx.hpp"

namespace ui::external_app::ui_secplustx {

static void v2_calc_parity(const uint64_t fixed, uint32_t* data) {
    uint32_t parity = (fixed >> 32) & 0xf;
    int8_t offset;

    *data &= 0xffff0fff;
    for (offset = 0; offset < 32; offset += 4) {
        parity ^= ((*data >> offset) & 0xf);
    }
    *data |= (parity << 12);
}

static void encode_v2_rolling(const uint32_t rolling,
                              uint32_t* rolling_halves) {
    uint32_t rolling_reversed = 0;
    int8_t i, half;

    for (i = 0; i < 28; i++) {
        rolling_reversed |= ((rolling >> i) & 1) << (28 - i - 1);
    }

    rolling_halves[0] = 0;
    rolling_halves[1] = 0;

    for (half = 0; half < 2; half++) {
        for (i = 0; i < 8; i += 2) {
            rolling_halves[half] |= rolling_reversed % 3 << i;
            rolling_reversed /= 3;
        }
    }

    for (half = 0; half < 2; half++) {
        for (i = 10; i < 18; i += 2) {
            rolling_halves[half] |= rolling_reversed % 3 << i;
            rolling_reversed /= 3;
        }
    }

    rolling_halves[0] |= (rolling_reversed % 3) << 8;
    rolling_reversed /= 3;

    rolling_halves[1] |= (rolling_reversed % 3) << 8;
}

static const int8_t ORDER[16] = {9, 33, 6, -1, 24, 18, 36, -1,
                                 24, 36, 6, -1, -1, -1, -1, -1};
static const int8_t INVERT[16] = {6, 2, 1, -1, 7, 5, 3, -1,
                                  4, 0, 5, -1, -1, -1, -1, -1};

static void v2_scramble(const uint32_t* parts, const uint8_t frame_type, uint8_t* packet_half) {
    const int8_t order = ORDER[packet_half[0] >> 4];
    const int8_t invert = INVERT[packet_half[0] & 0xf];
    int8_t i;
    uint8_t out_offset = 10;
    int8_t end;
    uint32_t parts_permuted[3];

    end = (frame_type == 0 ? 5 : 8);
    for (i = 1; i < end; i++) {
        packet_half[i] = 0;
    }

    parts_permuted[0] =
        (invert & 4) ? ~parts[(order >> 4) & 3] : parts[(order >> 4) & 3];
    parts_permuted[1] =
        (invert & 2) ? ~parts[(order >> 2) & 3] : parts[(order >> 2) & 3];
    parts_permuted[2] = (invert & 1) ? ~parts[order & 3] : parts[order & 3];

    end = (frame_type == 0 ? 8 : 0);
    for (i = 18 - 1; i >= end; i--) {
        packet_half[out_offset >> 3] |= ((parts_permuted[0] >> i) & 1)
                                        << (7 - (out_offset % 8));
        out_offset++;
        packet_half[out_offset >> 3] |= ((parts_permuted[1] >> i) & 1)
                                        << (7 - (out_offset % 8));
        out_offset++;
        packet_half[out_offset >> 3] |= ((parts_permuted[2] >> i) & 1)
                                        << (7 - (out_offset % 8));
        out_offset++;
    }
}

static void encode_v2_half_parts(const uint32_t rolling, const uint32_t fixed, const uint16_t data, const uint8_t frame_type, uint8_t* packet_half) {
    uint32_t parts[3];

    parts[0] = ((fixed >> 10) << 8) | (data >> 8);
    parts[1] = ((fixed & 0x3ff) << 8) | (data & 0xff);
    parts[2] = rolling;

    packet_half[0] = (uint8_t)rolling;

    v2_scramble(parts, frame_type, packet_half);
}

static int8_t v2_check_limits(const uint32_t rolling, const uint64_t fixed) {
    if ((rolling >> 28) != 0) {
        return -1;
    }

    if ((fixed >> 40) != 0) {
        return -1;
    }

    return 0;
}

static void encode_v2_half(const uint32_t rolling, const uint32_t fixed, const uint16_t data, const uint8_t frame_type, uint8_t* packet_half) {
    encode_v2_half_parts(rolling, fixed, data, frame_type, packet_half);

    /* shift indicator two bits to the right */
    packet_half[1] |= (packet_half[0] & 0x3) << 6;
    packet_half[0] >>= 2;

    /* set frame type */
    packet_half[0] |= (frame_type << 6);
}

int8_t encode_v2(const uint32_t rolling, const uint64_t fixed, uint32_t data, const uint8_t frame_type, uint8_t* packet1, uint8_t* packet2) {
    int8_t err = 0;
    uint32_t rolling_halves[2];

    err = v2_check_limits(rolling, fixed);
    if (err < 0) {
        return err;
    }

    encode_v2_rolling(rolling, rolling_halves);
    v2_calc_parity(fixed, &data);

    encode_v2_half(rolling_halves[0], fixed >> 20, data >> 16, frame_type,
                   packet1);
    encode_v2_half(rolling_halves[1], fixed & 0xfffff, data & 0xffff, frame_type,
                   packet2);

    return 0;
}

};  // namespace ui::external_app::ui_secplustx
