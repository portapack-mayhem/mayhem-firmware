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

/* Radix-4 transform (out_shift = 4): one twiddle-free radix-2 pass and five
 * radix-4 passes, 6 trips through the array instead of radix-2's 11 and 7680
 * complex multiplies instead of 11264. */

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

/* Load-bearing: with plain inline, -Os leaves cmul() a call (3 bl/butterfly). */
#define DAB_FFT_INLINE inline __attribute__((always_inline))

#if defined(DAB_FFT_PACKED)
DAB_FFT_INLINE uint32_t fft_load(const FftComplex* p) {
    uint32_t v;
    __builtin_memcpy(&v, p, sizeof(v));
    return v;
}

DAB_FFT_INLINE void fft_store(FftComplex* p, uint32_t v) {
    __builtin_memcpy(static_cast<void*>(p), &v, sizeof(v));
}

/* kFftTwiddles holds W^0..W^1023, half the circle; W^(3k) reaches 1533, so
 * the rest is W^(k+1024) == -W^k. Exact: the table never holds -32768. */
DAB_FFT_INLINE uint32_t twiddle(size_t idx) {
    const uint32_t w = fft_load(&kFftTwiddles[idx & (kFftSize / 2 - 1)]);
    return (idx & (kFftSize / 2)) ? dsp_qsub16(0u, w) : w;
}

/* (x*w) >> 15, truncating on the pack: SMUSD, SMUADX, ASR, and PKHBT's own
 * shifter for the imaginary half. */
DAB_FFT_INLINE uint32_t cmul(uint32_t x, uint32_t w) {
    const int32_t tr = dsp_smusd(x, w) >> 15;
    return dsp_pack_lsl1(static_cast<uint32_t>(tr),
                         static_cast<uint32_t>(dsp_smuadx(x, w)));
}
#else
DAB_FFT_INLINE FftComplex twiddle(size_t idx) {
    const FftComplex w = kFftTwiddles[idx & (kFftSize / 2 - 1)];
    if (idx & (kFftSize / 2))
        return FftComplex{static_cast<int16_t>(-w.re), static_cast<int16_t>(-w.im)};
    return w;
}
#endif

/* DIT stages s and s+1, kM == 1 << s. A group is j, j+m, j+2m, j+3m of a 4m
 * block; j+2m takes W^k, j+m W^2k, j+3m W^3k. kHalve halves the first sub-stage,
 * where 3, 5, 7 and 9 land. noinline for its own register frame; kM a constant
 * because as an argument GCC spills the strides. */
template <size_t kM, bool kHalve>
__attribute__((noinline)) void radix4_pass(FftComplex* data) {
    constexpr size_t kStep = kFftSize / (4 * kM);
    constexpr size_t kGroup = 4 * kM;

    for (size_t j = 0; j < kM; j++) {
        const size_t i1 = j * kStep;

#if defined(DAB_FFT_PACKED)
        const uint32_t w1 = twiddle(i1);
        const uint32_t w2 = twiddle(2 * i1);
        const uint32_t w3 = twiddle(3 * i1);

        FftComplex* p = data + j;
        /* Down-counted: an explicit end pointer is spilled at -Os. */
        size_t n = kFftSize / kGroup;
        do {
            const uint32_t a = fft_load(p);
            const uint32_t tc = cmul(fft_load(p + kM), w2);
            const uint32_t tb = cmul(fft_load(p + 2 * kM), w1);
            const uint32_t td = cmul(fft_load(p + 3 * kM), w3);

            const uint32_t s0 = kHalve ? dsp_shadd16(a, tc) : dsp_qadd16(a, tc);
            const uint32_t s1 = kHalve ? dsp_shsub16(a, tc) : dsp_qsub16(a, tc);
            const uint32_t s2 = kHalve ? dsp_shadd16(tb, td) : dsp_qadd16(tb, td);
            const uint32_t s3 = kHalve ? dsp_shsub16(tb, td) : dsp_qsub16(tb, td);

            /* The second sub-stage's -j rotation is free: QSAX and QASX
             * combine s1 and s3 with the halves exchanged. */
            fft_store(p, dsp_qadd16(s0, s2));
            fft_store(p + kM, dsp_qsax(s1, s3));
            fft_store(p + 2 * kM, dsp_qsub16(s0, s2));
            fft_store(p + 3 * kM, dsp_qasx(s1, s3));

            p += kGroup;
        } while (--n);
#else
        const FftComplex w1 = twiddle(i1);
        const FftComplex w2 = twiddle(2 * i1);
        const FftComplex w3 = twiddle(3 * i1);

        FftComplex* p = data + j;
        size_t n = kFftSize / kGroup;
        do {
            FftComplex* const p0 = p;
            FftComplex* const p1 = p + kM;
            FftComplex* const p2 = p + 2 * kM;
            FftComplex* const p3 = p + 3 * kM;

            const int32_t ar = p0->re;
            const int32_t ai = p0->im;

            const int32_t tcr = (int32_t(p1->re) * w2.re - int32_t(p1->im) * w2.im) >> 15;
            const int32_t tci = (int32_t(p1->re) * w2.im + int32_t(p1->im) * w2.re) >> 15;
            const int32_t tbr = (int32_t(p2->re) * w1.re - int32_t(p2->im) * w1.im) >> 15;
            const int32_t tbi = (int32_t(p2->re) * w1.im + int32_t(p2->im) * w1.re) >> 15;
            const int32_t tdr = (int32_t(p3->re) * w3.re - int32_t(p3->im) * w3.im) >> 15;
            const int32_t tdi = (int32_t(p3->re) * w3.im + int32_t(p3->im) * w3.re) >> 15;

            const int32_t s0r = kHalve ? (ar + tcr) >> 1 : ar + tcr;
            const int32_t s0i = kHalve ? (ai + tci) >> 1 : ai + tci;
            const int32_t s1r = kHalve ? (ar - tcr) >> 1 : ar - tcr;
            const int32_t s1i = kHalve ? (ai - tci) >> 1 : ai - tci;
            const int32_t s2r = kHalve ? (tbr + tdr) >> 1 : tbr + tdr;
            const int32_t s2i = kHalve ? (tbi + tdi) >> 1 : tbi + tdi;
            const int32_t s3r = kHalve ? (tbr - tdr) >> 1 : tbr - tdr;
            const int32_t s3i = kHalve ? (tbi - tdi) >> 1 : tbi - tdi;

            p0->re = saturate16(s0r + s2r);
            p0->im = saturate16(s0i + s2i);
            p1->re = saturate16(s1r + s3i);
            p1->im = saturate16(s1i - s3r);
            p2->re = saturate16(s0r - s2r);
            p2->im = saturate16(s0i - s2i);
            p3->re = saturate16(s1r - s3i);
            p3->im = saturate16(s1i + s3r);

            p += kGroup;
        } while (--n);
#endif
    }
}

}  // namespace

size_t fft_bit_reverse_index(size_t index) {
    return reverse_bits32(static_cast<uint32_t>(index)) >> (32 - kStages);
}

/* Permutation, radix-2 stage or one radix-4 pass: consistent between steps. */
static void fft2048_step(FftComplex* data, uint8_t step) {
    switch (step) {
        case 0:
            for (size_t i = 1; i < kFftSize - 1; i++) {
                const size_t j = reverse_bits32(static_cast<uint32_t>(i)) >> (32 - kStages);
                if (i < j) {
#if defined(DAB_FFT_PACKED)
                    const uint32_t t = fft_load(data + i);
                    fft_store(data + i, fft_load(data + j));
                    fft_store(data + j, t);
#else
                    const FftComplex t = data[i];
                    data[i] = data[j];
                    data[j] = t;
#endif
                }
            }
            break;
        case 1:
            /* The unpaired stage goes first, where every twiddle is W^0. */
            for (size_t i = 0; i < kFftSize; i += 2) {
#if defined(DAB_FFT_PACKED)
                const uint32_t a = fft_load(data + i);
                const uint32_t b = fft_load(data + i + 1);
                fft_store(data + i, dsp_qadd16(a, b));
                fft_store(data + i + 1, dsp_qsub16(a, b));
#else
                const int32_t ar = data[i].re, ai = data[i].im;
                const int32_t br = data[i + 1].re, bi = data[i + 1].im;
                data[i].re = saturate16(ar + br);
                data[i].im = saturate16(ai + bi);
                data[i + 1].re = saturate16(ar - br);
                data[i + 1].im = saturate16(ai - bi);
#endif
            }
            break;
        case 2:
            radix4_pass<2, false>(data);
            break; /* stages 1, 2                */
        case 3:
            radix4_pass<8, true>(data);
            break; /* stages 3, 4 -- halve at 3  */
        case 4:
            radix4_pass<32, true>(data);
            break; /* stages 5, 6 -- halve at 5  */
        case 5:
            radix4_pass<128, true>(data);
            break; /* stages 7, 8 -- halve at 7  */
        case 6:
            radix4_pass<512, true>(data);
            break; /* stages 9, 10 -- halve at 9 */
        default:
            break;
    }
}

void fft2048_begin(FftProgress& progress, FftComplex* data) {
    progress.data = data;
    progress.step = (data == nullptr) ? kFftSteps : 0;
}

bool fft2048_advance(FftProgress& progress, uint8_t budget) {
    while (progress.step < kFftSteps && budget > 0) {
        fft2048_step(progress.data, progress.step);
        ++progress.step;
        --budget;
    }
    if (progress.step >= kFftSteps)
        progress.shift = 4;
    return progress.step >= kFftSteps;
}

bool fft2048(FftComplex* data, uint8_t* out_shift) {
    if (data == nullptr)
        return false;

    /* Defined via the incremental path so the two cannot diverge. */
    FftProgress progress;
    fft2048_begin(progress, data);
    fft2048_advance(progress, kFftSteps);

    if (out_shift != nullptr)
        *out_shift = progress.shift;

    return true;
}

} /* namespace dab */
