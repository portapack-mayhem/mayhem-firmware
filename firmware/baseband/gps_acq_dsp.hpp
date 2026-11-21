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

#ifndef __GPS_ACQ_DSP_H__
#define __GPS_ACQ_DSP_H__

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>

#include "gps_scan_types.hpp"

namespace gps_scan {

struct c8_t {
    int8_t re;
    int8_t im;
};

struct c16_t {
    int16_t re;
    int16_t im;
};

// Quarter-wave sine table: round(32767 * sin(2*pi*k/2048)) for k = 0..512 (1026 bytes)
static constexpr int16_t kSinQ2048[513] = {
        0,   101,   201,   302,   402,   503,   603,   704,   804,   905,  1005,  1106,  1206,  1307,  1407,  1507,
     1608,  1708,  1809,  1909,  2009,  2110,  2210,  2310,  2410,  2511,  2611,  2711,  2811,  2911,  3012,  3112,
     3212,  3312,  3412,  3512,  3612,  3712,  3811,  3911,  4011,  4111,  4210,  4310,  4410,  4509,  4609,  4708,
     4808,  4907,  5007,  5106,  5205,  5305,  5404,  5503,  5602,  5701,  5800,  5899,  5998,  6096,  6195,  6294,
     6393,  6491,  6590,  6688,  6786,  6885,  6983,  7081,  7179,  7277,  7375,  7473,  7571,  7669,  7767,  7864,
     7962,  8059,  8157,  8254,  8351,  8448,  8545,  8642,  8739,  8836,  8933,  9030,  9126,  9223,  9319,  9416,
     9512,  9608,  9704,  9800,  9896,  9992, 10087, 10183, 10278, 10374, 10469, 10564, 10659, 10754, 10849, 10944,
    11039, 11133, 11228, 11322, 11417, 11511, 11605, 11699, 11793, 11886, 11980, 12074, 12167, 12260, 12353, 12446,
    12539, 12632, 12725, 12817, 12910, 13002, 13094, 13187, 13279, 13370, 13462, 13554, 13645, 13736, 13828, 13919,
    14010, 14101, 14191, 14282, 14372, 14462, 14553, 14643, 14732, 14822, 14912, 15001, 15090, 15180, 15269, 15358,
    15446, 15535, 15623, 15712, 15800, 15888, 15976, 16063, 16151, 16238, 16325, 16413, 16499, 16586, 16673, 16759,
    16846, 16932, 17018, 17104, 17189, 17275, 17360, 17445, 17530, 17615, 17700, 17784, 17869, 17953, 18037, 18121,
    18204, 18288, 18371, 18454, 18537, 18620, 18703, 18785, 18868, 18950, 19032, 19113, 19195, 19276, 19357, 19438,
    19519, 19600, 19680, 19761, 19841, 19921, 20000, 20080, 20159, 20238, 20317, 20396, 20475, 20553, 20631, 20709,
    20787, 20865, 20942, 21019, 21096, 21173, 21250, 21326, 21403, 21479, 21554, 21630, 21705, 21781, 21856, 21930,
    22005, 22079, 22154, 22227, 22301, 22375, 22448, 22521, 22594, 22667, 22739, 22812, 22884, 22956, 23027, 23099,
    23170, 23241, 23311, 23382, 23452, 23522, 23592, 23662, 23731, 23801, 23870, 23938, 24007, 24075, 24143, 24211,
    24279, 24346, 24413, 24480, 24547, 24613, 24680, 24746, 24811, 24877, 24942, 25007, 25072, 25137, 25201, 25265,
    25329, 25393, 25456, 25519, 25582, 25645, 25708, 25770, 25832, 25893, 25955, 26016, 26077, 26138, 26198, 26259,
    26319, 26378, 26438, 26497, 26556, 26615, 26674, 26732, 26790, 26848, 26905, 26962, 27019, 27076, 27133, 27189,
    27245, 27300, 27356, 27411, 27466, 27521, 27575, 27629, 27683, 27737, 27790, 27843, 27896, 27949, 28001, 28053,
    28105, 28157, 28208, 28259, 28310, 28360, 28411, 28460, 28510, 28560, 28609, 28658, 28706, 28755, 28803, 28850,
    28898, 28945, 28992, 29039, 29085, 29131, 29177, 29223, 29268, 29313, 29358, 29403, 29447, 29491, 29534, 29578,
    29621, 29664, 29706, 29749, 29791, 29832, 29874, 29915, 29956, 29997, 30037, 30077, 30117, 30156, 30195, 30234,
    30273, 30311, 30349, 30387, 30424, 30462, 30498, 30535, 30571, 30607, 30643, 30679, 30714, 30749, 30783, 30818,
    30852, 30885, 30919, 30952, 30985, 31017, 31050, 31082, 31113, 31145, 31176, 31206, 31237, 31267, 31297, 31327,
    31356, 31385, 31414, 31442, 31470, 31498, 31526, 31553, 31580, 31607, 31633, 31659, 31685, 31710, 31736, 31760,
    31785, 31809, 31833, 31857, 31880, 31903, 31926, 31949, 31971, 31993, 32014, 32036, 32057, 32077, 32098, 32118,
    32137, 32157, 32176, 32195, 32213, 32232, 32250, 32267, 32285, 32302, 32318, 32335, 32351, 32367, 32382, 32397,
    32412, 32427, 32441, 32455, 32469, 32482, 32495, 32508, 32521, 32533, 32545, 32556, 32567, 32578, 32589, 32599,
    32609, 32619, 32628, 32637, 32646, 32655, 32663, 32671, 32678, 32685, 32692, 32699, 32705, 32711, 32717, 32722,
    32728, 32732, 32737, 32741, 32745, 32748, 32752, 32755, 32757, 32759, 32761, 32763, 32765, 32766, 32766, 32767,
    32767,
};

// IS-GPS-200 Table 3-Ia: G2 phase selector taps (1-based) for PRN 1..32
static constexpr uint8_t kG2Taps[NUM_PRNS][2] = {
    {2, 6},   {3, 7},   {4, 8},   {5, 9},   {1, 9},   {2, 10},  {1, 8},   {2, 9},
    {3, 10},  {2, 3},   {3, 4},   {5, 6},   {6, 7},   {7, 8},   {8, 9},   {9, 10},
    {1, 4},   {2, 5},   {3, 6},   {4, 7},   {5, 8},   {6, 9},   {1, 3},   {4, 6},
    {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 6},   {2, 7},   {3, 8},   {4, 9},
};

static constexpr int8_t kQ2Levels[4] = {-3, -1, 1, 3};

static inline int16_t sin_2048(uint32_t idx) {
    idx &= 2047U;
    const uint32_t q = idx >> 9;
    const uint32_t r = idx & 511U;
    switch (q) {
        case 0:
            return kSinQ2048[r];
        case 1:
            return kSinQ2048[512U - r];
        case 2:
            return -kSinQ2048[r];
        default:
            return -kSinQ2048[512U - r];
    }
}

static inline int16_t cos_2048(uint32_t idx) {
    return sin_2048(idx + 512U);
}

static inline uint32_t bit_reverse_11(uint32_t x) {
#if defined(__arm__) && !defined(__aarch64__) && defined(__ARM_ARCH) && (__ARM_ARCH >= 7)
    uint32_t res;
    __asm__("rbit %0, %1" : "=r"(res) : "r"(x));
    return res >> 21;
#else
    uint32_t r = 0;
    for (int i = 0; i < 11; i++) {
        r = (r << 1) | (x & 1U);
        x >>= 1;
    }
    return r;
#endif
}

static inline int32_t iabs32(int32_t v) {
    return (v < 0) ? -v : v;
}

// Generate 1023-chip GPS L1 C/A code packed as 1 bit per chip (128 bytes: bit=0 => +1, bit=1 => -1).
static inline void generate_ca_bits(uint8_t prn, uint8_t bits[128]) {
    if (prn < 1 || prn > NUM_PRNS) prn = 1;
    const uint8_t s1 = kG2Taps[prn - 1][0] - 1;
    const uint8_t s2 = kG2Taps[prn - 1][1] - 1;

    std::memset(bits, 0, 128);
    uint16_t g1 = 0x3FF;
    uint16_t g2 = 0x3FF;

    for (size_t i = 0; i < 1023; i++) {
        const uint8_t g1_out = (g1 >> 9) & 1U;
        const uint8_t g2_out = ((g2 >> s1) ^ (g2 >> s2)) & 1U;
        if (g1_out ^ g2_out) {
            bits[i >> 3] |= static_cast<uint8_t>(1U << (i & 7U));
        }

        const uint16_t fb1 = ((g1 >> 2) ^ (g1 >> 9)) & 1U;
        const uint16_t fb2 = ((g2 >> 1) ^ (g2 >> 2) ^ (g2 >> 5) ^ (g2 >> 7) ^ (g2 >> 8) ^ (g2 >> 9)) & 1U;
        g1 = static_cast<uint16_t>(((g1 << 1) | fb1) & 0x3FFU);
        g2 = static_cast<uint16_t>(((g2 << 1) | fb2) & 0x3FFU);
    }
}

static inline int8_t get_ca_chip(const uint8_t bits[128], size_t chip_idx) {
    return ((bits[chip_idx >> 3] >> (chip_idx & 7U)) & 1U) ? -1 : 1;
}

// Generate 1023-chip GPS L1 C/A code for PRN 1..32 as +1 (chip 0) or -1 (chip 1).
static inline void generate_ca_chips(uint8_t prn, int8_t chips[1023]) {
    uint8_t bits[128];
    generate_ca_bits(prn, bits);
    for (size_t i = 0; i < 1023; i++) {
        chips[i] = get_ca_chip(bits, i);
    }
}

// Fill 2048-sample resampled C/A code into fft_work (amplitude ±4096, imag = 0).
// Uses only 128 bytes of stack (1-bit packed chips).
static inline void generate_ca_2048(uint8_t prn, c16_t out[SAMPLES_PER_MS]) {
    uint8_t bits[128];
    generate_ca_bits(prn, bits);
    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        const size_t chip_idx = (n * 1023U) >> 11;
        out[n].re = static_cast<int16_t>(get_ca_chip(bits, chip_idx) * 4096);
        out[n].im = 0;
    }
}

// In-place 2048-point radix-2 DIT FFT (inverse=false) or IFFT (inverse=true)
// with per-stage block-floating-point scaling.
// Precondition: |data[i].re|, |data[i].im| <= 16383.
// Returns total number of divide-by-2 shifts applied across the 11 stages (0..11).
static inline uint8_t fft_2048_q15(c16_t data[SAMPLES_PER_MS], bool inverse) {
    // Bit-reversal permutation + initial peak check
    int32_t max_abs = 0;
    for (uint32_t i = 0; i < SAMPLES_PER_MS; i++) {
        const uint32_t j = bit_reverse_11(i);
        if (i < j) {
            const c16_t tmp = data[i];
            data[i] = data[j];
            data[j] = tmp;
        }
        const int32_t ar = iabs32(data[i].re);
        const int32_t ai = iabs32(data[i].im);
        if (ar > max_abs) max_abs = ar;
        if (ai > max_abs) max_abs = ai;
    }

    uint8_t total_shifts = 0;

    for (uint32_t stage = 0; stage < 11; stage++) {
        const uint32_t half = 1U << stage;
        const uint32_t step = half << 1;
        const uint32_t tw_step = SAMPLES_PER_MS >> (stage + 1);
        const bool do_shift = (max_abs >= 8192);
        if (do_shift) total_shifts++;

        int32_t next_max = 0;

        if (!do_shift) {
            for (uint32_t m = 0; m < half; m++) {
                const uint32_t tw_idx = m * tw_step;
                const int32_t w_re = cos_2048(tw_idx);
                const int32_t w_im = inverse ? sin_2048(tw_idx) : -sin_2048(tw_idx);

                for (uint32_t i = m; i < SAMPLES_PER_MS; i += step) {
                    const uint32_t j = i + half;
                    const int32_t xr = data[j].re;
                    const int32_t xi = data[j].im;
                    const int32_t tr = (w_re * xr - w_im * xi + 16384) >> 15;
                    const int32_t ti = (w_re * xi + w_im * xr + 16384) >> 15;

                    const int32_t ur = data[i].re;
                    const int32_t ui = data[i].im;
                    const int32_t ar = ur + tr;
                    const int32_t ai = ui + ti;
                    const int32_t br = ur - tr;
                    const int32_t bi = ui - ti;

                    data[i].re = static_cast<int16_t>(ar);
                    data[i].im = static_cast<int16_t>(ai);
                    data[j].re = static_cast<int16_t>(br);
                    data[j].im = static_cast<int16_t>(bi);

                    const int32_t m1 = iabs32(ar) | iabs32(ai) | iabs32(br) | iabs32(bi);
                    if (m1 > next_max) next_max = m1;
                }
            }
        } else {
            for (uint32_t m = 0; m < half; m++) {
                const uint32_t tw_idx = m * tw_step;
                const int32_t w_re = cos_2048(tw_idx);
                const int32_t w_im = inverse ? sin_2048(tw_idx) : -sin_2048(tw_idx);

                for (uint32_t i = m; i < SAMPLES_PER_MS; i += step) {
                    const uint32_t j = i + half;
                    const int32_t xr = data[j].re;
                    const int32_t xi = data[j].im;
                    const int32_t tr = (w_re * xr - w_im * xi + 32768) >> 16;
                    const int32_t ti = (w_re * xi + w_im * xr + 32768) >> 16;

                    const int32_t ur = data[i].re >> 1;
                    const int32_t ui = data[i].im >> 1;
                    const int32_t ar = ur + tr;
                    const int32_t ai = ui + ti;
                    const int32_t br = ur - tr;
                    const int32_t bi = ui - ti;

                    data[i].re = static_cast<int16_t>(ar);
                    data[i].im = static_cast<int16_t>(ai);
                    data[j].re = static_cast<int16_t>(br);
                    data[j].im = static_cast<int16_t>(bi);

                    const int32_t m1 = iabs32(ar) | iabs32(ai) | iabs32(br) | iabs32(bi);
                    if (m1 > next_max) next_max = m1;
                }
            }
        }

        max_abs = next_max;
    }

    return total_shifts;
}

// Quantize a 2048-point complex16 FFT output into compact complex8 [-127..127].
// If conjugate == true, stores conj(src) (for PRN code FFT).
static inline void compress_fft_c8(const c16_t src[SAMPLES_PER_MS], c8_t dst[SAMPLES_PER_MS], bool conjugate) {
    int32_t max_abs = 1;
    for (size_t i = 0; i < SAMPLES_PER_MS; i++) {
        const int32_t ar = iabs32(src[i].re);
        const int32_t ai = iabs32(src[i].im);
        if (ar > max_abs) max_abs = ar;
        if (ai > max_abs) max_abs = ai;
    }
    const int32_t scale_q15 = (127 * 32768) / max_abs;
    for (size_t i = 0; i < SAMPLES_PER_MS; i++) {
        int32_t r = (static_cast<int32_t>(src[i].re) * scale_q15 + 16384) >> 15;
        int32_t im = (static_cast<int32_t>(src[i].im) * scale_q15 + 16384) >> 15;
        if (conjugate) im = -im;
        if (r > 127) r = 127;
        if (r < -127) r = -127;
        if (im > 127) im = 127;
        if (im < -127) im = -127;
        dst[i].re = static_cast<int8_t>(r);
        dst[i].im = static_cast<int8_t>(im);
    }
}

// Accumulate calibration stats on 1 ms of raw int8 I/Q samples (2048 complex samples).
struct CaptureCal {
    int32_t mean_i{0};
    int32_t mean_q{0};
    int32_t thresh_i{16};
    int32_t thresh_q{16};
};

struct RawStatsAccum {
    int64_t sum_i{0};
    int64_t sum_q{0};
    uint64_t sum_sq_i{0};
    uint64_t sum_sq_q{0};
    uint32_t total_samples{0};
    uint32_t clip_count{0};
    uint32_t q2_counts[4]{0, 0, 0, 0};
    uint8_t peak_abs_i{0};
    uint8_t peak_abs_q{0};

    void reset() {
        sum_i = 0;
        sum_q = 0;
        sum_sq_i = 0;
        sum_sq_q = 0;
        total_samples = 0;
        clip_count = 0;
        q2_counts[0] = q2_counts[1] = q2_counts[2] = q2_counts[3] = 0;
        peak_abs_i = 0;
        peak_abs_q = 0;
    }

    void measure_block(const int8_t* iq_interleaved, size_t count) {
        for (size_t n = 0; n < count; n++) {
            const int32_t re = iq_interleaved[2 * n];
            const int32_t im = iq_interleaved[2 * n + 1];
            sum_i += re;
            sum_q += im;
            sum_sq_i += static_cast<uint32_t>(re * re);
            sum_sq_q += static_cast<uint32_t>(im * im);
            const uint8_t ar = static_cast<uint8_t>(re < 0 ? -re : re);
            const uint8_t ai = static_cast<uint8_t>(im < 0 ? -im : im);
            if (ar > peak_abs_i) peak_abs_i = ar;
            if (ai > peak_abs_q) peak_abs_q = ai;
            if (ar >= 126 || ai >= 126) clip_count++;
        }
        total_samples += static_cast<uint32_t>(count);
    }

    CaptureCal derive_cal() const {
        CaptureCal cal{};
        if (total_samples == 0) return cal;
        const float n = static_cast<float>(total_samples);
        const float mi = static_cast<float>(sum_i) / n;
        const float mq = static_cast<float>(sum_q) / n;
        float vi = static_cast<float>(sum_sq_i) / n - mi * mi;
        float vq = static_cast<float>(sum_sq_q) / n - mq * mq;
        if (vi < 1.0f) vi = 1.0f;
        if (vq < 1.0f) vq = 1.0f;
        cal.mean_i = static_cast<int32_t>(mi >= 0.0f ? (mi + 0.5f) : (mi - 0.5f));
        cal.mean_q = static_cast<int32_t>(mq >= 0.0f ? (mq + 0.5f) : (mq - 0.5f));
        cal.thresh_i = static_cast<int32_t>(sqrtf(vi) + 0.5f);
        cal.thresh_q = static_cast<int32_t>(sqrtf(vq) + 0.5f);
        if (cal.thresh_i < 2) cal.thresh_i = 2;
        if (cal.thresh_q < 2) cal.thresh_q = 2;
        return cal;
    }

    // Quantize 2048 int8 complex samples to 2-bit I/Q packed into 1024 bytes,
    // while accumulating exact statistics.
    void pack_ms_2bit(const int8_t* iq_interleaved, const CaptureCal& cal, uint8_t dst[SAMPLES_PER_MS / 2]) {
        for (size_t n = 0; n < SAMPLES_PER_MS; n += 2) {
            uint8_t packed = 0;
            for (size_t k = 0; k < 2; k++) {
                const int32_t re = iq_interleaved[2 * (n + k)];
                const int32_t im = iq_interleaved[2 * (n + k) + 1];

                sum_i += re;
                sum_q += im;
                sum_sq_i += static_cast<uint32_t>(re * re);
                sum_sq_q += static_cast<uint32_t>(im * im);

                const uint8_t ar = static_cast<uint8_t>(re < 0 ? -re : re);
                const uint8_t ai = static_cast<uint8_t>(im < 0 ? -im : im);
                if (ar > peak_abs_i) peak_abs_i = ar;
                if (ai > peak_abs_q) peak_abs_q = ai;
                if (ar >= 126 || ai >= 126) clip_count++;

                const int32_t ri = re - cal.mean_i;
                const int32_t rq = im - cal.mean_q;

                uint8_t qi;
                if (ri < -cal.thresh_i)
                    qi = 0;
                else if (ri < 0)
                    qi = 1;
                else if (ri < cal.thresh_i)
                    qi = 2;
                else
                    qi = 3;

                uint8_t qq;
                if (rq < -cal.thresh_q)
                    qq = 0;
                else if (rq < 0)
                    qq = 1;
                else if (rq < cal.thresh_q)
                    qq = 2;
                else
                    qq = 3;

                q2_counts[qi]++;
                q2_counts[qq]++;

                const uint8_t nib = static_cast<uint8_t>((qi << 2) | qq);
                if (k == 0)
                    packed = nib;
                else
                    packed |= static_cast<uint8_t>(nib << 4);
            }
            dst[n >> 1] = packed;
        }
        total_samples += SAMPLES_PER_MS;
    }

    SignalStats to_signal_stats(uint16_t dma_drops, uint16_t dma_buffers, uint8_t snapshot_ms, uint8_t is_selftest) const {
        SignalStats st{};
        st.dma_drops = dma_drops;
        st.dma_buffers = dma_buffers;
        st.snapshot_ms = snapshot_ms;
        st.is_selftest = is_selftest;
        st.peak_abs_i = peak_abs_i;
        st.peak_abs_q = peak_abs_q;

        if (total_samples == 0) return st;
        const float n = static_cast<float>(total_samples);
        const float mi = static_cast<float>(sum_i) / n;
        const float mq = static_cast<float>(sum_q) / n;
        float vi = static_cast<float>(sum_sq_i) / n - mi * mi;
        float vq = static_cast<float>(sum_sq_q) / n - mq * mq;
        if (vi < 0.0f) vi = 0.0f;
        if (vq < 0.0f) vq = 0.0f;

        st.mean_i_x10 = static_cast<int16_t>(mi * 10.0f);
        st.mean_q_x10 = static_cast<int16_t>(mq * 10.0f);
        st.rms_i_x10 = static_cast<uint16_t>(sqrtf(vi) * 10.0f + 0.5f);
        st.rms_q_x10 = static_cast<uint16_t>(sqrtf(vq) * 10.0f + 0.5f);
        st.clip_pm = static_cast<uint16_t>((static_cast<uint64_t>(clip_count) * 1000ULL) / total_samples);

        const uint32_t q_tot = q2_counts[0] + q2_counts[1] + q2_counts[2] + q2_counts[3];
        if (q_tot > 0) {
            for (int b = 0; b < 4; b++) {
                st.q2_pct[b] = static_cast<uint8_t>((static_cast<uint64_t>(q2_counts[b]) * 100ULL + q_tot / 2) / q_tot);
            }
        }
        return st;
    }
};

// Coherent fold of K consecutive 1ms packed 2-bit blocks starting at snapshot[start_ms],
// applying sub-bin Doppler rotation f_sub = sub_idx * (1000 / (2 * K)) Hz.
// K must be 1, 2, or 4. sub_idx in [0, 2*K - 1].
static inline void fold_coherent_block(
    const uint8_t snapshot[][SAMPLES_PER_MS / 2],
    size_t start_ms,
    uint8_t coh_ms,
    uint8_t sub_idx,
    c16_t out[SAMPLES_PER_MS]) {
    // Precompute inter-ms rotation exp(-j * pi * m * sub_idx / coh_ms)
    int32_t rot_re[4];
    int32_t rot_im[4];
    for (uint8_t m = 0; m < coh_ms; m++) {
        const uint32_t ang = (static_cast<uint32_t>(m) * sub_idx * (1024U / coh_ms)) & 2047U;
        rot_re[m] = cos_2048(ang);
        rot_im[m] = -sin_2048(ang);
    }

    const uint8_t shift_k = (coh_ms == 1) ? 1 : (coh_ms == 2 ? 2 : 3);
    // Scale factor to keep |out| comfortably around ~2000..4000 for good q15 precision
    const int32_t amp_scale = (coh_ms == 1) ? 1024 : (coh_ms == 2 ? 512 : 256);

    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        const size_t byte_idx = n >> 1;
        const uint8_t nib_shift = (n & 1U) ? 4 : 0;

        int32_t sum_r = 0;
        int32_t sum_i = 0;

        for (uint8_t m = 0; m < coh_ms; m++) {
            const uint8_t nib = (snapshot[start_ms + m][byte_idx] >> nib_shift) & 0x0FU;
            const int32_t si = kQ2Levels[(nib >> 2) & 3U];
            const int32_t sq = kQ2Levels[nib & 3U];
            // (si + j*sq) * (rot_re + j*rot_im)
            sum_r += (si * rot_re[m] - sq * rot_im[m]);
            sum_i += (si * rot_im[m] + sq * rot_re[m]);
        }

        // Convert from Q15 to scaled integer (amp_scale / 32768)
        // Since amp_scale is 1024/512/256, >> (15 - log2(amp_scale)) = >> 5, 6, 7
        const int32_t r_shift = (coh_ms == 1) ? 5 : (coh_ms == 2 ? 6 : 7);
        int32_t fr = sum_r >> r_shift;
        int32_t fi = sum_i >> r_shift;

        // Intra-ms sub-bin rotation: exp(-j * 2*pi * (sub_idx * n / (2*K)) / 2048)
        if (sub_idx != 0) {
            const uint32_t ang = (static_cast<uint32_t>(sub_idx) * static_cast<uint32_t>(n)) >> shift_k;
            const int32_t wr = cos_2048(ang);
            const int32_t wi = -sin_2048(ang);
            const int32_t rr = (fr * wr - fi * wi + 16384) >> 15;
            const int32_t ri = (fr * wi + fi * wr + 16384) >> 15;
            fr = rr;
            fi = ri;
        }
        (void)amp_scale;

        out[n].re = static_cast<int16_t>(fr);
        out[n].im = static_cast<int16_t>(fi);
    }
}

// Compute correlation power accum[2048] across num_blocks cached FFTs at integer-kHz Doppler shift k_hz.
static inline void correlate_doppler_bin(
    const c8_t cached_fft[][SAMPLES_PER_MS],
    uint8_t num_blocks,
    const c8_t fft_code[SAMPLES_PER_MS],
    int32_t k_hz,
    c16_t fft_work[SAMPLES_PER_MS],
    uint32_t accum[SAMPLES_PER_MS]) {
    std::memset(accum, 0, SAMPLES_PER_MS * sizeof(uint32_t));
    const uint32_t k_off = static_cast<uint32_t>(k_hz + 2048) & 2047U;

    for (uint8_t b = 0; b < num_blocks; b++) {
        const c8_t* sig = cached_fft[b];
        for (uint32_t m = 0; m < SAMPLES_PER_MS; m++) {
            const c8_t x = sig[(m + k_off) & 2047U];
            const c8_t c = fft_code[m];
            // (x.re + j*x.im) * (c.re + j*c.im) >> 1  (c already holds conj(FFT(code)))
            fft_work[m].re = static_cast<int16_t>((static_cast<int32_t>(x.re) * c.re - static_cast<int32_t>(x.im) * c.im) >> 1);
            fft_work[m].im = static_cast<int16_t>((static_cast<int32_t>(x.re) * c.im + static_cast<int32_t>(x.im) * c.re) >> 1);
        }

        const uint8_t shifts = fft_2048_q15(fft_work, true);
        int32_t p_shift = 22 - 2 * static_cast<int32_t>(shifts);
        if (p_shift < 0) p_shift = 0;

        for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
            const int32_t re = fft_work[n].re;
            const int32_t im = fft_work[n].im;
            const uint32_t p = static_cast<uint32_t>(re * re + im * im);
            accum[n] += (p >> p_shift);
        }
    }
}

// Compact IEEE-754 float log10 approximation (accurate to ~0.005 / 0.05 dB, avoids libm log10f bloat).
static inline float fast_log10f(float x) {
    if (x <= 1e-6f) return -6.0f;
    uint32_t bits = 0;
    std::memcpy(&bits, &x, sizeof(bits));
    const int32_t exp = static_cast<int32_t>((bits >> 23) & 0xFFU) - 127;
    bits = (bits & 0x007FFFFFU) | 0x3F800000U;  // mantissa m in [1.0, 2.0)
    float m = 1.0f;
    std::memcpy(&m, &bits, sizeof(m));
    // Minimax quadratic for log2(m) on [1, 2): (-0.344845*m + 2.024658)*m - 1.674873
    const float log2_m = (-0.344845f * m + 2.024658f) * m - 1.674873f;
    return (static_cast<float>(exp) + log2_m) * 0.3010299957f;
}

// Fast scale-invariant peak-to-mean score (Q8 fixed-point: 256 = 1.0x)
// Used to filter out non-winning Doppler bins without computing sqrtf/log10f.
static inline uint32_t quick_peak_over_mean_q8(const uint32_t accum[SAMPLES_PER_MS]) {
    uint32_t peak = 0;
    uint64_t sum = 0;
    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        const uint32_t v = accum[n];
        if (v > peak) peak = v;
        sum += v;
    }
    const uint32_t mean = static_cast<uint32_t>(sum >> 11);  // / 2048
    return (mean > 0) ? ((peak << 8) / mean) : 0;
}

// Analyze accum[2048] for peak, 2nd peak (outside ±4 samples = ±2 chips), noise mean/std, Z-score, ratio, C/N0.
static inline void analyze_correlation(
    const uint32_t accum[SAMPLES_PER_MS],
    uint8_t prn,
    int16_t doppler_hz,
    uint8_t coh_ms,
    uint8_t num_blocks,
    PRNResult& out) {
    uint32_t peak = 0;
    uint16_t peak_idx = 0;
    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        if (accum[n] > peak) {
            peak = accum[n];
            peak_idx = static_cast<uint16_t>(n);
        }
    }

    uint64_t sum = 0;
    uint32_t second_peak = 0;
    uint32_t noise_cnt = 0;

    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        int32_t dist = static_cast<int32_t>(n) - static_cast<int32_t>(peak_idx);
        if (dist > 1024) dist -= 2048;
        if (dist < -1024) dist += 2048;
        if (iabs32(dist) <= 4) continue;

        const uint32_t v = accum[n];
        sum += v;
        if (v > second_peak) second_peak = v;
        noise_cnt++;
    }

    const float mean = (noise_cnt > 0) ? (static_cast<float>(sum) / static_cast<float>(noise_cnt)) : 1.0f;
    float var_sum = 0.0f;
    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        int32_t dist = static_cast<int32_t>(n) - static_cast<int32_t>(peak_idx);
        if (dist > 1024) dist -= 2048;
        if (dist < -1024) dist += 2048;
        if (iabs32(dist) <= 4) continue;
        const float d = static_cast<float>(accum[n]) - mean;
        var_sum += d * d;
    }
    const float std_dev = (noise_cnt > 1) ? sqrtf(var_sum / static_cast<float>(noise_cnt)) : 1.0f;

    const float ratio = (second_peak > 0) ? (static_cast<float>(peak) / static_cast<float>(second_peak)) : 0.0f;
    const float z_score = (std_dev > 1e-3f) ? ((static_cast<float>(peak) - mean) / std_dev) : 0.0f;

    // C/N0 estimation: SNR_coh = (peak - mean) / mean, C/N0 = 10 * log10(SNR_coh / T_coh)
    float snr_coh = (mean > 1e-3f) ? ((static_cast<float>(peak) - mean) / mean) : 0.01f;
    if (snr_coh < 0.01f) snr_coh = 0.01f;
    const float t_coh_s = static_cast<float>(coh_ms) * 0.001f;
    const float cn0 = 10.0f * fast_log10f(snr_coh / t_coh_s);

    DetStatus status = DetStatus::Noise;
    if (num_blocks <= 1) {
        if (ratio >= 2.10f && z_score >= 16.0f)
            status = DetStatus::Lock;
        else if (ratio >= 1.75f && z_score >= 13.5f)
            status = DetStatus::Weak;
    } else {
        if (ratio >= 2.00f && z_score >= 12.5f)
            status = DetStatus::Lock;
        else if (ratio >= 1.70f && z_score >= 10.8f)
            status = DetStatus::Weak;
    }

    out.prn = prn;
    out.status = static_cast<uint8_t>(status);
    out.doppler_hz = doppler_hz;
    out.code_phase = peak_idx;
    out.peak_power = peak;
    out.second_peak = second_peak;
    out.noise_mean = static_cast<uint32_t>(mean + 0.5f);
    out.noise_std_x10 = static_cast<uint32_t>(std_dev * 10.0f + 0.5f);
    out.ratio_x100 = static_cast<uint16_t>(ratio * 100.0f + 0.5f);
    out.z_score_x10 = static_cast<uint16_t>(z_score > 0.0f ? (z_score * 10.0f + 0.5f) : 0.0f);
    out.cn0_dbhz_x10 = static_cast<int16_t>(cn0 * 10.0f);
}

// Extract 64-bin correlation profile centered on result.code_phase (±32 samples = ±16 chips).
static inline void extract_corr_profile(
    const uint32_t accum[SAMPLES_PER_MS],
    const PRNResult& res,
    uint32_t scan_id,
    CorrProfile& prof) {
    prof.result = res;
    prof.scan_id = scan_id;
    const uint32_t denom = (res.peak_power > 0) ? res.peak_power : 1;
    const uint32_t start = (static_cast<uint32_t>(res.code_phase) + SAMPLES_PER_MS - (PROFILE_BINS / 2)) & 2047U;
    for (size_t i = 0; i < PROFILE_BINS; i++) {
        const uint32_t v = accum[(start + i) & 2047U];
        uint32_t scaled = (v * 255U) / denom;
        if (scaled > 255U) scaled = 255U;
        prof.bins[i] = static_cast<uint8_t>(scaled);
    }
    uint32_t m_scaled = (res.noise_mean * 255U) / denom;
    uint32_t s_scaled = (res.second_peak * 255U) / denom;
    prof.mean_level = static_cast<uint8_t>(m_scaled > 255U ? 255U : m_scaled);
    prof.second_level = static_cast<uint8_t>(s_scaled > 255U ? 255U : s_scaled);
}

// Synthetic self-test signal generator (1 ms of 2048 complex int8 samples).
// Injects two realistic sub-noise-floor GPS L1 C/A satellites into Gaussian-like noise:
//   - PRN 1:  Doppler = +2250 Hz, code_phase = 691 samples (345.2 chips), C/N0 ~ 43 dB-Hz
//   - PRN 17: Doppler = -3500 Hz, code_phase = 1581 samples (789.7 chips), C/N0 ~ 38 dB-Hz (nav bit flip at ms 5)
static inline void generate_selftest_ms(
    uint32_t ms_index,
    uint32_t& prng_state,
    int8_t out_iq[SAMPLES_PER_MS * 2]) {
    uint8_t bits1[128];
    uint8_t bits17[128];
    generate_ca_bits(1, bits1);
    generate_ca_bits(17, bits17);

    // Phase accumulator: 2*pi*f_d/2048000 mapped to 32-bit phase (2^32 = 1 cycle)
    // phase_inc = round(f_d / 2048000 * 2^32) = f_d * 2097.152
    static constexpr uint32_t kInc1 = 4718592U;              // +2250 Hz exact
    static constexpr uint32_t kInc17 = static_cast<uint32_t>(-7340032);  // -3500 Hz exact
    uint32_t ph1 = static_cast<uint32_t>(ms_index * SAMPLES_PER_MS) * kInc1;
    uint32_t ph17 = static_cast<uint32_t>(ms_index * SAMPLES_PER_MS) * kInc17;

    const int32_t nav17 = (ms_index < 5) ? 1 : -1;

    auto next_u32 = [&prng_state]() -> uint32_t {
        uint32_t x = prng_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        prng_state = x;
        return x;
    };

    for (size_t n = 0; n < SAMPLES_PER_MS; n++) {
        // Approximate Gaussian noise (sigma ~ 24) via sum of 4 uniform bytes
        const uint32_t r1 = next_u32();
        const uint32_t r2 = next_u32();
        int32_t ni = static_cast<int32_t>((r1 & 0xFFU) + ((r1 >> 8) & 0xFFU) + ((r1 >> 16) & 0xFFU) + ((r1 >> 24) & 0xFFU)) - 510;
        int32_t nq = static_cast<int32_t>((r2 & 0xFFU) + ((r2 >> 8) & 0xFFU) + ((r2 >> 16) & 0xFFU) + ((r2 >> 24) & 0xFFU)) - 510;
        // Uniform[0..255] std = 73.9 -> sum of 4 std = 147.8 -> /6 => std ~ 24.6
        ni = (ni / 6) + 1;   // slight realistic DC offset +1
        nq = (nq / 6) - 1;   // slight realistic DC offset -1

        // PRN 1 delayed by 691 samples: x[n] has c[(n - 691) mod 2048]
        const size_t idx1 = (n + SAMPLES_PER_MS - 691U) & 2047U;
        const int32_t c1 = get_ca_chip(bits1, (idx1 * 1023U) >> 11);
        const uint32_t a1 = ph1 >> 21;  // top 11 bits -> 0..2047
        // Amplitude ~3.8 counts (C/N0 ~ 43 dB-Hz in sigma=24.6 noise)
        const int32_t s1_i = (c1 * cos_2048(a1) * 38) >> 18;
        const int32_t s1_q = (c1 * sin_2048(a1) * 38) >> 18;

        // PRN 17 delayed by 1581 samples, amplitude ~2.3 counts (C/N0 ~ 38.5 dB-Hz)
        const size_t idx17 = (n + SAMPLES_PER_MS - 1581U) & 2047U;
        const int32_t c17 = get_ca_chip(bits17, (idx17 * 1023U) >> 11) * nav17;
        const uint32_t a17 = ph17 >> 21;
        const int32_t s17_i = (c17 * cos_2048(a17) * 23) >> 18;
        const int32_t s17_q = (c17 * sin_2048(a17) * 23) >> 18;

        int32_t vi = ni + s1_i + s17_i;
        int32_t vq = nq + s1_q + s17_q;
        if (vi > 127) vi = 127;
        if (vi < -128) vi = -128;
        if (vq > 127) vq = 127;
        if (vq < -128) vq = -128;

        out_iq[2 * n] = static_cast<int8_t>(vi);
        out_iq[2 * n + 1] = static_cast<int8_t>(vq);

        ph1 += kInc1;
        ph17 += kInc17;
    }
}

} /* namespace gps_scan */

#endif /* __GPS_ACQ_DSP_H__ */
