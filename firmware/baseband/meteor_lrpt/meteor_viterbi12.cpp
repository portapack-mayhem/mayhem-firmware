/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_viterbi12.hpp"

#include "meteor_soft_correlate.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace meteor_lrpt {
namespace {

void unpack_msb8(const uint8_t* packed, size_t nbytes, uint8_t* bit_lsbs) {
    for (size_t b = 0; b < nbytes; b++) {
        uint8_t v = packed[b];
        for (int j = 7; j >= 0; j--)
            *bit_lsbs++ = (uint8_t)((v >> (unsigned)j) & 1U);
    }
}

}  // namespace

/* BER scale `* 2.5f` matches SatDump `viterbi::Viterbi1_2::get_ber` (see SATDUMP_VENDOR.md). */
float MeteorViterbi12::get_ber(const uint8_t* raw_soft, const uint8_t* reencoded_bits, int len) {
    float errors = 0;
    float total = 0;
    for (int i = 0; i < len; i++) {
        if (raw_soft[i] != 128) {
            const bool soft_bit = raw_soft[i] > 127;
            const bool enc_bit = reencoded_bits[i] != 0;
            errors += (soft_bit != enc_bit) ? 1.0f : 0.0f;
            total += 1.0f;
        }
    }
    if (total <= 0.0f)
        return 10.0f;
    return (errors / total) * 2.5f;
}

void MeteorViterbi12::signed_soft_to_unsigned(const int8_t* in, uint8_t* out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int v = (int)in[i];
        if (v > 127) v = 127;
        if (v < -127) v = -127;
        out[i] = (uint8_t)(v + 128);
    }
}

MeteorViterbi12::MeteorViterbi12(
    float ber_threshold,
    int max_outsync,
    int buffer_size,
    bool check_iq_swap)
    : d_ber_threshold_{ber_threshold},
      d_max_outsync_{max_outsync},
      d_check_iq_swap_{check_iq_swap},
      d_buffer_size_{buffer_size} {
    for (int s = 0; s < 2; s++)
        for (int p = 0; p < 4; p++)
            for (int sh = 0; sh < 2; sh++)
                d_bers[s][p][sh] = 10.0f;
}

int MeteorViterbi12::work(int8_t* input, int size, uint8_t* output) {
    if (size < kTestBitsLength || size > (int)soft_u_.size())
        return 0;

    if (d_state_ == ST_IDLE) {
        d_ber_ = 10.0f;
        const int iq_max = d_check_iq_swap_ ? 2 : 1;
        for (int s = 0; s < iq_max; s++) {
            for (unsigned phase : {0u, 1u}) {
                cc_encoder_ber_.reset_register();
                std::memcpy(ber_test_.data(), input, (size_t)kTestBitsLength);
                rotate_soft_satdump(ber_test_.data(), kTestBitsLength, 0u, s != 0);
                rotate_soft_satdump(ber_test_.data(), kTestBitsLength, phase, false);
                signed_soft_to_unsigned(ber_test_.data(), ber_soft_.data(), (size_t)kTestBitsLength);

                for (int shift = 0; shift < 2; shift++) {
                    std::array<int8_t, kTestBitsLength> ber_i8{};
                    for (int i = 0; i < kTestBitsLength; i++)
                        ber_i8[(size_t)i] = (int8_t)((int)ber_soft_[(size_t)(i + shift)] - 128);

                    cc_decoder_ber_.decode_soft(ber_i8.data(), ber_decoded_.data(), 128);
                    std::array<uint8_t, 1024> bits_lsb{};
                    unpack_msb8(ber_decoded_.data(), 128, bits_lsb.data());
                    cc_encoder_ber_.reset_register();
                    cc_encoder_ber_.work(bits_lsb.data(), ber_encoded_.data());

                    d_bers[s][phase][(size_t)shift] = get_ber(ber_soft_.data() + shift, ber_encoded_.data(), kTestBitsLength);

                    if ((d_ber_ >= 10.0f && d_bers[s][phase][(size_t)shift] < d_ber_threshold_) ||
                        (d_ber_ < 10.0f && d_bers[s][phase][(size_t)shift] < d_ber_)) {
                        d_ber_ = d_bers[s][phase][(size_t)shift];
                        d_iq_swap_ = s != 0;
                        d_state_ = ST_SYNCED;
                        d_phase_ = phase;
                        d_shift_ = shift != 0;
                        d_invalid_ = 0;
                        std::memset(soft_u_.data(), 128, (size_t)d_buffer_size_ * 2u);
                    }
                }
            }
        }
    }

    int out_n = 0;
    if (d_state_ == ST_SYNCED) {
        rotate_soft_satdump(input, (size_t)size, 0u, d_iq_swap_);
        rotate_soft_satdump(input, (size_t)size, d_phase_, false);
        signed_soft_to_unsigned(input, soft_u_.data(), (size_t)size);

        std::array<int8_t, 16384> soft_i8{};
        const size_t shift = d_shift_ ? 1u : 0u;
        for (int i = 0; i < size; i++) {
            const size_t si = (size_t)i + shift;
            if (si < soft_u_.size())
                soft_i8[(size_t)i] = (int8_t)((int)soft_u_[si] - 128);
        }

        const size_t out_bytes = 512;
        cc_decoder_main_.decode_soft(soft_i8.data(), output, out_bytes);
        out_n = (int)out_bytes;

        std::array<uint8_t, 4096> bits4096{};
        unpack_msb8(output, out_bytes, bits4096.data());
        cc_encoder_main_.reset_register();
        cc_encoder_main_.work(bits4096.data(), main_encoded_.data());

        d_ber_ = get_ber(soft_u_.data() + (int)shift, main_encoded_.data(), kTestBitsLength);
        if (d_ber_ > d_ber_threshold_) {
            d_invalid_++;
            if (d_invalid_ > d_max_outsync_)
                d_state_ = ST_IDLE;
        } else {
            d_invalid_ = 0;
        }
    }

    return out_n;
}

float MeteorViterbi12::ber() const {
    if (d_state_ == ST_SYNCED)
        return d_ber_;
    float b = 10.0f;
    const int iq_max = d_check_iq_swap_ ? 2 : 1;
    for (int s = 0; s < iq_max; s++)
        for (unsigned p = 0; p < 4; p++)
            for (int o = 0; o < 2; o++)
                b = std::min(b, d_bers[s][p][(size_t)o]);
    return b;
}

}  // namespace meteor_lrpt
