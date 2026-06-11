/*
 * Copyright (C) 2026
 *
 * K=7 R=1/2 convolutional encoder (SatDump `CCEncoder` / cc_encoder.cpp semantics).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_CC_ENCODER_HPP
#define METEOR_LRPT_METEOR_CC_ENCODER_HPP

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

class MeteorCcEncoder {
   public:
    explicit MeteorCcEncoder(unsigned frame_bits);
    MeteorCcEncoder(const MeteorCcEncoder&) = delete;
    MeteorCcEncoder& operator=(const MeteorCcEncoder&) = delete;

    /** Each `in_bits[i]` uses LSB only; produces `2 * frame_bits_` bytes 0/1 in `out_syms`. */
    void work(const uint8_t* in_bits, uint8_t* out_syms);

    void reset_register() { shift_reg_ = 0; }

   private:
    static int parity(uint32_t x);
    static uint32_t abs_poly(int p) { return (uint32_t)(p < 0 ? -p : p); }

    unsigned frame_bits_{0};
    uint32_t shift_reg_{0};
    static constexpr int d_polys[2]{79, 109};
};

}  // namespace meteor_lrpt

#endif
