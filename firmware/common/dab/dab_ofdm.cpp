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

/* The demapper's packed path is what the device runs; DAB_DEMAP_SIMD_PORTABLE
 * builds it from dab_simd.hpp's plain-C twins so a host can check it. */
#if defined(__ARM_FEATURE_SIMD32) || defined(DAB_DEMAP_SIMD_PORTABLE)
#define DAB_DEMAP_PACKED 1
#endif
#if defined(DAB_DEMAP_SIMD_PORTABLE)
#define DAB_SIMD_PORTABLE 1
#endif

#include "dab_ofdm.hpp"
#include "dab_freq_interleave.hpp"
#include "dab_simd.hpp"
#include "sine_table_int8.hpp"

#include <cmath>

namespace dab {

namespace {

constexpr float kTwoPi = 6.283185307179586f;

/* p radians across one useful part is p / (2 pi * 1 ms) Hz. */
constexpr float kUsefulSymbolSeconds = static_cast<float>(kFftSize) / static_cast<float>(kSampleRate);

inline uint32_t sample_power(const Iq16& s) {
    const int32_t i = s.i;
    const int32_t q = s.q;
    return static_cast<uint32_t>(i * i + q * q);
}

/* max + min/2 (alpha-max-plus-beta-min, within ~4%). always_inline: -Os
 * otherwise emits a call per carrier. */
inline __attribute__((always_inline)) int32_t magnitude(int32_t i, int32_t q) {
    /* `m` is (ai - aq) when ai < aq, else 0: ai - m is the larger, aq + m the
     * smaller. */
    const int32_t si = i >> 31;
    const int32_t sq = q >> 31;
    const int32_t ai = (i ^ si) - si;
    const int32_t aq = (q ^ sq) - sq;
    const int32_t d = ai - aq;
    const int32_t m = d & (d >> 31);
    const int32_t hi = ai - m;
    const int32_t lo = aq + m;
    return hi + (lo >> 1);
}

#if defined(DAB_DEMAP_PACKED)
/* (q:i) halfword pair for SIMD32. Iq16 is 2-byte aligned: memcpy compiles to
 * one unaligned LDR on ARMv7-M. */
inline uint32_t iq_word(const Iq16* p) {
    uint32_t w;
    __builtin_memcpy(&w, p, sizeof(w));
    return w;
}
inline int32_t lane_lo(uint32_t w) { return static_cast<int16_t>(w & 0xFFFFu); }
inline int32_t lane_hi(uint32_t w) { return static_cast<int16_t>(w >> 16); }
#endif

inline int32_t sat8(int32_t v) {
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_FEATURE_SAT)
    int32_t r;
    __asm("ssat %0, #8, %1" : "=r"(r) : "r"(v));
    return r;
#else
    if (v > 127)
        return 127;
    if (v < -128)
        return -128;
    return v;
#endif
}

/* clamp(-v, -127, +127), since +128 is not an int8; exact for every int32
 * (v <= -128 -> -128 -> +128 -> +127). */
inline int32_t clamp127_neg(int32_t v) {
    return sat8(-sat8(v));
}

}  // namespace

bool find_null_symbol(const Iq16* samples,
                      size_t count,
                      size_t& null_offset,
                      uint32_t& confidence_ratio) {
    if (samples == nullptr || count < kNullSamples)
        return false;

    const size_t window = kNullSamples;
    const size_t positions = count - window + 1;

    uint64_t running = 0;
    for (size_t i = 0; i < window; ++i)
        running += sample_power(samples[i]);

    uint64_t best = running;
    size_t best_index = 0;
    uint64_t total = running;
    size_t windows_counted = 1;

    for (size_t start = 1; start < positions; ++start) {
        running -= sample_power(samples[start - 1]);
        running += sample_power(samples[start + window - 1]);

        if (running < best) {
            best = running;
            best_index = start;
        }

        total += running;
        ++windows_counted;
    }

    null_offset = best_index;

    const uint64_t mean = total / windows_counted;
    if (best == 0) {
        confidence_ratio = mean > 0 ? 1000 : 0;
    } else {
        confidence_ratio = static_cast<uint32_t>((mean * 100) / best);
    }

    return confidence_ratio >= 150;
}

float estimate_frequency_offset(const Iq16* symbol, uint16_t* guard_quality) {
    if (guard_quality != nullptr)
        *guard_quality = 0;
    if (symbol == nullptr)
        return 0.0f;

    float real_sum = 0.0f;
    float imag_sum = 0.0f;

    float energy_sum = 0.0f;

    for (size_t n = 0; n < kGuardSamples; ++n) {
        const Iq16& a = symbol[n];
        const Iq16& b = symbol[n + kFftSize];

        /* b * conj(a) */
        real_sum += static_cast<float>(b.i) * a.i + static_cast<float>(b.q) * a.q;
        imag_sum += static_cast<float>(b.q) * a.i - static_cast<float>(b.i) * a.q;

        /* Identical blocks correlate to their energy: a guard in place gives 1. */
        energy_sum += 0.5f * (static_cast<float>(a.i) * a.i + static_cast<float>(a.q) * a.q +
                              static_cast<float>(b.i) * b.i + static_cast<float>(b.q) * b.q);
    }

    if (guard_quality != nullptr && energy_sum > 0.0f) {
        const float ratio = __builtin_sqrtf(real_sum * real_sum + imag_sum * imag_sum) / energy_sum;
        const uint32_t scaled = static_cast<uint32_t>(ratio * 1000.0f);
        *guard_quality = static_cast<uint16_t>(scaled > 0xFFFF ? 0xFFFF : scaled);
    }

    if (real_sum == 0.0f && imag_sum == 0.0f)
        return 0.0f;

    const float phase = std::atan2(imag_sum, real_sum);
    return phase / (kTwoPi * kUsefulSymbolSeconds);
}

void accumulate_frequency_offset(const Iq16* symbol, int32_t* real_acc, int32_t* imag_acc) {
    if (symbol == nullptr || real_acc == nullptr || imag_acc == nullptr)
        return;

    int32_t re = 0;
    int32_t im = 0;
    for (size_t n = 0; n < kGuardSamples; ++n) {
        const Iq16& a = symbol[n];
        const Iq16& b = symbol[n + kFftSize];
        /* b * conj(a) */
        re += static_cast<int32_t>(b.i) * a.i + static_cast<int32_t>(b.q) * a.q;
        im += static_cast<int32_t>(b.q) * a.i - static_cast<int32_t>(b.i) * a.q;
    }

    /* Full-scale int8 gives at most 504 * 2 * 127 * 127 = 16.3M per symbol;
     * >> 6 leaves 254k, so 76 symbols cannot overflow. */
    *real_acc += re >> 6;
    *imag_acc += im >> 6;
}

float frequency_offset_from_sum(int32_t real_sum, int32_t imag_sum) {
    if (real_sum == 0 && imag_sum == 0)
        return 0.0f;
    const float phase = std::atan2(static_cast<float>(imag_sum), static_cast<float>(real_sum));
    return phase / (kTwoPi * kUsefulSymbolSeconds);
}

void correct_frequency_offset(Iq16* samples, size_t count, float offset_hz) {
    if (samples == nullptr || offset_hz == 0.0f)
        return;

    const float step = -kTwoPi * offset_hz / static_cast<float>(kSampleRate);
    float phase = 0.0f;

    for (size_t n = 0; n < count; ++n) {
        const float c = std::cos(phase);
        const float s = std::sin(phase);

        const float i = static_cast<float>(samples[n].i);
        const float q = static_cast<float>(samples[n].q);

        samples[n].i = static_cast<int16_t>(i * c - q * s);
        samples[n].q = static_cast<int16_t>(i * s + q * c);

        phase += step;
        if (phase > kTwoPi)
            phase -= kTwoPi;
        else if (phase < -kTwoPi)
            phase += kTwoPi;
    }
}

void FrequencyCorrector::reset() {
    phase_ = 0;
    phase_increment_ = 0;
}

void FrequencyCorrector::set_offset(float offset_hz) {
    const float turns_per_sample = -offset_hz / static_cast<float>(kSampleRate);
    phase_increment_ = static_cast<int32_t>(turns_per_sample * 4294967296.0f);
    phase_ = 0;
}

#if defined(__ARM_FEATURE_SIMD32)
namespace {
/* cos low, sin high for SMUSD/SMUADX, one load per sample; packing per sample
 * costs more than the dual multiplies save (DAB-IMPL 6.76). Built into .bss:
 * .rodata shares the nearly full bank 2 with code. */
int32_t g_packed_twiddle[256];
/* The boost the table holds; 0xFF until built. */
uint8_t g_packed_twiddle_boost = 0xFF;

void build_packed_twiddle(uint8_t boost) {
    const int32_t scale = 1 << boost;
    for (int i = 0; i < 256; ++i)
        g_packed_twiddle[i] = static_cast<int32_t>(
            (static_cast<uint32_t>(static_cast<uint16_t>(
                 static_cast<int16_t>(sine_table_i8[i] * scale)))
             << 16) |
            static_cast<uint32_t>(static_cast<uint16_t>(static_cast<int16_t>(
                sine_table_i8[static_cast<uint8_t>(i + 64)] * scale))));
    g_packed_twiddle_boost = boost;
}
}  // namespace
#endif

void FrequencyCorrector::apply(Iq16* samples, size_t count) {
    if (samples == nullptr || (phase_increment_ == 0 && boost_ == 0))
        return;

#if defined(__ARM_FEATURE_SIMD32)
    if (g_packed_twiddle_boost != boost_)
        build_packed_twiddle(boost_);
    /* Sample i low, q high; twiddle cos low, sin high:
     *   smusd  = i*cos - q*sin,  smuadx = i*sin + q*cos
     * 32-bit accumulation as in the scalar form, so bit-identical. */
    for (size_t n = 0; n < count; n++) {
        const uint8_t index = static_cast<uint8_t>(phase_ >> 24);
        const int32_t tw = g_packed_twiddle[index];
        uint32_t sample;
        __builtin_memcpy(&sample, &samples[n], sizeof(sample));

        const int32_t re = dsp_smusd(sample, static_cast<uint32_t>(tw));
        const int32_t im = dsp_smuadx(sample, static_cast<uint32_t>(tw));

        samples[n].i = saturate16(re >> 7);
        samples[n].q = saturate16(im >> 7);

        phase_ += static_cast<uint32_t>(phase_increment_);
    }
#else
    for (size_t n = 0; n < count; n++) {
        const uint8_t index = static_cast<uint8_t>(phase_ >> 24);
        const int32_t sin_v = sine_table_i8[index] * (1 << boost_);
        const int32_t cos_v = sine_table_i8[static_cast<uint8_t>(index + 64)] * (1 << boost_);

        const int32_t i = samples[n].i;
        const int32_t q = samples[n].q;

        /* A rotation can exceed either input component: saturate, not wrap. */
        samples[n].i = saturate16((i * cos_v - q * sin_v) >> 7);
        samples[n].q = saturate16((i * sin_v + q * cos_v) >> 7);

        phase_ += static_cast<uint32_t>(phase_increment_);
    }
#endif
}

void extract_carriers(const Iq16* bins, Iq16* out) {
    if (bins == nullptr || out == nullptr)
        return;

    const int16_t* map = frequency_map();
    for (size_t k = 0; k < kCarriers; ++k)
        out[k] = bins[carrier_to_fft_bin(map[k])];
}

void dqpsk_demap(const Iq16* previous, const Iq16* current, size_t count, int8_t* soft_out) {
    if (previous == nullptr || current == nullptr || soft_out == nullptr || count == 0)
        return;

    /* Divide by the reference magnitude, then the symbol's mean: a global scale
     * lets strong carriers crush the rest to near-erasures (DAB-IMPL 6.9). A
     * typical carrier lands at kSoftScale; dividing by |previous|^2 would
     * equalise every carrier and lose the fading. */
    constexpr int32_t kSoftScale = 72;

    /* Every fourth carrier is enough for the mean. */
    int32_t mag_total = 0;
#if defined(DAB_DEMAP_PACKED)
    for (size_t k = 0; k < count; k += 4) {
        const uint32_t w = iq_word(previous + k);
        mag_total += magnitude(lane_lo(w), lane_hi(w));
    }
#else
    for (size_t k = 0; k < count; k += 4)
        mag_total += magnitude(previous[k].i, previous[k].q);
#endif

    const int32_t mean_mag = mag_total / static_cast<int32_t>((count + 3) / 4);
    if (mean_mag <= 0) {
        for (size_t k = 0; k < 2 * count; ++k)
            soft_out[k] = 0;
        return;
    }

    /* Mean folded into one Q12 multiplier. The quotient below is bounded by
     * 1.5 * 32767, so capping scale_q keeps the product in int32; the cap binds
     * only at a mean magnitude under seven, i.e. no signal. */
    constexpr int32_t kScaleShift = 12;
    constexpr int32_t kScaleMax = 43000;  // 43000 * 49151 < 2^31
    int32_t scale_q = static_cast<int32_t>((kSoftScale * (1 << kScaleShift)) / mean_mag);
    if (scale_q > kScaleMax)
        scale_q = kScaleMax;

    for (size_t k = 0; k < count; ++k) {
#if defined(DAB_DEMAP_PACKED)
        const uint32_t pw = iq_word(previous + k);
        const int32_t pi_ = lane_lo(pw);
        const int32_t pq = lane_hi(pw);
#else
        const int32_t pi_ = previous[k].i;
        const int32_t pq = previous[k].q;
        const int32_t ci = current[k].i;
        const int32_t cq = current[k].q;
#endif

        const int32_t mag = magnitude(pi_, pq);
        if (mag == 0) {
            soft_out[k] = 0;
            soft_out[k + count] = 0;
            continue;
        }

        /* Same int32 as the scalar form. The FFT's scaling holds carriers near
         * 16k, so int32 has room. */
#if defined(DAB_DEMAP_PACKED)
        const uint32_t cw = iq_word(current + k);
        const int32_t real = dsp_smuad(cw, pw);
        const int32_t imag = dsp_smusdx(pw, cw);
#else
        const int32_t real = ci * pi_ + cq * pq;
        const int32_t imag = cq * pi_ - ci * pq;
#endif

        /* Divide first so the scale can be a multiply. Keep the SDIVs: reciprocal
         * plus SMMUL rounds toward -inf, a one-sided soft bias (DAB-IMPL 6.74). */
        const int32_t rs = ((real / mag) * scale_q) >> kScaleShift;
        const int32_t is = ((imag / mag) * scale_q) >> kScaleShift;

        /* clamp127_neg is the negation the header describes. */
        soft_out[k] = static_cast<int8_t>(clamp127_neg(rs));
        soft_out[k + count] = static_cast<int8_t>(clamp127_neg(is));
    }
}

} /* namespace dab */
