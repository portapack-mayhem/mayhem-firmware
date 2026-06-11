/*
 * Copyright (C) 2026
 *
 * SatDump `viterbi::Viterbi1_2` port (see SATDUMP_VENDOR.md). Uses `MeteorCcDecoder` + `MeteorCcEncoder`.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_METEOR_VITERBI12_HPP
#define METEOR_LRPT_METEOR_VITERBI12_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#include "meteor_cc_decoder.hpp"
#include "meteor_cc_encoder.hpp"

namespace meteor_lrpt {

class MeteorViterbi12 {
   public:
    MeteorViterbi12(
        float ber_threshold,
        int max_outsync,
        int buffer_size,
        bool check_iq_swap);
    MeteorViterbi12(const MeteorViterbi12&) = delete;
    MeteorViterbi12& operator=(const MeteorViterbi12&) = delete;

    /** `input`/`size` int8 soft (I/Q pairs); `output` holds decoded packed bits (512 B for size 8192). */
    int work(int8_t* input, int size, uint8_t* output);

    float ber() const;
    int getState() const { return d_state_; }
    bool getShift() const { return d_shift_; }

   private:
    static constexpr int ST_IDLE = 0;
    static constexpr int ST_SYNCED = 1;
    static constexpr int kTestBitsLength = 2048;

    static float get_ber(const uint8_t* raw_soft, const uint8_t* reencoded_bits, int len);
    static void signed_soft_to_unsigned(const int8_t* in, uint8_t* out, size_t n);

    float d_ber_threshold_;
    int d_max_outsync_;
    bool d_check_iq_swap_;
    int d_buffer_size_;

    int d_state_{ST_IDLE};
    bool d_iq_swap_{false};
    unsigned d_phase_{0};
    bool d_shift_{false};
    int d_invalid_{0};
    float d_ber_{10.0f};
    float d_bers[2][4][2]{};

    MeteorCcDecoder cc_decoder_ber_{1024};
    MeteorCcDecoder cc_decoder_main_{4096};
    MeteorCcEncoder cc_encoder_ber_{1024};
    MeteorCcEncoder cc_encoder_main_{4096};

    std::array<int8_t, kTestBitsLength> ber_test_{};
    /* +1: BER path indexes ber_soft_[i + shift] for i in [0,kTestBitsLength) and shift in {0,1}. */
    std::array<uint8_t, kTestBitsLength + 1u> ber_soft_{};
    std::array<uint8_t, 256> ber_decoded_{};
    std::array<uint8_t, kTestBitsLength> ber_encoded_{};
    std::array<uint8_t, 8192> main_encoded_{};
    std::array<uint8_t, 16384> soft_u_{};
};

}  // namespace meteor_lrpt

#endif
