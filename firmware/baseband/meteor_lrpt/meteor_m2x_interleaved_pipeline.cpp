/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_m2x_interleaved_pipeline.hpp"

#include <algorithm>
#include <cstring>
#include <new>

namespace meteor_lrpt {

M2xInterleavedPostDeintPipeline::M2xInterleavedPostDeintPipeline()
    : vit0_{0.17f, 20, 8192, true},
      vit1_{0.17f, 20, 8192, true} {
}

void M2xInterleavedPostDeintPipeline::reset() {
    vit0_.~MeteorViterbi12();
    new (&vit0_) MeteorViterbi12(0.17f, 20, 8192, true);
    vit1_.~MeteorViterbi12();
    new (&vit1_) MeteorViterbi12(0.17f, 20, 8192, true);
    nrzm_bits_.reset();
    def_.reset();
    work0_.fill(0);
    work1_.fill(0);
    vout0_.fill(0);
    vout1_.fill(0);
    m2x_bits_.fill(0);
    def_batch_.fill(0);
    last_viterbi_winner_ = 0;
}

bool M2xInterleavedPostDeintPipeline::process(
    const int8_t* stream_a8192,
    const int8_t* stream_b8192,
    bool diff_nrzm,
    std::array<uint8_t, 1024>& frame_out) {
    last_viterbi_winner_ = 0;
    std::memcpy(work0_.data(), stream_a8192, 8192);
    std::memcpy(work1_.data(), stream_b8192, 8192);

    const int n0 = vit0_.work(work0_.data(), 8192, vout0_.data());
    const int n1 = vit1_.work(work1_.data(), 8192, vout1_.data());
    if (n0 <= 0 || n1 <= 0)
        return false;

    const uint8_t* win = vout0_.data();
    last_viterbi_winner_ = 0;
    if (vit1_.getState() > vit0_.getState()) {
        win = vout1_.data();
        last_viterbi_winner_ = 1;
    }

    const unsigned win_n = (unsigned)std::min(n0, n1);
    size_t bi = 0;
    for (size_t i = 0; i < win_n; i++) {
        const uint8_t b = win[i];
        for (int k = 7; k >= 0; k--)
            m2x_bits_[bi++] = (uint8_t)((b >> (unsigned)k) & 1u);
    }
    const size_t nbits = win_n * 8u;
    if (nbits < 4096)
        return false;

    if (diff_nrzm)
        nrzm_bits_.decode_bits(m2x_bits_.data(), 4096);

    const int nf = def_.work(m2x_bits_.data(), 4096, def_batch_.data());
    const int max_frames = (int)(def_batch_.size() / 1024);
    for (int fi = 0; fi < nf && fi < max_frames; fi++) {
        std::memcpy(frame_out.data(), def_batch_.data() + (size_t)fi * 1024, 1024);
        return true;
    }
    return false;
}

}  // namespace meteor_lrpt
