/*
 * Copyright (C) 2026 PortaPack Mayhem Contributors
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

#ifndef __GPS_TRACK_DSP_H__
#define __GPS_TRACK_DSP_H__

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>

#include "gps_acq_dsp.hpp"

namespace gps_scan {

// Fast minimax atan2f(y, x) avoiding libm bloat on M4 (max error ~0.005 rad = 0.3 deg).
static inline float fast_atanf(float z) {
    // Valid for |z| <= 1: z * (0.999866 - 0.330299*z^2 + 0.180141*z^4 - 0.085133*z^6) -> simple cubic/quadratic
    // Classic rational approx for |z| <= 1: z * (pi/4 + 0.273 * (1 - |z|)) (max err 0.0038 rad)
    const float az = (z < 0.0f) ? -z : z;
    return z * (0.78539816f + 0.273f * (1.0f - az));
}

static inline float fast_atan2f(float y, float x) {
    static constexpr float kPi = 3.14159265f;
    static constexpr float kHalfPi = 1.57079633f;
    const float ax = (x < 0.0f) ? -x : x;
    const float ay = (y < 0.0f) ? -y : y;
    if (ax < 1e-9f && ay < 1e-9f) return 0.0f;

    float a;
    if (ax >= ay) {
        a = fast_atanf(y / x);
        if (x < 0.0f) a += (y >= 0.0f) ? kPi : -kPi;
    } else {
        a = ((y >= 0.0f) ? kHalfPi : -kHalfPi) - fast_atanf(x / y);
    }
    return a;
}

// Fast 180-degree-insensitive Costas phase error atan(Q/I) in [-pi/2, +pi/2].
static inline float fast_costas_atan(float q, float i) {
    static constexpr float kHalfPi = 1.57079633f;
    if (i < 0.0f) {
        i = -i;
        q = -q;
    }
    const float aq = (q < 0.0f) ? -q : q;
    if (i < 1e-6f && aq < 1e-6f) return 0.0f;
    if (i >= aq) {
        return fast_atanf(q / i);
    }
    return ((q >= 0.0f) ? kHalfPi : -kHalfPi) - fast_atanf(i / q);
}

// IS-GPS-200 Table 20-XIV Hamming (32,26) parity masks over d1..d24 (bit 23 = d1, bit 0 = d24).
static constexpr uint32_t kParityMask[6] = {
    0xEC7CD2U,  // D25 (XOR with D29*)
    0x763E69U,  // D26 (XOR with D30*)
    0xBB1F34U,  // D27 (XOR with D29*)
    0x5D8F9AU,  // D28 (XOR with D30*)
    0xAEC7CDU,  // D29 (XOR with D30*)
    0x2DEA27U,  // D30 (XOR with D29*)
};

static inline uint32_t parity_bit24(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return v & 1U;
}

// Compute 6-bit parity D25..D30 (bit 5 = D25, bit 0 = D30) from recovered data d24 (d1..d24)
// and previous word's transmitted D29* and D30*.
static inline uint32_t compute_gps_parity(uint32_t d24, uint32_t d29_prev, uint32_t d30_prev) {
    uint32_t p = 0;
    p |= (d29_prev ^ parity_bit24(d24 & kParityMask[0])) << 5;
    p |= (d30_prev ^ parity_bit24(d24 & kParityMask[1])) << 4;
    p |= (d29_prev ^ parity_bit24(d24 & kParityMask[2])) << 3;
    p |= (d30_prev ^ parity_bit24(d24 & kParityMask[3])) << 2;
    p |= (d30_prev ^ parity_bit24(d24 & kParityMask[4])) << 1;
    p |= (d29_prev ^ parity_bit24(d24 & kParityMask[5])) << 0;
    return p;
}

// Verify a 30-bit received GPS word (MSB = D1, LSB = D30) given previous D29*, D30*.
// Returns true if parity passes, and writes recovered 24-bit data d1..d24 to out_d24.
static inline bool check_gps_word(uint32_t word30, uint32_t d29_prev, uint32_t d30_prev, uint32_t& out_d24) {
    uint32_t raw_d24 = (word30 >> 6) & 0xFFFFFFU;
    if (d30_prev) raw_d24 ^= 0xFFFFFFU;
    const uint32_t rx_p = word30 & 0x3FU;
    const uint32_t calc_p = compute_gps_parity(raw_d24, d29_prev, d30_prev);
    out_d24 = raw_d24;
    return (rx_p == calc_p);
}

// Encode a 24-bit data word into a 30-bit transmitted GPS word.
// If force_zero_d29_d30 is true (for Word 2 HOW / Word 10), solves bits d23..d24 so D29=0, D30=0.
static inline uint32_t encode_gps_word(uint32_t d24, uint32_t d29_prev, uint32_t d30_prev, bool force_zero_d29_d30) {
    d24 &= 0xFFFFFFU;
    if (force_zero_d29_d30) {
        d24 &= 0xFFFFFCU;
        for (uint32_t tail = 0; tail < 4; tail++) {
            const uint32_t cand = d24 | tail;
            const uint32_t p = compute_gps_parity(cand, d29_prev, d30_prev);
            if ((p & 3U) == 0U) {
                d24 = cand;
                break;
            }
        }
    }
    const uint32_t p = compute_gps_parity(d24, d29_prev, d30_prev);
    const uint32_t tx_d24 = d30_prev ? (d24 ^ 0xFFFFFFU) : d24;
    return (tx_d24 << 6) | p;
}

// 50 bps GPS LNAV Subframe Synchronizer & Telemetry Decoder
class GPSNavDecoder {
   public:
    void reset() {
        word_sr_ = 0;
        bits_in_sr_ = 0;
        frame_synced_ = 0;
        inverted_ = false;
        word_idx_ = 0;
        bit_in_word_ = 0;
        d29_prev_ = 0;
        d30_prev_ = 0;
        total_bits_ = 0;
        recent_bits_ = 0;
        tow_sec_ = 0;
        gps_week_ = 0;
        sf_mask_ = 0;
        last_sf_id_ = 0;
        last_parity_ok_ = 0;
        sv_health_ = 0xFF;
        ura_idx_ = 0xFF;
        last_iode_ = 0;
        alert_as_ = 0;
        frames_ok_ = 0;
        frames_err_ = 0;
        cur_parity_ok_ = 0;
        std::memset(sf_words_, 0, sizeof(sf_words_));
    }

    void push_bit(uint8_t raw_bit) {
        raw_bit &= 1U;
        total_bits_++;
        recent_bits_ = (recent_bits_ << 1) | raw_bit;

        if (frame_synced_ == 0) {
            // Shift into 62-bit window to test Word 1 (TLM) + Word 2 (HOW) simultaneously!
            // Keep prev2 (D29*, D30*) + 30-bit Word 1 + 30-bit Word 2 = 62 bits!
            word_sr_ = (word_sr_ << 1) | raw_bit;
            if (bits_in_sr_ < 62) {
                bits_in_sr_++;
                return;
            }

            // Try normal polarity (inv=false) and inverted polarity (inv=true)
            for (int pol = 0; pol < 2; pol++) {
                const uint64_t sr = pol ? ~word_sr_ : word_sr_;
                const uint32_t w2 = static_cast<uint32_t>(sr & 0x3FFFFFFFU);
                const uint32_t w1 = static_cast<uint32_t>((sr >> 30) & 0x3FFFFFFFU);
                const uint32_t p30 = static_cast<uint32_t>((sr >> 60) & 1U);
                const uint32_t p29 = static_cast<uint32_t>((sr >> 61) & 1U);

                // Quick preamble check: recovered d1..d8 of Word 1 must equal 0x8B
                const uint32_t pre = ((w1 >> 22) ^ (p30 ? 0xFFU : 0U)) & 0xFFU;
                if (pre != 0x8BU) continue;

                uint32_t d1 = 0, d2 = 0;
                if (!check_gps_word(w1, p29, p30, d1)) continue;
                const uint32_t w1_d29 = (w1 >> 1) & 1U;
                const uint32_t w1_d30 = w1 & 1U;
                if (!check_gps_word(w2, w1_d29, w1_d30, d2)) continue;

                // Verify HOW subframe ID (bits d20..d22 => (d2 >> 2) & 7) in 1..5
                // and HOW D29=0, D30=0 (standard IS-GPS-200 requirement!)
                const uint8_t sf_id = static_cast<uint8_t>((d2 >> 2) & 7U);
                if (sf_id < 1 || sf_id > 5) continue;
                if ((w2 & 3U) != 0U) continue;

                // Subframe lock achieved on TLM + HOW!
                inverted_ = (pol != 0);
                frame_synced_ = inverted_ ? 2 : 1;
                sf_words_[0] = d1;
                sf_words_[1] = d2;
                cur_parity_ok_ = 2;
                word_idx_ = 2;  // Next bit starts Word 3
                bit_in_word_ = 0;
                d29_prev_ = (w2 >> 1) & 1U;
                d30_prev_ = w2 & 1U;
                cur_word_ = 0;
                decode_how(d2);
                return;
            }
            return;
        }

        // Frame-synced mode: collect 30 bits per word
        const uint32_t b = inverted_ ? (raw_bit ^ 1U) : raw_bit;
        cur_word_ = ((cur_word_ << 1) | b) & 0x3FFFFFFFU;
        bit_in_word_++;

        if (bit_in_word_ < 30) return;

        // Complete 30-bit word received
        uint32_t d24 = 0;
        const bool ok = check_gps_word(cur_word_, d29_prev_, d30_prev_, d24);
        d29_prev_ = (cur_word_ >> 1) & 1U;
        d30_prev_ = cur_word_ & 1U;

        if (word_idx_ == 0) {
            cur_parity_ok_ = 0;
            // Verify preamble still matches 0x8B on Word 1
            if (!ok || (((d24 >> 16) & 0xFFU) != 0x8BU)) {
                frames_err_++;
                frame_synced_ = 0;
                bits_in_sr_ = 0;
                word_sr_ = 0;
                return;
            }
        }

        if (ok) {
            cur_parity_ok_++;
            sf_words_[word_idx_] = d24;
            if (word_idx_ == 1) {
                decode_how(d24);
            }
        }

        word_idx_++;
        bit_in_word_ = 0;
        cur_word_ = 0;

        if (word_idx_ >= 10) {
            // Complete 300-bit subframe received!
            last_parity_ok_ = cur_parity_ok_;
            if (cur_parity_ok_ == 10) {
                frames_ok_++;
                decode_subframe();
            } else {
                frames_err_++;
            }
            word_idx_ = 0;
        }
    }

    void fill_status(TrackStatus& st) const {
        st.frame_synced = frame_synced_;
        st.total_bits = total_bits_;
        st.recent_bits = recent_bits_;
        st.tow_sec = tow_sec_;
        st.gps_week = gps_week_;
        st.sf_mask = sf_mask_;
        st.last_sf_id = last_sf_id_;
        st.last_parity_ok = last_parity_ok_;
        st.sv_health = sv_health_;
        st.ura_idx = ura_idx_;
        st.last_iode = last_iode_;
        st.alert_as = alert_as_;
        st.frames_ok = frames_ok_;
        st.frames_err = frames_err_;
    }

   private:
    void decode_how(uint32_t d2) {
        const uint32_t tow_count = (d2 >> 7) & 0x1FFFFU;
        // HOW TOW-count is start of NEXT subframe; subtract 1 (mod 100800) for current subframe start
        const uint32_t cur_z = (tow_count == 0) ? 100799U : (tow_count - 1U);
        tow_sec_ = (cur_z * 6U) % 604800U;
        alert_as_ = static_cast<uint8_t>(((d2 >> 6) & 1U) | (((d2 >> 5) & 1U) << 1));
        const uint8_t sf = static_cast<uint8_t>((d2 >> 2) & 7U);
        if (sf >= 1 && sf <= 5) {
            last_sf_id_ = sf;
        }
    }

    void decode_subframe() {
        const uint8_t sf = static_cast<uint8_t>((sf_words_[1] >> 2) & 7U);
        if (sf < 1 || sf > 5) return;
        last_sf_id_ = sf;
        sf_mask_ |= static_cast<uint8_t>(1U << (sf - 1));

        const uint32_t w3 = sf_words_[2];
        if (sf == 1) {
            const uint16_t wn10 = static_cast<uint16_t>((w3 >> 14) & 0x3FFU);
            gps_week_ = static_cast<uint16_t>(2048U + wn10);
            ura_idx_ = static_cast<uint8_t>((w3 >> 8) & 0x0FU);
            sv_health_ = static_cast<uint8_t>((w3 >> 2) & 0x3FU);
        } else if (sf == 2 || sf == 3) {
            last_iode_ = static_cast<uint8_t>((w3 >> 16) & 0xFFU);
        }
    }

    uint64_t word_sr_{0};
    uint8_t bits_in_sr_{0};
    uint8_t frame_synced_{0};
    bool inverted_{false};
    uint8_t word_idx_{0};
    uint8_t bit_in_word_{0};
    uint32_t cur_word_{0};
    uint32_t d29_prev_{0};
    uint32_t d30_prev_{0};
    uint8_t cur_parity_ok_{0};
    uint32_t sf_words_[10]{};

    uint32_t total_bits_{0};
    uint64_t recent_bits_{0};
    uint32_t tow_sec_{0};
    uint16_t gps_week_{0};
    uint8_t sf_mask_{0};
    uint8_t last_sf_id_{0};
    uint8_t last_parity_ok_{0};
    uint8_t sv_health_{0xFF};
    uint8_t ura_idx_{0xFF};
    uint8_t last_iode_{0};
    uint8_t alert_as_{0};
    uint16_t frames_ok_{0};
    uint16_t frames_err_{0};
};

// Single-Channel Real-Time GPS L1 C/A Tracker (FLL -> Costas PLL + Carrier-Aided Non-Coherent DLL)
class GPSTracker {
   public:
    void init(uint8_t prn, float init_doppler_hz, float init_code_chip, bool is_selftest) {
        if (prn < 1 || prn > NUM_PRNS) prn = 1;
        prn_ = prn;
        is_selftest_ = is_selftest ? 1 : 0;

        // Build padded ±1 C/A chip LUT: [0]=chip[1022], [1..1023]=chip[0..1022], [1024]=chip[0]
        uint8_t bits[128];
        generate_ca_bits(prn_, bits);
        for (size_t i = 0; i < 1023; i++) {
            code_padded_[i + 1] = get_ca_chip(bits, i);
        }
        code_padded_[0] = code_padded_[1023];
        code_padded_[1024] = code_padded_[1];

        // Precompute 320-entry int8 sin/cos table once (cos[i] = sin_tbl_[i + 64])
        for (size_t i = 0; i < 320; i++) {
            const int32_t s = sin_2048((static_cast<uint32_t>(i) & 255U) * 8U);
            sin_tbl_[i] = static_cast<int8_t>((s * 127 + 16384) >> 15);
        }

        // Wrap initial code phase into [0, 1023)
        while (init_code_chip < 0.0f) init_code_chip += 1023.0f;
        while (init_code_chip >= 1023.0f) init_code_chip -= 1023.0f;
        chip_int_ = static_cast<uint16_t>(init_code_chip);
        if (chip_int_ >= 1023) chip_int_ = 0;
        const float frac = init_code_chip - static_cast<float>(chip_int_);
        chip_frac_ = static_cast<uint32_t>(frac * 4294967296.0f);

        carr_phase_ = 0;
        carr_freq_hz_ = init_doppler_hz;
        code_freq_err_ = 0.0f;
        update_nco_steps(carr_freq_hz_, 1023000.0f + carr_freq_hz_ * (1.0f / 1540.0f));

        ie_ = qe_ = ip_ = qp_ = il_ = ql_ = 0;
        ip_prev_ = qp_prev_ = 0.0f;
        has_prev_ = false;
        epoch_count_ = 0;
        state_ = TrackState::PullIn;

        // Lock & C/N0 estimators
        pli_filt_ = 0.0f;
        cn0_dbhz_ = 0.0f;
        early_mag_ = prompt_mag_ = late_mag_ = 0.0f;
        iq_scale_ = 1.0f;
        iq_hist_pos_ = 0;
        std::memset(iq_hist_, 0, sizeof(iq_hist_));
        low_lock_ms_ = 0;

        // C/N0 Van Dierendonck accumulators (over 20ms windows, averaged over 10 blocks)
        cn0_ms_ = 0;
        cn0_si_ = cn0_sq_ = cn0_wbp_ = 0.0f;
        cn0_np_sum_ = 0.0f;
        cn0_np_cnt_ = 0;

        // 20ms Bit Sync state
        bit_synced_ = false;
        bit_phase_ = 0;
        bit_flips_ = 0;
        bit_accum_ = 0.0f;
        std::memset(edge_hist_, 0, sizeof(edge_hist_));

        nav_.reset();
    }

    // Process 1 DMA buffer (2048 complex int8 samples = 1.000 ms) in real time.
    // Split into 2 zero-spill passes over 128-sample chunks so all 6 correlator
    // accumulators stay in Cortex-M4 registers (<0.30 ms per 1.000 ms buffer!).
    GPS_HOT_NOINLINE void process_samples(const int8_t* iq_interleaved, size_t count) {
        static constexpr size_t kChunk = 128;
        c16_t bb[kChunk];

        size_t pos = 0;
        while (pos < count) {
            const size_t n_chunk = ((count - pos) < kChunk) ? (count - pos) : kChunk;
            mix_chunk(iq_interleaved + pos * 2, n_chunk, bb);
            size_t sub = 0;
            while (sub < n_chunk) {
                const size_t consumed = correlate_segment(bb + sub, n_chunk - sub);
                sub += consumed;
                if (chip_int_ >= 1023) {
                    chip_int_ = 0;
                    on_epoch_dump();
                    ie_ = qe_ = ip_ = qp_ = il_ = ql_ = 0;
                }
            }
            pos += n_chunk;
        }
    }

    void fill_status(TrackStatus& st, uint16_t dma_drops) const {
        st.prn = prn_;
        st.state = static_cast<uint8_t>(state_);
        st.bit_synced = bit_synced_ ? 1 : 0;
        st.doppler_hz_x10 = static_cast<int32_t>(carr_freq_hz_ * 10.0f);
        const float cur_chip = static_cast<float>(chip_int_) + static_cast<float>(chip_frac_) * (1.0f / 4294967296.0f);
        st.code_chip_x100 = static_cast<uint32_t>(cur_chip * 100.0f);
        st.cn0_dbhz_x10 = static_cast<int16_t>(cn0_dbhz_ * 10.0f);
        st.pli_x100 = static_cast<int16_t>(pli_filt_ * 100.0f);
        st.early_mag = static_cast<uint16_t>(early_mag_ > 65535.0f ? 65535.0f : early_mag_);
        st.prompt_mag = static_cast<uint16_t>(prompt_mag_ > 65535.0f ? 65535.0f : prompt_mag_);
        st.late_mag = static_cast<uint16_t>(late_mag_ > 65535.0f ? 65535.0f : late_mag_);
        st.track_sec = static_cast<uint16_t>(epoch_count_ / 1000U);
        st.dma_drops = dma_drops;
        st.is_selftest = is_selftest_;

        // Copy chronological I/Q history (oldest -> newest)
        for (size_t i = 0; i < TRACK_IQ_HISTORY; i++) {
            st.iq_hist[i] = iq_hist_[(iq_hist_pos_ + i) & (TRACK_IQ_HISTORY - 1U)];
        }
        nav_.fill_status(st);
    }

    TrackState state() const { return state_; }
    uint32_t epoch_count() const { return epoch_count_; }

   private:
    GPS_HOT_NOINLINE void mix_chunk(const int8_t* iq, size_t n, c16_t* dst) {
        uint32_t c_ph = carr_phase_;
        const uint32_t c_step = carr_step_;
        const int8_t* const sin_p = sin_tbl_;
        const int8_t* const cos_p = sin_tbl_ + 64;

        for (size_t k = 0; k < n; k++) {
            const int32_t re = iq[2 * k];
            const int32_t im = iq[2 * k + 1];
            const uint32_t ang = c_ph >> 24;
            const int32_t c = cos_p[ang];
            const int32_t s = sin_p[ang];
            c_ph += c_step;
            dst[k].re = static_cast<int16_t>(re * c + im * s);
            dst[k].im = static_cast<int16_t>(im * c - re * s);
        }
        carr_phase_ = c_ph;
    }

    // Correlates until either `n` samples are processed or chip_int_ hits 1023.
    // No function calls inside -> keeps accumulators in registers with zero spills!
    GPS_HOT_NOINLINE size_t correlate_segment(const c16_t* bb, size_t n) {
        uint32_t ch_i = chip_int_;
        uint32_t ch_f = chip_frac_;
        const uint32_t ch_step = chip_step_;
        const int8_t* const code_p = code_padded_;

        int32_t ie = ie_, qe = qe_;
        int32_t ip = ip_, qp = qp_;
        int32_t il = il_, ql = ql_;

        const c16_t* p = bb;
        const c16_t* const end = bb + n;

        while (p < end) {
            const int32_t bi = p->re;
            const int32_t bq = p->im;
            p++;

            const uint32_t half = ch_f >> 31;
            const int8_t* cp_ptr = code_p + ch_i;
            const int32_t cp = cp_ptr[1];
            const int32_t ce = cp_ptr[1 + half];
            const int32_t cl = cp_ptr[half];

            ie += bi * ce;
            qe += bq * ce;
            ip += bi * cp;
            qp += bq * cp;
            il += bi * cl;
            ql += bq * cl;

            const uint32_t prev_f = ch_f;
            ch_f += ch_step;
            if (ch_f < prev_f) {
                ch_i++;
                if (ch_i >= 1023) break;
            }
        }

        chip_int_ = static_cast<uint16_t>(ch_i);
        chip_frac_ = ch_f;
        ie_ = ie;
        qe_ = qe;
        ip_ = ip;
        qp_ = qp;
        il_ = il;
        ql_ = ql;
        return static_cast<size_t>(p - bb);
    }

    void update_nco_steps(float f_carr_hz, float f_code_hz) {
        // carr_step = round(f_carr_hz / 2048000.0 * 2^32) = f_carr_hz * 2097.152
        const int32_t cs = static_cast<int32_t>(f_carr_hz * 2097.152f);
        carr_step_ = static_cast<uint32_t>(cs);
        // chip_step = round(f_code_hz / 2048000.0 * 2^32) = f_code_hz * 2097.152
        // Use double/split arithmetic for exact sub-Hz code NCO precision!
        const float delta_code = f_code_hz - 1023000.0f;
        // Exact nominal: 1023000 / 2048000 * 2^32 = 2145386496 (exact integer!)
        const int32_t delta_step = static_cast<int32_t>(delta_code * 2097.152f);
        chip_step_ = static_cast<uint32_t>(2145386496U + delta_step);
    }

    void on_epoch_dump() {
        epoch_count_++;

        // Scale accumulators down by 1/1024 to comfortable float range (~100..5000)
        const float fe_i = static_cast<float>(ie_) * (1.0f / 1024.0f);
        const float fe_q = static_cast<float>(qe_) * (1.0f / 1024.0f);
        const float fp_i = static_cast<float>(ip_) * (1.0f / 1024.0f);
        const float fp_q = static_cast<float>(qp_) * (1.0f / 1024.0f);
        const float fl_i = static_cast<float>(il_) * (1.0f / 1024.0f);
        const float fl_q = static_cast<float>(ql_) * (1.0f / 1024.0f);

        const float mag_e = sqrtf(fe_i * fe_i + fe_q * fe_q);
        const float mag_p = sqrtf(fp_i * fp_i + fp_q * fp_q);
        const float mag_l = sqrtf(fl_i * fl_i + fl_q * fl_q);

        // Smooth E/P/L magnitudes for UI display
        if (epoch_count_ == 1) {
            early_mag_ = mag_e;
            prompt_mag_ = mag_p;
            late_mag_ = mag_l;
            iq_scale_ = (mag_p > 10.0f) ? mag_p : 10.0f;
        } else {
            early_mag_ += 0.05f * (mag_e - early_mag_);
            prompt_mag_ += 0.05f * (mag_p - prompt_mag_);
            late_mag_ += 0.05f * (mag_l - late_mag_);
            iq_scale_ += 0.03f * ((mag_p > 10.0f ? mag_p : 10.0f) - iq_scale_);
        }

        // Record normalized prompt I/Q point for constellation plot (every 4 ms => 128ms window)
        if ((epoch_count_ & 3U) == 0U) {
            const float inv_s = 70.0f / (iq_scale_ > 1.0f ? iq_scale_ : 1.0f);
            int32_t qi = static_cast<int32_t>(fp_i * inv_s);
            int32_t qq = static_cast<int32_t>(fp_q * inv_s);
            if (qi > 127) qi = 127;
            if (qi < -127) qi = -127;
            if (qq > 127) qq = 127;
            if (qq < -127) qq = -127;
            iq_hist_[iq_hist_pos_] = {static_cast<int8_t>(qi), static_cast<int8_t>(qq)};
            iq_hist_pos_ = (iq_hist_pos_ + 1U) & (TRACK_IQ_HISTORY - 1U);
        }

        // 1. Carrier loop (PullIn FLL -> WidePLL -> Locked Costas PLL)
        float f_carr_nco = carr_freq_hz_;
        if (epoch_count_ <= 250) {
            state_ = TrackState::PullIn;
            if (has_prev_) {
                float dot = ip_prev_ * fp_i + qp_prev_ * fp_q;
                float cross = ip_prev_ * fp_q - qp_prev_ * fp_i;
                // 180-degree nav-bit-flip immune FLL discriminator!
                if (dot < 0.0f) {
                    dot = -dot;
                    cross = -cross;
                }
                const float dphi = fast_atan2f(cross, dot);  // in [-pi/2, +pi/2]
                const float f_err = dphi * (1000.0f / 6.2831853f);  // Hz
                carr_freq_hz_ += 0.12f * f_err;
                f_carr_nco = carr_freq_hz_;
            }
        } else {
            const bool wide = (epoch_count_ <= 650);
            if (state_ != TrackState::Lost) {
                state_ = wide ? TrackState::WidePLL : TrackState::Locked;
            }
            const float ph_err = fast_costas_atan(fp_q, fp_i);  // radians in [-pi/2, +pi/2]
            const float kp = wide ? 10.62f : 6.37f;
            const float ki = wide ? 0.354f : 0.127f;
            carr_freq_hz_ += ki * ph_err;
            f_carr_nco = carr_freq_hz_ + kp * ph_err;
        }

        // Clamp carrier frequency to reasonable bounds
        if (carr_freq_hz_ > 45000.0f) carr_freq_hz_ = 45000.0f;
        if (carr_freq_hz_ < -45000.0f) carr_freq_hz_ = -45000.0f;

        // 2. Non-coherent Early-Minus-Late Envelope DLL (carrier-aided!)
        const float el_sum = mag_e + mag_l;
        float code_err = 0.0f;
        if (el_sum > 1e-3f) {
            code_err = 0.5f * (mag_e - mag_l) / el_sum;
        }
        const bool dll_wide = (epoch_count_ <= 650);
        const float kp_dll = dll_wide ? 5.33f : 2.67f;
        const float ki_dll = dll_wide ? 0.0142f : 0.0036f;
        code_freq_err_ += ki_dll * code_err;
        if (code_freq_err_ > 50.0f) code_freq_err_ = 50.0f;
        if (code_freq_err_ < -50.0f) code_freq_err_ = -50.0f;

        const float f_code_nco = 1023000.0f + carr_freq_hz_ * (1.0f / 1540.0f) + code_freq_err_ + kp_dll * code_err;
        update_nco_steps(f_carr_nco, f_code_nco);

        // 3. Phase Lock Indicator (PLI)
        const float p_sq = fp_i * fp_i + fp_q * fp_q;
        if (p_sq > 1e-3f) {
            const float pli_inst = (fp_i * fp_i - fp_q * fp_q) / p_sq;
            pli_filt_ += 0.03f * (pli_inst - pli_filt_);
        }

        // 4. 50 bps Navigation Bit Edge Sync & Integration + Bit-Aligned Van Dierendonck C/N0
        const uint8_t phase20 = static_cast<uint8_t>(epoch_count_ % 20U);
        bool dump_cn0_window = false;

        if (epoch_count_ > 400 && has_prev_) {
            if (!bit_synced_) {
                if ((ip_prev_ > 0.0f && fp_i < 0.0f) || (ip_prev_ < 0.0f && fp_i > 0.0f)) {
                    edge_hist_[phase20]++;
                    bit_flips_++;
                    if (bit_flips_ >= 8) {
                        uint16_t max1 = 0, max2 = 0;
                        uint8_t best_ph = 0;
                        for (uint8_t k = 0; k < 20; k++) {
                            if (edge_hist_[k] > max1) {
                                max2 = max1;
                                max1 = edge_hist_[k];
                                best_ph = k;
                            } else if (edge_hist_[k] > max2) {
                                max2 = edge_hist_[k];
                            }
                        }
                        if (max1 >= 6 && max1 >= max2 * 3 + 2) {
                            bit_phase_ = best_ph;
                            bit_synced_ = true;
                            bit_accum_ = fp_i;
                            // Start C/N0 window strictly aligned on this new bit!
                            cn0_si_ = fp_i;
                            cn0_sq_ = fp_q;
                            cn0_wbp_ = p_sq;
                            cn0_ms_ = 1;
                            cn0_np_sum_ = 0.0f;
                            cn0_np_cnt_ = 0;
                        } else if (bit_flips_ >= 60) {
                            // Reset stale histogram if transient noise or early pull-in smeared edges
                            std::memset(edge_hist_, 0, sizeof(edge_hist_));
                            bit_flips_ = 0;
                        }
                    }
                }
            } else {
                // Aligned to bit edge: phase20 == bit_phase_ is the FIRST ms of a new bit!
                if (phase20 == bit_phase_) {
                    const uint8_t bit = (bit_accum_ >= 0.0f) ? 1U : 0U;
                    nav_.push_bit(bit);
                    bit_accum_ = fp_i;
                    dump_cn0_window = (cn0_ms_ == 20);
                } else {
                    bit_accum_ += fp_i;
                }
            }
        }

        // Van Dierendonck C/N0 (20ms coherent NBP/WBP, bit-aligned once bit_synced_)
        if (bit_synced_) {
            if (phase20 == bit_phase_) {
                if (dump_cn0_window && cn0_wbp_ > 1e-3f) {
                    const float nbp = cn0_si_ * cn0_si_ + cn0_sq_ * cn0_sq_;
                    cn0_np_sum_ += nbp / cn0_wbp_;
                    cn0_np_cnt_++;
                }
                cn0_si_ = fp_i;
                cn0_sq_ = fp_q;
                cn0_wbp_ = p_sq;
                cn0_ms_ = 1;
            } else if (cn0_ms_ > 0 && cn0_ms_ < 20) {
                cn0_si_ += fp_i;
                cn0_sq_ += fp_q;
                cn0_wbp_ += p_sq;
                cn0_ms_++;
            }
        } else {
            cn0_si_ += fp_i;
            cn0_sq_ += fp_q;
            cn0_wbp_ += p_sq;
            cn0_ms_++;
            if (cn0_ms_ >= 20) {
                if (cn0_wbp_ > 1e-3f) {
                    const float nbp = cn0_si_ * cn0_si_ + cn0_sq_ * cn0_sq_;
                    cn0_np_sum_ += nbp / cn0_wbp_;
                    cn0_np_cnt_++;
                }
                cn0_si_ = cn0_sq_ = cn0_wbp_ = 0.0f;
                cn0_ms_ = 0;
            }
        }

        if (cn0_np_cnt_ >= 10) {
            float mu = cn0_np_sum_ / static_cast<float>(cn0_np_cnt_);
            if (mu < 1.05f) mu = 1.05f;
            if (mu > 19.95f) mu = 19.95f;
            const float cn0_inst = 10.0f * fast_log10f((mu - 1.0f) / ((20.0f - mu) * 0.001f));
            if (cn0_dbhz_ == 0.0f)
                cn0_dbhz_ = cn0_inst;
            else
                cn0_dbhz_ += 0.35f * (cn0_inst - cn0_dbhz_);
            cn0_np_sum_ = 0.0f;
            cn0_np_cnt_ = 0;
        }

        // Lock-loss watchdog (only after pull-in)
        if (epoch_count_ > 1000) {
            if (pli_filt_ < 0.20f && cn0_dbhz_ < 25.0f) {
                low_lock_ms_++;
                if (low_lock_ms_ > 2000) state_ = TrackState::Lost;
            } else {
                low_lock_ms_ = 0;
                if (state_ == TrackState::Lost) state_ = TrackState::Locked;
            }
        }

        ip_prev_ = fp_i;
        qp_prev_ = fp_q;
        has_prev_ = true;
    }

    uint8_t prn_{1};
    uint8_t is_selftest_{0};
    TrackState state_{TrackState::Off};

    int8_t code_padded_[1025]{};
    int8_t sin_tbl_[320]{};

    uint32_t carr_phase_{0};
    uint32_t carr_step_{0};
    uint16_t chip_int_{0};
    uint32_t chip_frac_{0};
    uint32_t chip_step_{2145386496U};

    float carr_freq_hz_{0.0f};
    float code_freq_err_{0.0f};

    int32_t ie_{0}, qe_{0}, ip_{0}, qp_{0}, il_{0}, ql_{0};
    float ip_prev_{0.0f}, qp_prev_{0.0f};
    bool has_prev_{false};
    uint32_t epoch_count_{0};

    float pli_filt_{0.0f};
    float cn0_dbhz_{0.0f};
    float early_mag_{0.0f}, prompt_mag_{0.0f}, late_mag_{0.0f};
    float iq_scale_{1.0f};
    TrackIQ iq_hist_[TRACK_IQ_HISTORY]{};
    uint8_t iq_hist_pos_{0};
    uint16_t low_lock_ms_{0};

    uint8_t cn0_ms_{0};
    float cn0_si_{0.0f}, cn0_sq_{0.0f}, cn0_wbp_{0.0f};
    float cn0_np_sum_{0.0f};
    uint8_t cn0_np_cnt_{0};

    bool bit_synced_{false};
    uint8_t bit_phase_{0};
    uint16_t bit_flips_{0};
    uint16_t edge_hist_[20]{};
    float bit_accum_{0.0f};

    GPSNavDecoder nav_{};
};

// Real-Time Synthetic GPS L1 + Valid IS-GPS-200 LNAV Generator (for TrackSelfTest)
// Broadcasts PRN 1 at +1370 Hz Doppler, valid Subframes 1..3 with 10/10 Hamming parity,
// GPS Week 2445, SV Health OK, IODE 42, and live ticking TOW every 6 seconds!
class GPSTrackSim {
   public:
    void init(uint8_t prn, float true_dopp_hz, float init_chip) {
        prn_ = (prn >= 1 && prn <= NUM_PRNS) ? prn : 1;
        uint8_t bits[128];
        generate_ca_bits(prn_, bits);
        for (size_t i = 0; i < 1023; i++) code_[i] = get_ca_chip(bits, i);
        for (size_t i = 0; i < 320; i++) {
            const int32_t s = sin_2048((static_cast<uint32_t>(i) & 255U) * 8U);
            sin_tbl_[i] = static_cast<int8_t>((s * 127 + 16384) >> 15);
        }

        carr_ph_ = 0;
        carr_step_ = static_cast<uint32_t>(static_cast<int32_t>(true_dopp_hz * 2097.152f));
        const float f_code = 1023000.0f + true_dopp_hz * (1.0f / 1540.0f);
        chip_step_ = static_cast<uint32_t>(2145386496U + static_cast<int32_t>((f_code - 1023000.0f) * 2097.152f));

        while (init_chip < 0.0f) init_chip += 1023.0f;
        while (init_chip >= 1023.0f) init_chip -= 1023.0f;
        chip_i_ = static_cast<uint16_t>(init_chip);
        chip_f_ = static_cast<uint32_t>((init_chip - static_cast<float>(chip_i_)) * 4294967296.0f);

        prng_ = 0x5A3C96E1U;
        ms_in_bit_ = 7;  // Start mid-bit to test real bit-edge synchronization!
        sf_id_ = 1;
        tow_z_ = 50400U;  // Start at TOW = 302400s (Wed 12:00:00)
        d29_ = 0;
        d30_ = 0;
        build_subframe(sf_id_, tow_z_);
        bit_in_sf_ = 260;  // Start near end of subframe so first full SF arrives fast (~1s)!
        cur_nav_sign_ = get_sf_bit_sign(bit_in_sf_);
    }

    void generate_ms(int8_t out_iq[SAMPLES_PER_MS * 2]) {
        uint32_t c_ph = carr_ph_;
        const uint32_t c_step = carr_step_;
        uint16_t ch_i = chip_i_;
        uint32_t ch_f = chip_f_;
        const uint32_t ch_step = chip_step_;
        uint32_t rng = prng_;
        int32_t nav_s = cur_nav_sign_;

        const int8_t* const sin_p = sin_tbl_;
        const int8_t* const cos_p = sin_tbl_ + 64;

        for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
            // Fast Gaussian-like noise (sigma ~ 24)
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            const int32_t ni = (static_cast<int32_t>((rng & 0xFFU) + ((rng >> 8) & 0xFFU) + ((rng >> 16) & 0xFFU) + ((rng >> 24) & 0xFFU)) - 510) / 6;
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            const int32_t nq = (static_cast<int32_t>((rng & 0xFFU) + ((rng >> 8) & 0xFFU) + ((rng >> 16) & 0xFFU) + ((rng >> 24) & 0xFFU)) - 510) / 6;

            // Signal amplitude ~4.2 counts in sigma=24 noise (C/N0 ~ 44 dB-Hz)
            const int32_t sym = static_cast<int32_t>(code_[ch_i]) * nav_s;
            const uint32_t ang = c_ph >> 24;
            const int32_t si = (sym * cos_p[ang] * 42) >> 10;
            const int32_t sq = (sym * sin_p[ang] * 42) >> 10;

            int32_t vi = ni + si;
            int32_t vq = nq + sq;
            if (vi > 127) vi = 127;
            if (vi < -128) vi = -128;
            if (vq > 127) vq = 127;
            if (vq < -128) vq = -128;

            out_iq[2 * n] = static_cast<int8_t>(vi);
            out_iq[2 * n + 1] = static_cast<int8_t>(vq);

            c_ph += c_step;
            const uint32_t pf = ch_f;
            ch_f += ch_step;
            if (ch_f < pf) {
                ch_i++;
                if (ch_i >= 1023) {
                    ch_i = 0;
                    ms_in_bit_++;
                    if (ms_in_bit_ >= 20) {
                        ms_in_bit_ = 0;
                        bit_in_sf_++;
                        if (bit_in_sf_ >= 300) {
                            bit_in_sf_ = 0;
                            sf_id_ = (sf_id_ >= 3) ? 1 : static_cast<uint8_t>(sf_id_ + 1);
                            tow_z_ = (tow_z_ + 1U) % 100800U;
                            build_subframe(sf_id_, tow_z_);
                        }
                        nav_s = get_sf_bit_sign(bit_in_sf_);
                    }
                }
            }
        }

        carr_ph_ = c_ph;
        chip_i_ = ch_i;
        chip_f_ = ch_f;
        prng_ = rng;
        cur_nav_sign_ = nav_s;
    }

   private:
    void build_subframe(uint8_t sf_id, uint32_t tow_z) {
        // Word 1: TLM (preamble 0x8B in bits d1..d8)
        const uint32_t w1_d = 0x8B0000U | 0x001234U;
        // Word 2: HOW (next subframe Z-count in d1..d17, alert=0, as=0, sf_id in d20..d22)
        const uint32_t next_z = (tow_z + 1U) % 100800U;
        const uint32_t w2_d = ((next_z & 0x1FFFFU) << 7) | ((static_cast<uint32_t>(sf_id) & 7U) << 2);

        uint32_t raw_d[10];
        raw_d[0] = w1_d;
        raw_d[1] = w2_d;
        if (sf_id == 1) {
            // Week 2445 (wn10 = 397), L2=1, URA=0, SV Health=0, IODC=0
            raw_d[2] = ((397U & 0x3FFU) << 14) | (1U << 12);
        } else {
            // SF2/SF3: IODE = 42 in d1..d8
            raw_d[2] = (42U << 16) | 0x005A3CU;
        }
        for (int w = 3; w < 10; w++) {
            raw_d[w] = 0xA55A00U ^ (static_cast<uint32_t>(w) * 0x13579U) ^ (static_cast<uint32_t>(sf_id) << 8);
        }

        for (int w = 0; w < 10; w++) {
            const bool zero_tail = (w == 1 || w == 9);
            sf_tx_[w] = encode_gps_word(raw_d[w], d29_, d30_, zero_tail);
            d29_ = (sf_tx_[w] >> 1) & 1U;
            d30_ = sf_tx_[w] & 1U;
        }
    }

    int32_t get_sf_bit_sign(uint16_t bit_idx) const {
        const uint32_t w = sf_tx_[bit_idx / 30];
        const uint32_t b = (w >> (29 - (bit_idx % 30))) & 1U;
        return b ? 1 : -1;
    }

    uint8_t prn_{1};
    int8_t code_[1023]{};
    int8_t sin_tbl_[320]{};
    uint32_t carr_ph_{0};
    uint32_t carr_step_{0};
    uint16_t chip_i_{0};
    uint32_t chip_f_{0};
    uint32_t chip_step_{2145386496U};
    uint32_t prng_{0x5A3C96E1U};
    uint8_t ms_in_bit_{0};
    uint16_t bit_in_sf_{0};
    uint8_t sf_id_{1};
    uint32_t tow_z_{0};
    uint32_t d29_{0}, d30_{0};
    uint32_t sf_tx_[10]{};
    int32_t cur_nav_sign_{1};
};

} /* namespace gps_scan */

#endif /* __GPS_TRACK_DSP_H__ */
