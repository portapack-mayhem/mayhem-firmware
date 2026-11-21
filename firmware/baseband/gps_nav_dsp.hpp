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
 * the Free Software Foundation; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GPS_NAV_DSP_H__
#define __GPS_NAV_DSP_H__

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>

#include "gps_track_dsp.hpp"

namespace gps_scan {

static constexpr uint32_t NAV_FS_HZ = 1'024'000;
static constexpr size_t NAV_SAMPLES_PER_MS = 1024;
static constexpr size_t NAV_NUM_BLOCKS = 32;  // 32 blocks x 32 samples = 1024 samples/ms
static constexpr size_t NAV_BLK_LEN = 32;
static constexpr uint32_t NAV_CODE_STEP_NOM = 0xFFC00000U;  // Exact (1023/1024) * 2^32
static constexpr float NAV_HZ_TO_STEP = 4194.304f;          // 2^32 / 1,024,000

// Build 256-entry packed (sin<<16)|(cos&0xFFFF) table with amplitude 127.
static inline void build_nav_cs_table(uint32_t cs_tbl[256]) {
    for (uint32_t i = 0; i < 256; i++) {
        const uint32_t phase11 = i << 3;
        const int16_t c = static_cast<int16_t>((cos_2048(phase11) + 128) >> 8);
        const int16_t s = static_cast<int16_t>((sin_2048(phase11) + 128) >> 8);
        cs_tbl[i] = (static_cast<uint32_t>(static_cast<uint16_t>(s)) << 16) |
                    static_cast<uint16_t>(c);
    }
}

#if defined(__GNUC__)
#define GPS_FORCE_INLINE inline __attribute__((always_inline))
#else
#define GPS_FORCE_INLINE inline
#endif

// Fast 32-bit SWAR popcount (11 single-cycle ALU ops, zero branches, zero memory).
// Must be inlined: the correlator calls it 9x per 32-sample block (~1500x/ms per channel), and an
// out-of-line call under -Os roughly doubles the per-ms tracking cost.
static GPS_FORCE_INLINE uint32_t popcnt32(uint32_t v) {
    v = v - ((v >> 1) & 0x55555555U);
    v = (v & 0x33333333U) + ((v >> 2) & 0x33333333U);
    return (((v + (v >> 4)) & 0x0F0F0F0FU) * 0x01010101U) >> 24;
}

// Build doubled+padded 1023-chip C/A code bit array (2082 bits = 67 uint32_t words = 268 B).
// Bit = 0 => chip +1, bit = 1 => chip -1. Enables single 34-bit fetch for L/P/E with zero modulo!
GPS_NOINLINE static void build_ca_doubled_words(uint8_t prn, uint32_t out_w[67]) {
    uint8_t bits[128];
    generate_ca_bits(prn, bits);
    std::memset(out_w, 0, 67 * sizeof(uint32_t));
    for (uint32_t i = 0; i < 2082U; i++) {
        const uint32_t ci = i % 1023U;
        if ((bits[ci >> 3] >> (ci & 7U)) & 1U) {
            out_w[i >> 5] |= (1U << (i & 31U));
        }
    }
}

// Fetch 32 consecutive C/A code bits starting at idx (0..2048) using pure 32-bit ops (zero 64-bit libgcc!).
static GPS_FORCE_INLINE uint32_t fetch_ca32(const uint32_t* cw, uint32_t idx) {
    const uint32_t w = idx >> 5;
    const uint32_t b = idx & 31U;
    return (b == 0U) ? cw[w] : ((cw[w] >> b) | (cw[w + 1] << (32U - b)));
}

// Shared 2-bit bitplane buffer for 1.000 ms (32 blocks x 32 samples @ 1.024 Msps = 576 bytes!).
// Each sample encodes {-3, -1, +1, +3} via (sgn, mag): val = (1 - 2*sgn) * (1 + 2*mag).
struct NavBitplanes {
    uint32_t i_sgn[NAV_NUM_BLOCKS]{};
    uint32_t i_mag[NAV_NUM_BLOCKS]{};
    uint32_t q_sgn[NAV_NUM_BLOCKS]{};
    uint32_t q_mag[NAV_NUM_BLOCKS]{};
    uint8_t i_mag_cnt[NAV_NUM_BLOCKS]{};
    uint8_t q_mag_cnt[NAV_NUM_BLOCKS]{};
    int16_t mean_i{0};
    int16_t mean_q{0};
    int16_t thresh{16};
    uint16_t cal_cnt{0};

    void reset_cal() {
        mean_i = 0;
        mean_q = 0;
        thresh = 16;
        cal_cnt = 0;
    }

    // Update DC mean & shared 2-bit threshold from 128 decimated samples (~5 us, once per 128 ms).
    GPS_NOINLINE void update_cal_from_raw(const int8_t* raw_2048) {
        int32_t si = 0, sq = 0;
        for (size_t n = 0; n < 128; n++) {
            si += raw_2048[4 * n] + raw_2048[4 * n + 2];
            sq += raw_2048[4 * n + 1] + raw_2048[4 * n + 3];
        }
        const int32_t mi = si >> 7;
        const int32_t mq = sq >> 7;
        int32_t a = 0;
        for (size_t n = 0; n < 128; n++) {
            const int32_t di = (raw_2048[4 * n] + raw_2048[4 * n + 2]) - mi;
            const int32_t dq = (raw_2048[4 * n + 1] + raw_2048[4 * n + 3]) - mq;
            a += (di < 0 ? -di : di) + (dq < 0 ? -dq : dq);
        }
        mean_i = static_cast<int16_t>(mi);
        mean_q = static_cast<int16_t>(mq);
        int32_t t = ((a >> 8) * 5) >> 2;
        if (t < 4) t = 4;
        thresh = static_cast<int16_t>(t);
    }

    // 2x boxcar decimate (2048 int8 -> 1024) + pack directly into 32-bit 2-bit bitplanes (shared across all 5 ch!).
    // Uses zero-spill unsigned range test: |x| >= t <=> uint32_t(x + t - 1) > uint32_t(2*t - 2).
    GPS_NOINLINE void pack_from_raw_2048(const int8_t* __restrict__ raw_2048) {
        if ((cal_cnt++ & 127U) == 0U) {
            update_cal_from_raw(raw_2048);
        }
        const int32_t mi = mean_i;
        const int32_t mq = mean_q;
        const int32_t t_off = static_cast<int32_t>(thresh) - 1;
        const uint32_t t_lim = static_cast<uint32_t>(t_off * 2);

#if defined(__ARM_FEATURE_DSP)
        const uint32_t* __restrict__ p32 = reinterpret_cast<const uint32_t*>(raw_2048);
        for (size_t blk = 0; blk < NAV_NUM_BLOCKS; blk++) {
            uint32_t is_b = 0, im_b = 0, qs_b = 0, qm_b = 0;
            uint32_t bit = 1U;
            for (uint32_t k = 0; k < NAV_BLK_LEN; k++) {
                const uint32_t w = *p32++;
                uint32_t ri, rq;
                __asm__ volatile("sxtb16 %0, %1" : "=r"(ri) : "r"(w));
                __asm__ volatile("sxtb16 %0, %1, ror #8" : "=r"(rq) : "r"(w));
                const int32_t si = static_cast<int16_t>(static_cast<int16_t>(ri) + static_cast<int16_t>(ri >> 16)) - mi;
                const int32_t sq = static_cast<int16_t>(static_cast<int16_t>(rq) + static_cast<int16_t>(rq >> 16)) - mq;

                if (si < 0) is_b |= bit;
                if (static_cast<uint32_t>(si + t_off) > t_lim) im_b |= bit;
                if (sq < 0) qs_b |= bit;
                if (static_cast<uint32_t>(sq + t_off) > t_lim) qm_b |= bit;
                bit <<= 1;
            }
            i_sgn[blk] = is_b;
            i_mag[blk] = im_b;
            q_sgn[blk] = qs_b;
            q_mag[blk] = qm_b;
            i_mag_cnt[blk] = static_cast<uint8_t>(popcnt32(im_b));
            q_mag_cnt[blk] = static_cast<uint8_t>(popcnt32(qm_b));
        }
#else
        for (size_t blk = 0; blk < NAV_NUM_BLOCKS; blk++) {
            uint32_t is_b = 0, im_b = 0, qs_b = 0, qm_b = 0;
            uint32_t bit = 1U;
            for (uint32_t k = 0; k < NAV_BLK_LEN; k++) {
                const int32_t si = static_cast<int32_t>(raw_2048[0] + raw_2048[2]) - mi;
                const int32_t sq = static_cast<int32_t>(raw_2048[1] + raw_2048[3]) - mq;
                raw_2048 += 4;
                if (si < 0) is_b |= bit;
                if (static_cast<uint32_t>(si + t_off) > t_lim) im_b |= bit;
                if (sq < 0) qs_b |= bit;
                if (static_cast<uint32_t>(sq + t_off) > t_lim) qm_b |= bit;
                bit <<= 1;
            }
            i_sgn[blk] = is_b;
            i_mag[blk] = im_b;
            q_sgn[blk] = qs_b;
            q_mag[blk] = qm_b;
            i_mag_cnt[blk] = static_cast<uint8_t>(popcnt32(im_b));
            q_mag_cnt[blk] = static_cast<uint8_t>(popcnt32(qm_b));
        }
#endif
    }
};

// Single real-time 1.024 Msps GPS tracking channel using 32-sample bitwise-parallel correlation (~0.04 ms/ch on M4!).
class GPSFastChannel {
   public:
    void reset() {
        active_ = false;
        prn_ = 0;
        state_ = TrackState::Off;
        nav_.reset();
    }

    void init(uint8_t prn, float init_doppler_hz, float init_code_chip) {
        if (prn < 1 || prn > NUM_PRNS) prn = 1;
        prn_ = prn;
        active_ = true;
        state_ = TrackState::PullIn;

        build_ca_doubled_words(prn_, code_w_);

        while (init_code_chip < 0.0f) init_code_chip += 1023.0f;
        while (init_code_chip >= 1023.0f) init_code_chip -= 1023.0f;
        ch_i_ = static_cast<uint32_t>(init_code_chip);
        if (ch_i_ >= 1023U) ch_i_ = 0U;
        const float frac = init_code_chip - static_cast<float>(ch_i_);
        ch_f_ = static_cast<uint32_t>(frac * 4294967296.0f);

        carr_phase_ = 0;
        carr_freq_hz_ = init_doppler_hz;
        carr_freq_base_ = init_doppler_hz;
        dll_nco_hz_ = 0.0f;
        code_dopp_hz_ = init_doppler_hz * (1.0f / 1540.0f);
        update_nco_steps();

        ip_ = qp_ = id_ = qd_ = 0;
        prev_ip_ = prev_qp_ = 0.0f;
        has_prev_ = false;
        epoch_cnt_ = 0;
        loss_cnt_ = 0;

        pli_i2_ = 0.0f;
        pli_q2_ = 0.0f;
        pli_ = 0.0f;

        cn0_si_ = 0.0f;
        cn0_sq_ = 0.0f;
        cn0_wbp_ = 0.0f;
        cn0_ms_ = 0;
        cn0_np_avg_ = 0.0f;
        cn0_dbhz_ = 0.0f;

        bit_synced_ = false;
        bit_flips_ = 0;
        ms_in_bit_ = 0;
        bit_sum_ = 0.0f;
        std::memset(bit_hist_, 0, sizeof(bit_hist_));
        nav_.reset();
    }

    bool active() const { return active_; }
    uint8_t prn() const { return prn_; }
    TrackState state() const { return state_; }

    // Process one 1.000 ms bitplane buffer (32 blocks x 32 samples).
    // Splits seamlessly at any C/A code epoch wrap (1022 -> 0) for exact epoch-aligned dump.
    // Hot path (5 ch x 1000/s): O2 here cuts register spills ~3x vs the image-wide -Os for ~120 B of code.
    __attribute__((noinline, optimize("O2"))) void process_bitplanes(const NavBitplanes& bp,
                           const uint32_t* cs_tbl,
                           NavSubframe* out_sf,
                           bool& got_sf) {
        if (!active_) return;

        for (size_t blk = 0; blk < NAV_NUM_BLOCKS; blk++) {
            uint32_t is_w = bp.i_sgn[blk];
            uint32_t im_w = bp.i_mag[blk];
            uint32_t qs_w = bp.q_sgn[blk];
            uint32_t qm_w = bp.q_mag[blk];
            uint32_t rem_blk = NAV_BLK_LEN;

            while (rem_blk > 0U) {
                if (ch_i_ >= 1023U) {
                    ch_i_ -= 1023U;
                    on_epoch_dump(out_sf, got_sf);
                    continue;
                }

                const uint32_t rem_chips = 1023U - ch_i_;
                const uint32_t step = (rem_blk <= rem_chips) ? rem_blk : rem_chips;
                const int32_t im_cnt_hint = (step == NAV_BLK_LEN) ? static_cast<int32_t>(bp.i_mag_cnt[blk]) : -1;
                const int32_t qm_cnt_hint = (step == NAV_BLK_LEN) ? static_cast<int32_t>(bp.q_mag_cnt[blk]) : -1;

                corr_sub(is_w, im_w, qs_w, qm_w, step, im_cnt_hint, qm_cnt_hint, cs_tbl);

                rem_blk -= step;
                if (rem_blk > 0U) {
                    is_w >>= step;
                    im_w >>= step;
                    qs_w >>= step;
                    qm_w >>= step;
                }
                if (ch_i_ >= 1023U) {
                    ch_i_ -= 1023U;
                    on_epoch_dump(out_sf, got_sf);
                }
            }
        }
    }

    // Coast across n whole milliseconds of samples that the M4 never saw (DMA buffer overrun).
    // Without this, every dropped buffer silently removes one C/A epoch from the channel's count:
    // the carrier NCO phase jumps by f_dopp * 1 ms and the 20 ms bit boundary shifts by 1 ms, so after
    // a few drops bits straddle data transitions, word parity fails and frame sync / TOW is lost.
    // Here the carrier & code NCOs are advanced by exactly n * 1024 samples, and each C/A epoch crossed
    // is booked (epoch count, bit timing, LNAV bit push) using the last prompt value as a hold.
    GPS_NOINLINE void skip_ms(uint32_t n, NavSubframe* out_sf, bool& got_sf) {
        if (!active_) return;
        while (n-- > 0U) {
            carr_phase_ += static_cast<uint32_t>(carr_step_) * static_cast<uint32_t>(NAV_SAMPLES_PER_MS);
            const uint64_t tot = static_cast<uint64_t>(ch_f_) + static_cast<uint64_t>(code_step_) * NAV_SAMPLES_PER_MS;
            ch_i_ += static_cast<uint32_t>(tot >> 32);
            ch_f_ = static_cast<uint32_t>(tot);
            while (ch_i_ >= 1023U) {
                ch_i_ -= 1023U;
                epoch_cnt_++;
                if (bit_synced_) {
                    bit_sum_ += prev_ip_;
                    if (++ms_in_bit_ >= 20U) {
                        ms_in_bit_ = 0;
                        nav_.push_bit(bit_sum_ >= 0.0f ? 1U : 0U);
                        bit_sum_ = 0.0f;
                        if (out_sf && nav_.consume_completed_subframe(prn_, *out_sf)) got_sf = true;
                    }
                }
            }
        }
        // The C/N0 window and the FLL/bit-flip "previous prompt" now span a gap: restart them.
        cn0_si_ = cn0_sq_ = cn0_wbp_ = 0.0f;
        cn0_ms_ = 0;
        has_prev_ = false;
    }

    void fill_obs(NavChanObs& obs) const {
        obs.prn = prn_;
        obs.state = static_cast<uint8_t>(state_);
        obs.bit_synced = bit_synced_ ? 1 : 0;
        obs.frame_synced = nav_.frame_synced();
        obs.sf_mask = nav_.sf_mask();
        obs.ms_in_bit = ms_in_bit_;
        obs.bit_in_sf = nav_.bit_in_sf();
        obs.cn0_dbhz_x10 = static_cast<int16_t>(cn0_dbhz_ * 10.0f);
        obs.pli_x100 = static_cast<int16_t>(pli_ * 100.0f);
        obs.doppler_hz_x10 = static_cast<int32_t>(carr_freq_hz_ * 10.0f);
        obs.sf_start_tow = nav_.sf_start_tow();
        obs.chip_q20 = (ch_i_ << 20) | (ch_f_ >> 12);
    }

   private:
    static constexpr float kPi = 3.14159265f;
    static constexpr float kTwoPi = 6.28318531f;
    static constexpr float kT = 0.001f;

    // 32-sample bitwise-parallel correlator (inlined into process_bitplanes: no 8-arg call per block).
    GPS_FORCE_INLINE void corr_sub(uint32_t is_w,
                               uint32_t im_w,
                               uint32_t qs_w,
                               uint32_t qm_w,
                               uint32_t len,
                               int32_t im_cnt_hint,
                               int32_t qm_cnt_hint,
                               const uint32_t* __restrict__ cs_tbl) {
        const uint32_t mask = (len >= 32U) ? 0xFFFFFFFFU : ((1U << len) - 1U);
        is_w &= mask;
        im_w &= mask;
        qs_w &= mask;
        qm_w &= mask;

        // Pure 32-bit code fetches: only need Prompt + 1 shifted word (E when half=1, L when half=0)!
        const uint32_t cp = fetch_ca32(code_w_, ch_i_) & mask;
        const uint32_t half = ch_f_ >> 31;
        const uint32_t other = fetch_ca32(code_w_, half ? (ch_i_ + 1U) : (ch_i_ + 1022U)) & mask;
        const uint32_t ce = half ? other : cp;
        const uint32_t cl = half ? cp : other;

        const uint32_t imc = (im_cnt_hint >= 0) ? static_cast<uint32_t>(im_cnt_hint) : popcnt32(im_w);
        const uint32_t qmc = (qm_cnt_hint >= 0) ? static_cast<uint32_t>(qm_cnt_hint) : popcnt32(qm_w);

        // 2-bit Prompt I/Q correlation
        const uint32_t xi = (is_w ^ cp) & mask;
        const uint32_t xq = (qs_w ^ cp) & mask;
        const int32_t sip = static_cast<int32_t>(len + (imc << 1) - (popcnt32(xi) << 1) - (popcnt32(xi & im_w) << 2));
        const int32_t sqp = static_cast<int32_t>(len + (qmc << 1) - (popcnt32(xq) << 1) - (popcnt32(xq & qm_w) << 2));

        // 1-bit Diff D = (E - L)/2 correlation (scaled x2 to match 2-bit Prompt gain)
        const uint32_t dm = (ce ^ cl) & mask;
        const uint32_t dm_cnt = popcnt32(dm);
        const int32_t sid = static_cast<int32_t>(dm_cnt - (popcnt32((is_w ^ ce) & dm) << 1)) << 1;
        const int32_t sqd = static_cast<int32_t>(dm_cnt - (popcnt32((qs_w ^ ce) & dm) << 1)) << 1;

        // Midpoint block carrier wipe-off
        const uint32_t c_st = static_cast<uint32_t>(carr_step_);
        const uint32_t mid_ph = carr_phase_ + (len >> 1) * c_st;
        carr_phase_ += len * c_st;

        const uint32_t cs = cs_tbl[mid_ph >> 24];
        const int32_t c = static_cast<int16_t>(cs & 0xFFFFU);
        const int32_t s = static_cast<int16_t>(cs >> 16);

        ip_ += sip * c + sqp * s;
        qp_ += sqp * c - sip * s;
        id_ += sid * c + sqd * s;
        qd_ += sqd * c - sid * s;

        // Exact 64-bit code phase advance
        const uint64_t tot = static_cast<uint64_t>(ch_f_) + static_cast<uint64_t>(code_step_) * len;
        ch_i_ += static_cast<uint32_t>(tot >> 32);
        ch_f_ = static_cast<uint32_t>(tot);
    }

    void update_nco_steps() {
        carr_step_ = static_cast<int32_t>(carr_freq_hz_ * NAV_HZ_TO_STEP);
        const int32_t delta = static_cast<int32_t>(code_dopp_hz_ * NAV_HZ_TO_STEP);
        code_step_ = static_cast<uint32_t>(static_cast<int32_t>(NAV_CODE_STEP_NOM) + delta);
    }

    void on_epoch_dump(NavSubframe* out_sf, bool& got_sf) {
        const float fp_i = static_cast<float>(ip_) * (1.0f / 256.0f);
        const float fp_q = static_cast<float>(qp_) * (1.0f / 256.0f);
        const float fd_i = static_cast<float>(id_) * (1.0f / 256.0f);
        const float fd_q = static_cast<float>(qd_) * (1.0f / 256.0f);
        ip_ = qp_ = id_ = qd_ = 0;

        epoch_cnt_++;

        // 1. Carrier loop (2-stage FLL pull-in -> Wide Costas PLL -> Tight Costas PLL)
        if (epoch_cnt_ <= 350U) {
            state_ = TrackState::PullIn;
            if (has_prev_) {
                const float dot = prev_ip_ * fp_i + prev_qp_ * fp_q;
                const float cross = prev_ip_ * fp_q - prev_qp_ * fp_i;
                float ang = fast_atan2f(cross, dot);
                if (ang > kPi * 0.5f) ang -= kPi;
                if (ang < -kPi * 0.5f) ang += kPi;
                const float f_err = ang / (kTwoPi * kT);
                const float fll_gain = (epoch_cnt_ <= 200U) ? 0.14f : 0.045f;
                carr_freq_base_ += fll_gain * f_err;
                carr_freq_hz_ = carr_freq_base_;
            }
        } else {
            const bool wide = (epoch_cnt_ <= 900U);
            if (state_ != TrackState::Lost) {
                state_ = wide ? TrackState::WidePLL : TrackState::Locked;
            }
            const float ph_err = fast_costas_atan(fp_q, fp_i);
            const float wn = wide ? 34.0f : 18.9f;
            const float kp = (1.4142f * wn) / kTwoPi;
            const float ki = (wn * wn * kT) / kTwoPi;
            carr_freq_base_ += ki * ph_err;
            carr_freq_hz_ = carr_freq_base_ + kp * ph_err;
        }

        if (carr_freq_base_ > 50000.0f) carr_freq_base_ = 50000.0f;
        if (carr_freq_base_ < -50000.0f) carr_freq_base_ = -50000.0f;
        if (carr_freq_hz_ > 50000.0f) carr_freq_hz_ = 50000.0f;
        if (carr_freq_hz_ < -50000.0f) carr_freq_hz_ = -50000.0f;

        // 2. Carrier-Aided Dot-Product DLL: D = (Ip*Id + Qp*Qd) / (Ip^2 + Qp^2)
        const float p2 = fp_i * fp_i + fp_q * fp_q;
        float code_err = 0.0f;
        if (p2 > 0.05f) {
            code_err = (fp_i * fd_i + fp_q * fd_q) / p2;
            if (code_err > 0.5f) code_err = 0.5f;
            if (code_err < -0.5f) code_err = -0.5f;
        }

        const bool tight_dll = (epoch_cnt_ > 900U);
        const float d_wn = tight_dll ? 1.32f : 2.83f;
        const float d_kp = 1.4142f * d_wn;
        const float d_ki = d_wn * d_wn * kT;
        dll_nco_hz_ += d_ki * code_err;
        if (dll_nco_hz_ > 25.0f) dll_nco_hz_ = 25.0f;
        if (dll_nco_hz_ < -25.0f) dll_nco_hz_ = -25.0f;

        code_dopp_hz_ = carr_freq_hz_ * (1.0f / 1540.0f) + dll_nco_hz_ + d_kp * code_err;
        update_nco_steps();

        // 3. Phase Lock Indicator (PLI)
        pli_i2_ = 0.96f * pli_i2_ + 0.04f * (fp_i * fp_i);
        pli_q2_ = 0.96f * pli_q2_ + 0.04f * (fp_q * fp_q);
        const float pli_den = pli_i2_ + pli_q2_;
        pli_ = (pli_den > 0.05f) ? ((pli_i2_ - pli_q2_) / pli_den) : 0.0f;

        if (epoch_cnt_ > 1200U) {
            if (pli_ < 0.15f) {
                loss_cnt_++;
                if (loss_cnt_ > 600U) state_ = TrackState::Lost;
            } else {
                loss_cnt_ = 0;
                state_ = TrackState::Locked;
            }
        }

        // 4. Epoch-Aligned 20 ms Bit Sync + 50 bps LNAV Decode
        if (!bit_synced_) {
            if (epoch_cnt_ > 600U && pli_ > 0.25f) {
                if (has_prev_ && ((fp_i * prev_ip_) < 0.0f)) {
                    const uint32_t b = epoch_cnt_ % 20U;
                    bit_hist_[b]++;
                    bit_flips_++;

                    if (bit_flips_ >= 10U) {
                        uint16_t top1 = 0, top2 = 0;
                        const uint8_t best_b = find_bit_hist_peak(bit_hist_, top1, top2);
                        if (top1 >= 8U && top1 >= static_cast<uint16_t>(top2 * 2U + 3U)) {
                            bit_synced_ = true;
                            bit_sum_ = 0.0f;
                            ms_in_bit_ = static_cast<uint8_t>((epoch_cnt_ + 21U - best_b) % 20U);
                            cn0_si_ = cn0_sq_ = cn0_wbp_ = 0.0f;
                            cn0_ms_ = 0;
                        } else if (bit_flips_ >= 60U) {
                            std::memset(bit_hist_, 0, sizeof(bit_hist_));
                            bit_flips_ = 0;
                        }
                    }
                }
            }
        } else {
            bit_sum_ += fp_i;
            ms_in_bit_++;
            if (ms_in_bit_ >= 20U) {
                ms_in_bit_ = 0;
                nav_.push_bit(bit_sum_ >= 0.0f ? 1U : 0U);
                bit_sum_ = 0.0f;
                if (out_sf && nav_.consume_completed_subframe(prn_, *out_sf)) {
                    got_sf = true;
                }
            }
        }

        // 5. Van Dierendonck C/N0 Estimator (aligned to 20 ms bit window)
        cn0_si_ += fp_i;
        cn0_sq_ += fp_q;
        cn0_wbp_ += p2;
        cn0_ms_++;
        const bool cn0_dump = bit_synced_ ? (ms_in_bit_ == 0U && cn0_ms_ > 0U) : (cn0_ms_ >= 20U);
        if (cn0_dump) {
            if (cn0_ms_ == 20U && cn0_wbp_ > 0.05f) {
                const float nbp = cn0_si_ * cn0_si_ + cn0_sq_ * cn0_sq_;
                const float np = nbp / cn0_wbp_;
                if (cn0_np_avg_ <= 0.1f) {
                    cn0_np_avg_ = np;
                } else {
                    cn0_np_avg_ = 0.85f * cn0_np_avg_ + 0.15f * np;
                }
                cn0_dbhz_ = np_to_cn0_dbhz(cn0_np_avg_);
            }
            cn0_si_ = cn0_sq_ = cn0_wbp_ = 0.0f;
            cn0_ms_ = 0;
        }

        prev_ip_ = fp_i;
        prev_qp_ = fp_q;
        has_prev_ = true;
    }

    bool active_{false};
    uint8_t prn_{0};
    TrackState state_{TrackState::Off};

    uint32_t code_w_[67]{};  // Doubled+padded 1023-chip C/A code (268 B)

    uint32_t ch_i_{0};
    uint32_t ch_f_{0};
    uint32_t code_step_{NAV_CODE_STEP_NOM};
    float code_dopp_hz_{0.0f};
    float dll_nco_hz_{0.0f};

    uint32_t carr_phase_{0};
    int32_t carr_step_{0};
    float carr_freq_hz_{0.0f};
    float carr_freq_base_{0.0f};

    int32_t ip_{0}, qp_{0}, id_{0}, qd_{0};
    float prev_ip_{0.0f}, prev_qp_{0.0f};
    bool has_prev_{false};
    uint32_t epoch_cnt_{0};
    uint16_t loss_cnt_{0};

    float pli_i2_{0.0f}, pli_q2_{0.0f}, pli_{0.0f};
    float cn0_si_{0.0f}, cn0_sq_{0.0f}, cn0_wbp_{0.0f};
    uint8_t cn0_ms_{0};
    float cn0_np_avg_{0.0f}, cn0_dbhz_{0.0f};

    bool bit_synced_{false};
    uint16_t bit_hist_[20]{};
    uint16_t bit_flips_{0};
    uint8_t ms_in_bit_{0};
    float bit_sum_{0.0f};

    GPSNavDecoder nav_{};
};

// 5-Channel Simultaneous 32-Sample Bitwise-Parallel GPS Navigation Bank (~4.4 KiB RAM, ~0.22 ms/ms on M4!)
class GPSNavBank {
   public:
    void init_tables() {
        build_nav_cs_table(cs_tbl_);
        bp_.reset_cal();
        num_ch_ = 0;
        ms_tracked_ = 0;
        for (size_t i = 0; i < MAX_NAV_CH; i++) ch_[i].reset();
    }

    void init_channel(size_t idx, uint8_t prn, float init_doppler_hz, float init_code_chip) {
        if (idx >= MAX_NAV_CH) return;
        ch_[idx].init(prn, init_doppler_hz, init_code_chip);
        if (idx + 1 > num_ch_) num_ch_ = static_cast<uint8_t>(idx + 1);
    }

    void pack_raw_2048(const int8_t* raw_2048) {
        bp_.pack_from_raw_2048(raw_2048);
    }

    size_t process_bitplanes(NavSubframe out_sfs[MAX_NAV_CH]) {
        ms_tracked_++;
        size_t sf_cnt = 0;
        for (size_t i = 0; i < num_ch_; i++) {
            bool got_sf = false;
            ch_[i].process_bitplanes(bp_, cs_tbl_, &out_sfs[sf_cnt], got_sf);
            if (got_sf) sf_cnt++;
        }
        return sf_cnt;
    }

    // Coast all channels across n dropped 1 ms DMA buffers (see GPSFastChannel::skip_ms).
    size_t skip_ms(uint32_t n, NavSubframe out_sfs[MAX_NAV_CH]) {
        ms_tracked_ += n;
        size_t sf_cnt = 0;
        for (size_t i = 0; i < num_ch_; i++) {
            bool got_sf = false;
            ch_[i].skip_ms(n, &out_sfs[sf_cnt], got_sf);
            if (got_sf) sf_cnt++;
        }
        return sf_cnt;
    }

    void fill_obs_packet(NavObsPacket& pkt,
                         uint8_t phase,
                         uint8_t reacq_idx,
                         uint8_t is_selftest,
                         uint16_t dma_drops,
                         uint16_t cpu_us_pack,
                         uint16_t cpu_us_nav) const {
        pkt.num_ch = num_ch_;
        pkt.phase = phase;
        pkt.reacq_idx = reacq_idx;
        pkt.is_selftest = is_selftest;
        pkt.track_sec = static_cast<uint16_t>(ms_tracked_ / 1000U);
        pkt.dma_drops = dma_drops;
        pkt.cpu_us_pack = cpu_us_pack;
        pkt.cpu_us_nav = cpu_us_nav;
        for (size_t i = 0; i < num_ch_ && i < MAX_NAV_CH; i++) {
            ch_[i].fill_obs(pkt.ch[i]);
        }
    }

    NavBitplanes& bitplanes() { return bp_; }
    const uint32_t* cs_tbl() const { return cs_tbl_; }
    uint8_t num_ch() const { return num_ch_; }

   private:
    uint32_t cs_tbl_[256]{};
    NavBitplanes bp_{};
    GPSFastChannel ch_[MAX_NAV_CH]{};
    uint8_t num_ch_{0};
    uint32_t ms_tracked_{0};
};

// Pre-computed 5-SV Keplerian ephemeris & trajectory table for Mode::NavFixSelfTest
// True receiver location: (37.00000° N, -123.00000° E, 0.0 m) -- an arbitrary point in open Pacific water,
// t_start = 539998.0 s (2.0s before SF1). Per-SV init_* / Doppler fields are derived from the ephemeris words
// below by ranging to that point (light time + Sagnac, same model as solve_pvt).
struct SimNavSvSpec {
    uint8_t prn;
    uint32_t init_sf_tow;
    uint16_t init_bit;
    uint8_t init_ms;
    float init_chip;
    float fd0_hz;
    float fdot_hz_s;
    uint32_t sf1_w7;
    uint32_t sf1_w9;
    uint32_t sf2_w[8];
    uint32_t sf3_w[8];
};

static constexpr SimNavSvSpec kSimNavConstellation[MAX_NAV_CH] = {
    /* PRN  1 */ {1, 539994U, 196U, 12U, 712.784102f, -721.5075f, -0.52820f,
                  0x2A83D6U, 0x0FBA88U,
                  {0x2A0190U, 0x2DF027U, 0x1C71C7U, 0x021902U, 0x0C49BAU, 0x0432A1U, 0x0D3333U, 0x83D600U},
                  {0x0000D6U, 0x2884D5U, 0x000027U, 0x1C71C7U, 0x1B8000U, 0x000000U, 0xFFA881U, 0x2A0460U}},
    /* PRN  7 */ {7, 539994U, 196U, 2U, 107.463238f, 625.1476f, -0.58689f,
                  0x2B83D6U, 0xF583A4U,
                  {0x2B0190U, 0x2DF018U, 0xE38E39U, 0x021902U, 0xD0E560U, 0x0432A1U, 0x0D3333U, 0x83D600U},
                  {0x00000BU, 0x7DDA2AU, 0x000027U, 0x1C71C7U, 0x1B8000U, 0x000000U, 0xFFA881U, 0x2B0460U}},
    /* PRN 13 */ {13, 539994U, 196U, 4U, 472.122262f, -2844.3975f, -0.36681f,
                  0x2C83D6U, 0x068DB8U,
                  {0x2C0190U, 0x2DF03AU, 0x4FA4FAU, 0x021901U, 0x89374CU, 0x0432A1U, 0x0D3333U, 0x83D600U},
                  {0x0000E4U, 0x616863U, 0x000027U, 0x1C71C7U, 0x1B8000U, 0x000000U, 0xFFA881U, 0x2C0460U}},
    /* PRN 19 */ {19, 539994U, 196U, 6U, 5.800157f, 1967.6394f, -0.24209f,
                  0x2D83D6U, 0xEC56D4U,
                  {0x2D0190U, 0x2DF018U, 0xE38E39U, 0x021903U, 0x2CA57AU, 0x0432A1U, 0x0D3333U, 0x83D600U},
                  {0x0000B9U, 0xB6BDB8U, 0x000027U, 0x1C71C7U, 0x1B8000U, 0x000000U, 0xFFA881U, 0x2D0460U}},
    /* PRN 27 */ {27, 539994U, 196U, 10U, 520.627652f, 2016.1311f, -0.56488f,
                  0x2E83D6U, 0x0BCBE8U,
                  {0x2E0190U, 0x2DF00AU, 0xAAAAABU, 0x021902U, 0x4DD2F2U, 0x0432A1U, 0x0D3333U, 0x83D600U},
                  {0x0000EBU, 0x7DDA2AU, 0x000027U, 0x1C71C7U, 0x1B8000U, 0x000000U, 0xFFA881U, 0x2E0460U}},
};

// Ultra-fast 32-sample packed-SIMD 5-SV GPS LNAV simulator (~1.7 KiB RAM, ~0.25 ms/ms on M4!)
class GPSNavSim {
   public:
    void init() {
        prng_ = 0x51A9C3F1U;
        ms_elapsed_ = 0;
        for (size_t i = 0; i < MAX_NAV_CH; i++) {
            const auto& sp = kSimNavConstellation[i];
            auto& s = sv_[i];
            build_ca_doubled_words(sp.prn, s.code_w);
            s.sf_tow = sp.init_sf_tow;
            s.sf_id = static_cast<uint8_t>(((sp.init_sf_tow / 6U) % 5U) + 1U);
            s.bit_in_sf = sp.init_bit;
            s.ms_in_bit = sp.init_ms;
            s.ch_i = static_cast<uint32_t>(sp.init_chip);
            const float frac = sp.init_chip - static_cast<float>(s.ch_i);
            s.ch_f = static_cast<uint32_t>(frac * 4294967296.0f);
            s.carr_phase = static_cast<uint32_t>(i * 0x31415926U);
            s.f_dopp = sp.fd0_hz;
            s.f_dot = sp.fdot_hz_s;
            build_subframe(i, s.sf_id, s.sf_tow);
        }
    }

    // Generate 1.000 ms (32 blocks x 32 samples) directly into NavBitplanes using packed int16 SIMD!
    GPS_NOINLINE void generate_bitplanes_ms(NavBitplanes& bp, const uint32_t* __restrict__ cs_tbl) {
        uint32_t sv_cst[MAX_NAV_CH];
        uint32_t sv_chst[MAX_NAV_CH];
        const float t_sec = static_cast<float>(ms_elapsed_) * 0.001f;
        for (size_t i = 0; i < MAX_NAV_CH; i++) {
            const float f_now = sv_[i].f_dopp + sv_[i].f_dot * t_sec;
            sv_cst[i] = static_cast<uint32_t>(static_cast<int32_t>(f_now * NAV_HZ_TO_STEP));
            const int32_t code_delta = static_cast<int32_t>((f_now * (1.0f / 1540.0f)) * NAV_HZ_TO_STEP);
            sv_chst[i] = static_cast<uint32_t>(static_cast<int32_t>(NAV_CODE_STEP_NOM) + code_delta);
        }

        uint32_t r = prng_;
        uint32_t iq_blk[NAV_BLK_LEN];  // Packed (int16 Q << 16) | (uint16 I)

        for (size_t blk = 0; blk < NAV_NUM_BLOCKS; blk++) {
            // 1. Fast zero-mean noise (sigma ~ 48) packed into (Q<<16 | I)
            for (size_t k = 0; k < NAV_BLK_LEN; k++) {
                r ^= r << 13;
                r ^= r >> 17;
                r ^= r << 5;
                const int16_t ni = static_cast<int16_t>(static_cast<int32_t>((r & 0x7FU) + ((r >> 7) & 0x7FU)) - 128);
                const int16_t nq = static_cast<int16_t>(static_cast<int32_t>(((r >> 14) & 0x7FU) + ((r >> 21) & 0x7FU)) - 128);
                iq_blk[k] = (static_cast<uint32_t>(static_cast<uint16_t>(nq)) << 16) | static_cast<uint16_t>(ni);
            }

            // 2. Superimpose all 5 SVs using midpoint block carrier + packed SADD16/SSUB16
            for (size_t i = 0; i < MAX_NAV_CH; i++) {
                auto& s = sv_[i];
                const uint32_t mid_ph = s.carr_phase + 16U * sv_cst[i];
                s.carr_phase += 32U * sv_cst[i];
                const uint32_t cs = cs_tbl[mid_ph >> 24];
                const int16_t ci0 = static_cast<int16_t>(static_cast<int16_t>(cs & 0xFFFFU) >> 3);
                const int16_t sq0 = static_cast<int16_t>(static_cast<int16_t>(cs >> 16) >> 3);

                uint32_t c32 = fetch_ca32(s.code_w, s.ch_i);

                int32_t bsign = cur_bit_sign(s);
                int16_t ci = (bsign < 0) ? static_cast<int16_t>(-ci0) : ci0;
                int16_t sq = (bsign < 0) ? static_cast<int16_t>(-sq0) : sq0;
                const uint32_t cs_pk = (static_cast<uint32_t>(static_cast<uint16_t>(sq)) << 16) | static_cast<uint16_t>(ci);

                const uint64_t tot = static_cast<uint64_t>(s.ch_f) + static_cast<uint64_t>(sv_chst[i]) * NAV_BLK_LEN;
                const uint32_t next_ch_i = s.ch_i + static_cast<uint32_t>(tot >> 32);
                s.ch_f = static_cast<uint32_t>(tot);
                if (next_ch_i >= 1023U) {
                    // C/A epoch ends inside (or exactly at the end of) this block: samples k >= wrap_k belong to
                    // the next epoch. If the data bit flips there, fold the sign change into the chip mask.
                    const uint32_t wrap_k = 1023U - s.ch_i;
                    s.ch_i = next_ch_i - 1023U;
                    const int32_t old_sign = bsign;
                    advance_epoch(i, s, bsign, ci, sq, ci0, sq0);
                    if (bsign != old_sign && wrap_k < NAV_BLK_LEN) {
                        c32 ^= 0xFFFFFFFFU << wrap_k;
                    }
                } else {
                    s.ch_i = next_ch_i;
                }

                // Branch-free tight loop (keeps c32 / pointer / value in registers; ~2x faster than the old
                // per-sample wrap test, which pushed the Test-mode M4 load past 1 ms and caused DMA drops).
                uint32_t* p = iq_blk;
                for (uint32_t k = 0; k < NAV_BLK_LEN; k++) {
#if defined(__ARM_FEATURE_DSP)
                    uint32_t cur = *p;
                    __asm__(
                        "tst %[c], #1\n\t"
                        "ite ne\n\t"
                        "ssub16ne %[cur], %[cur], %[v]\n\t"
                        "sadd16eq %[cur], %[cur], %[v]"
                        : [cur] "+r"(cur)
                        : [c] "r"(c32), [v] "r"(cs_pk)
                        : "cc");
                    *p++ = cur;
#else
                    const int16_t vi = static_cast<int16_t>(cs_pk & 0xFFFFU);
                    const int16_t vq = static_cast<int16_t>(cs_pk >> 16);
                    int16_t ai = static_cast<int16_t>(*p & 0xFFFFU);
                    int16_t aq = static_cast<int16_t>(*p >> 16);
                    if (c32 & 1U) {
                        ai = static_cast<int16_t>(ai - vi);
                        aq = static_cast<int16_t>(aq - vq);
                    } else {
                        ai = static_cast<int16_t>(ai + vi);
                        aq = static_cast<int16_t>(aq + vq);
                    }
                    *p++ = (static_cast<uint32_t>(static_cast<uint16_t>(aq)) << 16) | static_cast<uint16_t>(ai);
#endif
                    c32 >>= 1;
                }
            }

            // 3. Quantize 32 packed samples to 2-bit bitplanes (thresh = 52)
            uint32_t is_b = 0, im_b = 0, qs_b = 0, qm_b = 0;
            uint32_t bit = 1U;
            for (uint32_t k = 0; k < NAV_BLK_LEN; k++) {
                const uint32_t v = iq_blk[k];
                const int32_t vi = static_cast<int16_t>(v & 0xFFFFU);
                const int32_t vq = static_cast<int16_t>(v >> 16);
                if (vi < 0) is_b |= bit;
                if (static_cast<uint32_t>(vi + 51) > 102U) im_b |= bit;
                if (vq < 0) qs_b |= bit;
                if (static_cast<uint32_t>(vq + 51) > 102U) qm_b |= bit;
                bit <<= 1;
            }
            bp.i_sgn[blk] = is_b;
            bp.i_mag[blk] = im_b;
            bp.q_sgn[blk] = qs_b;
            bp.q_mag[blk] = qm_b;
            bp.i_mag_cnt[blk] = static_cast<uint8_t>(popcnt32(im_b));
            bp.q_mag_cnt[blk] = static_cast<uint8_t>(popcnt32(qm_b));
        }

        prng_ = r;
        ms_elapsed_++;
    }

   private:
    struct SvStateSim {
        uint32_t code_w[67]{};
        uint32_t sf_words[10]{};
        uint32_t sf_tow{0};
        uint8_t sf_id{1};
        uint16_t bit_in_sf{0};
        uint8_t ms_in_bit{0};
        uint32_t ch_i{0};
        uint32_t ch_f{0};
        uint32_t carr_phase{0};
        float f_dopp{0.0f};
        float f_dot{0.0f};
    };

    static inline int32_t cur_bit_sign(const SvStateSim& s) {
        const uint32_t w = s.sf_words[s.bit_in_sf / 30U];
        const uint32_t b = (w >> (29U - (s.bit_in_sf % 30U))) & 1U;
        return b ? 1 : -1;
    }

    GPS_NOINLINE void advance_epoch(size_t i, SvStateSim& s, int32_t& bsign, int16_t& ci, int16_t& sq, int16_t ci0, int16_t sq0) {
        s.ms_in_bit++;
        if (s.ms_in_bit >= 20U) {
            s.ms_in_bit = 0;
            s.bit_in_sf++;
            if (s.bit_in_sf >= 300U) {
                s.bit_in_sf = 0;
                s.sf_tow = (s.sf_tow + 6U) % 604800U;
                s.sf_id = (s.sf_id >= 5U) ? 1U : static_cast<uint8_t>(s.sf_id + 1U);
                build_subframe(i, s.sf_id, s.sf_tow);
            }
            bsign = cur_bit_sign(s);
            ci = (bsign < 0) ? static_cast<int16_t>(-ci0) : ci0;
            sq = (bsign < 0) ? static_cast<int16_t>(-sq0) : sq0;
        }
    }

    GPS_NOINLINE void build_subframe(size_t idx, uint8_t sf_id, uint32_t sf_start_tow) {
        auto& s = sv_[idx];
        const auto& sp = kSimNavConstellation[idx];
        uint32_t d[10]{};
        d[0] = (0x8BU << 16) | 0x001234U;
        const uint32_t next_z = ((sf_start_tow / 6U) + 1U) % 100800U;
        d[1] = (next_z << 7) | (static_cast<uint32_t>(sf_id) << 2);

        if (sf_id == 1) {
            d[2] = 0x635000U;
            d[6] = 0x0000F5U;
            d[7] = sp.sf1_w7;
            d[8] = 0x000012U;
            d[9] = sp.sf1_w9;
        } else if (sf_id == 2) {
            std::memcpy(&d[2], sp.sf2_w, 8 * sizeof(uint32_t));
        } else if (sf_id == 3) {
            std::memcpy(&d[2], sp.sf3_w, 8 * sizeof(uint32_t));
        } else {
            for (int k = 2; k < 10; k++) {
                d[k] = (static_cast<uint32_t>(sf_id) << 20) ^ (static_cast<uint32_t>(k) * 0x13579BU);
            }
        }

        uint32_t d29 = 0, d30 = 0;
        for (int k = 0; k < 10; k++) {
            const bool force_z = (k == 1 || k == 9);
            const uint32_t w30 = encode_gps_word(d[k], d29, d30, force_z);
            s.sf_words[k] = w30;
            d29 = (w30 >> 1) & 1U;
            d30 = w30 & 1U;
        }
    }

    SvStateSim sv_[MAX_NAV_CH]{};
    uint32_t prng_{0x51A9C3F1U};
    uint32_t ms_elapsed_{0};
};

} /* namespace gps_scan */

#endif /* __GPS_NAV_DSP_H__ */
