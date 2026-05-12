/*
 * Copyright (C) 2026
 *
 * Post-deinterleave M2-x interleaved chain: dual `MeteorViterbi12`, winner mux, NRZ-M bits,
 * `MeteorBpskCcsdsDeframer` — SatDump `module_meteor_lrpt_decoder.cpp` order after dual readers.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_M2X_INTERLEAVED_PIPELINE_HPP
#define METEOR_LRPT_METEOR_M2X_INTERLEAVED_PIPELINE_HPP

#include "meteor_bpsk_ccsds_deframer.hpp"
#include "meteor_nrzm.hpp"
#include "meteor_viterbi12.hpp"

#include <array>
#include <cstdint>

namespace meteor_lrpt {

class M2xInterleavedPostDeintPipeline {
   public:
    M2xInterleavedPostDeintPipeline();

    /** Mode change / RX stop: clears Viterbi + deframer state (must not carry across sessions). */
    void reset();

    /**
     * Each stream is 8192 int8 soft bytes after `MeteorDeinterleaverReader::read_samples`.
     * Returns true if a CADU-sized frame passed ASM checks in `frame_out` (1024 bytes).
     */
    bool process(const int8_t* stream_a8192, const int8_t* stream_b8192, bool diff_nrzm, std::array<uint8_t, 1024>& frame_out);

    int bpsk_deframer_state() const { return def_.get_state(); }

    /** Valid after successful `process()`: 0 = stream A won, 1 = stream B (SatDump `getState()` compare). */
    uint8_t last_viterbi_winner() const { return last_viterbi_winner_; }
    int viterbi_state_a() const { return vit0_.getState(); }
    int viterbi_state_b() const { return vit1_.getState(); }
    float viterbi_ber_a() const { return vit0_.ber(); }
    float viterbi_ber_b() const { return vit1_.ber(); }

   private:
    MeteorViterbi12 vit0_;
    MeteorViterbi12 vit1_;
    MeteorNrzmBitDecoder nrzm_bits_{};
    MeteorBpskCcsdsDeframer def_{};
    std::array<int8_t, 8192> work0_{};
    std::array<int8_t, 8192> work1_{};
    std::array<uint8_t, 512> vout0_{};
    std::array<uint8_t, 512> vout1_{};
    std::array<uint8_t, 8192> m2x_bits_{};
    std::array<uint8_t, 2048> def_batch_{};
    uint8_t last_viterbi_winner_{0};
};

}  // namespace meteor_lrpt

#endif
