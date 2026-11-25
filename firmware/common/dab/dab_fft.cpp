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

/* Radix-2, eleven stages: bit-identical to the transform it replaced, the
 * arithmetic control for dab_fft_r4.cpp (DAB-IMPL 6.19). Stages 0 and 1 keep
 * their twiddle multiply: kFftTwiddles[0] = {32767, 0} is not 1, (x*32767)>>15
 * is x - (x>0), and any cheaper form gives up the bit-identity. */

/* The packed path is what the device runs; DAB_FFT_SIMD_PORTABLE builds it
 * from dab_simd.hpp's plain-C twins so a host can check it. */
#if defined(__ARM_FEATURE_SIMD32) || defined(DAB_FFT_SIMD_PORTABLE)
#define DAB_FFT_PACKED 1
#endif
#if defined(DAB_FFT_SIMD_PORTABLE)
#define DAB_SIMD_PORTABLE 1
#endif

#include "dab_simd.hpp"
#include "dab_fft.hpp"
#include "dab_fft_twiddles.hpp"

namespace dab {

namespace {

constexpr size_t kStages = 11;
static_assert(1u << kStages == kFftSize, "stage count must match the transform size");

#if defined(DAB_FFT_PACKED)
using Twiddle = uint32_t;
inline __attribute__((always_inline)) Twiddle twiddle_at(const FftComplex* w) {
    uint32_t v;
    __builtin_memcpy(&v, w, sizeof(v));
    return v;
}
#else
struct Twiddle {
    int32_t re;
    int32_t im;
};
inline __attribute__((always_inline)) Twiddle twiddle_at(const FftComplex* w) {
    return Twiddle{w->re, w->im};
}
#endif

/* Scale picks halving (SHADD16/SHSUB16) or saturating (QADD16/QSUB16) adds.
 * always_inline: -Os otherwise emits a call. memcpy because FftComplex is 2-byte
 * aligned: one unaligned LDR/STR, where LDRD/LDM would fault. PKHBT's shifter
 * does the imaginary product's `asr #15`. */
template <bool Scale>
inline __attribute__((always_inline)) void butterfly(FftComplex* a, FftComplex* b, Twiddle w) {
#if defined(DAB_FFT_PACKED)
    uint32_t ap, bp;
    __builtin_memcpy(&ap, a, sizeof(ap));
    __builtin_memcpy(&bp, b, sizeof(bp));

    const int32_t tr = dsp_smusd(bp, w) >> 15;
    const uint32_t tp = dsp_pack_lsl1(static_cast<uint32_t>(tr),
                                      static_cast<uint32_t>(dsp_smuadx(bp, w)));

    const uint32_t sp = Scale ? dsp_shadd16(ap, tp) : dsp_qadd16(ap, tp);
    const uint32_t dp = Scale ? dsp_shsub16(ap, tp) : dsp_qsub16(ap, tp);

    __builtin_memcpy(a, &sp, sizeof(sp));
    __builtin_memcpy(b, &dp, sizeof(dp));
#else
    const int32_t br = b->re;
    const int32_t bi = b->im;

    const int32_t tr = (br * w.re - bi * w.im) >> 15;
    const int32_t ti = (br * w.im + bi * w.re) >> 15;

    const int32_t ar = a->re;
    const int32_t ai = a->im;

    /* Saturates as the packed adds do per lane: a halving stage can follow
     * an unscaled one that grew the peak past int16. */
    a->re = saturate16(Scale ? (ar + tr) >> 1 : ar + tr);
    a->im = saturate16(Scale ? (ai + ti) >> 1 : ai + ti);
    b->re = saturate16(Scale ? (ar - tr) >> 1 : ar - tr);
    b->im = saturate16(Scale ? (ai - ti) >> 1 : ai - ti);
#endif
}

/* Twiddle outermost: each twiddle stays in a register across the group loop.
 * `trips` = kFftSize/span is the group count and the twiddle step. noinline for
 * its own register frame (inlined at -Os it spills per butterfly). half and
 * trips are runtime to share one instantiation; pass_twiddle<> checks them. */
template <bool Scale>
__attribute__((noinline)) void pass_twiddle_outer(FftComplex* data, size_t half, size_t trips) {
    const size_t span = half << 1;
    const FftComplex* w = kFftTwiddles;
    /* The twiddle pointer reaching the table's end is the loop's end test. */
    const FftComplex* const wend = kFftTwiddles + (kFftSize >> 1);
    FftComplex* base = data;
    do {
        const Twiddle tw = twiddle_at(w);
        FftComplex* a = base;
        size_t n = trips >> 1;
        do {
            butterfly<Scale>(a, a + half, tw);
            butterfly<Scale>(a + span, a + span + half, tw);
            a += span << 1;
        } while (--n);
        w += trips;
        ++base;
    } while (w != wend);
}

/* Group outermost, one twiddle load per butterfly: pays from stage 8, where the
 * loop above runs twice or less. Constant Half/Trips give a post-indexed
 * twiddle load and drop stage 10's outer loop. */
template <bool Scale, size_t Half, size_t Trips>
__attribute__((noinline)) void pass_group_outer(FftComplex* data) {
    static_assert(Half * Trips == kFftSize / 2, "one stage covers half the transform");
    static_assert(Trips >= 1, "the group loop is a down-counter");
    const size_t span = Half << 1;
    FftComplex* base = data;
    size_t g = Trips;
    do {
        const FftComplex* w = kFftTwiddles;
        FftComplex* a = base;
        FftComplex* b = base + Half;
        size_t n = Half;
        do {
            butterfly<Scale>(a, b, twiddle_at(w));
            w += Trips;
            ++a;
            ++b;
        } while (--n);
        base += span;
    } while (--g);
}

/* Checks pass_twiddle_outer's assumptions; a wrapper so the body is
 * instantiated once per Scale, not per stage. */
template <bool Scale, size_t Half, size_t Trips>
inline void pass_twiddle(FftComplex* data) {
    static_assert(Half * Trips == kFftSize / 2, "the twiddle pointer IS the k loop's end test");
    static_assert(Trips >= 2 && (Trips % 2) == 0, "the group loop is unrolled two ways with no tail");
    pass_twiddle_outer<Scale>(data, Half, Trips);
}

}  // namespace

size_t fft_bit_reverse_index(size_t index) {
    return reverse_bits32(static_cast<uint32_t>(index)) >> (32 - kStages);
}

/* Not split here: the first advance() runs the whole transform. */
void fft2048_begin(FftProgress& progress, FftComplex* data) {
    progress.data = data;
    progress.step = (data == nullptr) ? kFftSteps : 0;
}

bool fft2048_advance(FftProgress& progress, uint8_t budget) {
    if (progress.step < kFftSteps && budget > 0) {
        fft2048(progress.data, &progress.shift);
        progress.step = kFftSteps;
    }
    return progress.step >= kFftSteps;
}

bool fft2048(FftComplex* data, uint8_t* out_shift) {
    if (data == nullptr)
        return false;

    for (size_t i = 1; i < kFftSize - 1; i++) {
        const size_t j = reverse_bits32(static_cast<uint32_t>(i)) >> (32 - kStages);
        if (i < j) {
            uint32_t x, y;
            __builtin_memcpy(&x, &data[i], sizeof(x));
            __builtin_memcpy(&y, &data[j], sizeof(y));
            __builtin_memcpy(&data[i], &y, sizeof(y));
            __builtin_memcpy(&data[j], &x, sizeof(x));
        }
    }

    pass_twiddle<false, 1, 1024>(data);
    pass_twiddle<false, 2, 512>(data);
    pass_twiddle<false, 4, 256>(data);
    pass_twiddle<true, 8, 128>(data);
    pass_twiddle<false, 16, 64>(data);
    pass_twiddle<true, 32, 32>(data);
    pass_twiddle<false, 64, 16>(data);
    pass_twiddle<true, 128, 8>(data);
    pass_group_outer<false, 256, 4>(data);
    pass_group_outer<true, 512, 2>(data);
    pass_group_outer<false, 1024, 1>(data);

    /* Four halvings, at stages 3, 5, 7 and 9. */
    if (out_shift != nullptr)
        *out_shift = 4;

    return true;
}

} /* namespace dab */
