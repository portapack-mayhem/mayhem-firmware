/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_cc_encoder.hpp"

namespace meteor_lrpt {

int MeteorCcEncoder::parity(uint32_t x) {
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (int)(x & 1U);
}

MeteorCcEncoder::MeteorCcEncoder(unsigned frame_bits)
    : frame_bits_{frame_bits} {
}

void MeteorCcEncoder::work(const uint8_t* in_bits, uint8_t* out_syms) {
    uint32_t st = shift_reg_;
    for (unsigned i = 0; i < frame_bits_; i++) {
        st = (st << 1) | (uint32_t)(in_bits[i] & 1U);
        for (unsigned j = 0; j < 2u; j++) {
            const int poly = d_polys[j];
            const uint32_t ap = abs_poly(poly);
            const int bit = parity(st & ap) ^ (poly < 0 ? 1 : 0);
            out_syms[i * 2u + j] = (uint8_t)(bit & 1);
        }
    }
    shift_reg_ = st;
}

}  // namespace meteor_lrpt
