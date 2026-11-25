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

/* AAC-LC core decoder for DAB+ (ISO/IEC 14496-3), 960-sample frames, mono out.
 * Core only: FAAD2 is 97 KB against 25 KB free in the code bank, and SBR/PS
 * would add 51 KB, so HE-AAC plays as its 24 kHz core. SCE, or a CPE's first
 * channel (M/S downmixed); all codebooks, pulses, TNS, both window shapes.
 * Not: a CPE's second channel, LTP, main prediction, PNS, SSR, error resilience. */

#ifndef __DAB_AAC_LC_H__
#define __DAB_AAC_LC_H__

#include <cstddef>
#include <cstdint>

namespace dab {

constexpr size_t kAacFrameLength = 960;
constexpr size_t kAacShortLength = kAacFrameLength / 8;
constexpr size_t kAacMaxSfb = 51;
constexpr size_t kAacMaxWindowGroups = 8;

class AacLcDecoder {
   public:
    /* Borrowed, not owned: only the transform uses it, so the receiver's FFT
     * buffer serves and 7.7 KB stays off a 25 KB heap. */
    static constexpr size_t kScratchFloats = 2 * kAacFrameLength;

    /* `sample_rate` is the *core* rate: 24000 for a 48 kHz service with SBR. */
    bool configure(uint32_t sample_rate, uint8_t channels, float* scratch, size_t scratch_floats);

    /* `pcm` holds kAacFrameLength. Returns samples written: zero for a rejected
     * unit and for the first after configure(), which only primes the overlap.
     * `ok` reports whether the unit parsed. */
    size_t decode(const uint8_t* au, size_t length, int16_t* pcm, bool* ok = nullptr);

    uint8_t last_error() const { return error_; }

    /* For the host harness. */
    uint8_t last_window_sequence() const { return last_window_sequence_; }
    uint8_t last_window_shape() const { return last_window_shape_; }
    bool last_had_tns() const { return last_had_tns_; }

    /* Last unit's DSE payload: start in bits from the unit start, length in
     * bytes (0: none). In DAB+ it carries the PAD (X-PAD, then two F-PAD bytes)
     * and need not be byte aligned. */
    size_t dse_bit() const { return dse_bit_; }
    size_t dse_bytes() const { return dse_bytes_; }

    void reset();

    /* Public only so the file-local Huffman decoder can take one. */
    struct BitReader {
        const uint8_t* data;
        size_t bytes;
        size_t bit;

        uint32_t read(uint8_t n);
        void skip(uint8_t n) { bit += n; }
        bool overrun() const { return bit > bytes * 8; }
    };

   private:
    size_t dse_bit_{0};
    uint16_t dse_bytes_{0};

    /* One channel's individual_channel_stream, before dequantisation. */
    struct Ics {
        uint8_t window_sequence;
        uint8_t window_shape;
        uint8_t max_sfb;
        uint8_t num_windows;
        uint8_t num_window_groups;
        uint8_t window_group_length[kAacMaxWindowGroups];
        uint8_t num_swb;
        uint16_t swb_offset[kAacMaxSfb + 1];

        uint8_t sfb_cb[kAacMaxWindowGroups][kAacMaxSfb];
        int16_t scale_factor[kAacMaxWindowGroups][kAacMaxSfb];

        uint8_t tns_present;
        uint8_t tns_n_filt[kAacMaxWindowGroups];
        uint8_t tns_order[kAacMaxWindowGroups][3];
        uint8_t tns_length[kAacMaxWindowGroups][3];
        uint8_t tns_direction[kAacMaxWindowGroups][3];
        int8_t tns_coef[kAacMaxWindowGroups][3][20];
        uint8_t tns_coef_res[kAacMaxWindowGroups];
        uint8_t tns_coef_compress[kAacMaxWindowGroups][3];
    };

    bool decode_ics_info(BitReader& br, Ics& ics);
    bool decode_section_data(BitReader& br, Ics& ics);
    bool decode_scale_factors(BitReader& br, Ics& ics, uint8_t global_gain);
    bool decode_pulse_data(BitReader& br, const Ics& ics);
    bool decode_tns_data(BitReader& br, Ics& ics);
    bool decode_spectral_data(BitReader& br, const Ics& ics);
    bool decode_channel_stream(BitReader& br, Ics& ics, bool read_info);
    void apply_pulses(const Ics& ics);
    void apply_tns(const Ics& ics);
    void filterbank(const Ics& ics, int16_t* pcm);

    bool configured_{false};
    bool primed_{false};
    uint8_t error_{0};
    uint8_t prev_window_shape_{0};
    uint8_t last_window_sequence_{0};
    uint8_t last_window_shape_{0};
    bool last_had_tns_{false};

    /* Dequantised as decoded (scalefactors are known by then). */
    float spec_[kAacFrameLength];
    float overlap_[kAacFrameLength];
    float* scratch_{nullptr};

    /* A member, not a local: ~2 KB against a 4.4 KB thread stack. */
    Ics ics_{};

    /* Pulse data patches quantised values before dequantisation. */
    uint8_t pulse_present_{0};
    uint8_t pulse_count_{0};
    uint16_t pulse_offset_[4];
    uint8_t pulse_amp_[4];
    uint8_t pulse_start_sfb_{0};
};

}  // namespace dab

#endif /*__DAB_AAC_LC_H__*/
