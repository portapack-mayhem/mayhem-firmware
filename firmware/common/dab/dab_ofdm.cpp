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

/* The packed path is what the device runs; DAB_FFT_SIMD_PORTABLE builds it
 * from dab_simd.hpp's plain-C twins so a host can check it. */
#if defined(__ARM_FEATURE_SIMD32) || defined(DAB_FFT_SIMD_PORTABLE)
#define DAB_FFT_PACKED 1
#endif
#if defined(DAB_FFT_SIMD_PORTABLE)
#define DAB_SIMD_PORTABLE 1
#endif

/* The demapper's packed path is what the device runs; DAB_DEMAP_SIMD_PORTABLE
 * builds it from dab_simd.hpp's plain-C twins so a host can check it. */
#if defined(__ARM_FEATURE_SIMD32) || defined(DAB_DEMAP_SIMD_PORTABLE)
#define DAB_DEMAP_PACKED 1
#endif
#if defined(DAB_DEMAP_SIMD_PORTABLE)
#define DAB_SIMD_PORTABLE 1
#endif

#include "dab_ofdm.hpp"
#include "dab_simd.hpp"
#include "dab_fft_twiddles.hpp"
#include "sine_table_int8.hpp"
#include <cmath>

namespace dab {

/* ---- FFT ---- */

/* Radix-4 transform (out_shift = 4): one twiddle-free radix-2 pass and five
 * radix-4 passes, 6 trips through the array instead of radix-2's 11 and 7680
 * complex multiplies instead of 11264. */

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

/* ---- Phase reference symbol ---- */

namespace {

/* EN 300 401 table 44, mode I: 48 blocks of 32 carriers,
 * phi_k = pi/2 * (h[i][k - k_min] + n). As printed, so a transcription error
 * can be caught by reading; k_max = k_min + 31 is not stored. */
struct PhaseRow {
    int16_t k_min;
    int8_t i;
    int8_t n;
};

constexpr PhaseRow kTable44[48] = {
    {-768, 0, 1},
    {-736, 1, 2},
    {-704, 2, 0},
    {-672, 3, 1},
    {-640, 0, 3},
    {-608, 1, 2},
    {-576, 2, 2},
    {-544, 3, 3},
    {-512, 0, 2},
    {-480, 1, 1},
    {-448, 2, 2},
    {-416, 3, 3},
    {-384, 0, 1},
    {-352, 1, 2},
    {-320, 2, 3},
    {-288, 3, 3},
    {-256, 0, 2},
    {-224, 1, 2},
    {-192, 2, 2},
    {-160, 3, 1},
    {-128, 0, 1},
    {-96, 1, 3},
    {-64, 2, 1},
    {-32, 3, 2},
    {1, 0, 3},
    {33, 3, 1},
    {65, 2, 1},
    {97, 1, 1},
    {129, 0, 2},
    {161, 3, 2},
    {193, 2, 1},
    {225, 1, 0},
    {257, 0, 2},
    {289, 3, 2},
    {321, 2, 3},
    {353, 1, 3},
    {385, 0, 0},
    {417, 3, 2},
    {449, 2, 1},
    {481, 1, 3},
    {513, 0, 3},
    {545, 3, 3},
    {577, 2, 3},
    {609, 1, 0},
    {641, 0, 3},
    {673, 3, 0},
    {705, 2, 1},
    {737, 1, 1},
};

/* EN 300 401 table 43, unfolded as printed. */
constexpr int8_t kH[4][32] = {
    {0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1,
     0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1},
    {0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0,
     0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0},
    {0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3,
     0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3},
    {0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2,
     0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2},
};

constexpr size_t kRows = 48;
constexpr size_t kBlockCarriers = 32;
constexpr size_t kPackedBytes = kCarriers / 4;

/* The bins the standard leaves empty: DC, and everything above the top
 * carrier up to the bottom one, which wraps to the negative half. */
constexpr size_t kFirstEmptyBin = kCarriers / 2 + 1;
constexpr size_t kLastEmptyBin = kFftSize - kCarriers / 2 - 1;

struct PackedPhases {
    uint8_t byte[kPackedBytes];
};

/* An overlap would OR two phases in build_packed(); a gap would leave a
 * silent zero. */
constexpr size_t table_coverage() {
    bool seen[kCarriers] = {};
    size_t n = 0;
    for (size_t r = 0; r < kRows; ++r) {
        for (size_t j = 0; j < kBlockCarriers; ++j) {
            const int k = kTable44[r].k_min + static_cast<int>(j);
            if (k == 0 || k < -768 || k > 768)
                return 0;
            const size_t pos = carrier_position(k);
            if (seen[pos])
                return 0;
            seen[pos] = true;
            ++n;
        }
    }
    return n;
}

static_assert(table_coverage() == kCarriers,
              "table 44 must tile all 1536 carriers exactly once");

/* Compile time, so only the 384-byte result reaches the image. */
constexpr PackedPhases build_packed() {
    PackedPhases p{};
    for (size_t r = 0; r < kRows; ++r) {
        for (size_t j = 0; j < kBlockCarriers; ++j) {
            const int k = kTable44[r].k_min + static_cast<int>(j);
            const size_t pos = carrier_position(k);
            const uint8_t v = static_cast<uint8_t>((kH[kTable44[r].i][j] + kTable44[r].n) & 3);
            p.byte[pos >> 2] |= static_cast<uint8_t>(v << (2 * (pos & 3)));
        }
    }
    return p;
}

constexpr PackedPhases kPacked = build_packed();

inline uint8_t phase_at(size_t index) {
    return static_cast<uint8_t>((kPacked.byte[index >> 2] >> (2 * (index & 3))) & 3);
}

}  // namespace

uint8_t prs_phase(size_t index) {
    return index < kCarriers ? phase_at(index) : 0;
}

uint8_t prs_phase_of_carrier(int16_t k) {
    if (k == 0 || k < -768 || k > 768)
        return 0xFF;
    return phase_at(carrier_position(k));
}

void prs_conjugate_multiply(FftComplex* bins, size_t bin_shift) {
    if (bins == nullptr)
        return;

    /* conj(exp(j*pi/2*n)) is 1, -j, -1, +j: a swap and a sign, no multiply. */
    for (size_t p = 0; p < kCarriers; ++p) {
        const int16_t k = prs_carrier_at(p);
        const size_t bin = (static_cast<size_t>(k + static_cast<int>(kFftSize)) + bin_shift) & (kFftSize - 1);
        FftComplex& c = bins[bin];
        const int16_t re = c.re;
        const int16_t im = c.im;
        switch (phase_at(p)) {
            case 0:
                break;
            case 1:
                c.re = im;
                c.im = static_cast<int16_t>(-static_cast<int>(re));
                break;
            case 2:
                c.re = static_cast<int16_t>(-static_cast<int>(re));
                c.im = static_cast<int16_t>(-static_cast<int>(im));
                break;
            default:
                c.re = static_cast<int16_t>(-static_cast<int>(im));
                c.im = re;
                break;
        }
    }

    /* Empty bins would add a flat pedestal to the impulse response's peak
     * search. */
    bins[bin_shift & (kFftSize - 1)] = FftComplex{0, 0};
    for (size_t b = kFirstEmptyBin; b <= kLastEmptyBin; ++b)
        bins[(b + bin_shift) & (kFftSize - 1)] = FftComplex{0, 0};
}

/* ---- Frequency interleaving ---- */

size_t build_frequency_map(int16_t* out, size_t n) {
    if (out == nullptr || n != kCarriers)
        return 0;

    size_t written = 0;
    uint32_t pi = 0;

    /* One full cycle of the affine map visits all 2048 residues exactly once. */
    for (size_t i = 0; i < 2048 && written < n; ++i) {
        pi = (13 * pi + 511) & 2047;
        if (pi >= 256 && pi <= 1792 && pi != 1024)
            out[written++] = static_cast<int16_t>(static_cast<int32_t>(pi) - 1024);
    }

    return written;
}

const int16_t* frequency_map_inverse() {
    static int16_t inverse[kCarriers];
    static bool built = false;
    if (!built) {
        const int16_t* forward = frequency_map();

        /* forward[k] is the carrier that logical index k reads. */
        static size_t perm_inv[kCarriers];
        for (size_t k = 0; k < kCarriers; ++k)
            perm_inv[carrier_position(forward[k])] = k;

        for (size_t k = 0; k < kCarriers; ++k)
            inverse[k] = carrier_at_position(perm_inv[k]);

        built = true;
    }
    return inverse;
}

const int16_t* frequency_map() {
    static int16_t map[kCarriers];
    static bool built = false;
    if (!built) {
        build_frequency_map(map, kCarriers);
        built = true;
    }
    return map;
}

/* ---- OFDM demodulation ---- */

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
inline int32_t lane_lo(uint32_t w) {
    return static_cast<int16_t>(w & 0xFFFFu);
}
inline int32_t lane_hi(uint32_t w) {
    return static_cast<int16_t>(w >> 16);
}
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
 * costs more than the dual multiplies save. Built into .bss:
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
     * lets strong carriers crush the rest to near-erasures. A
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
         * plus SMMUL rounds toward -inf, a one-sided soft bias. */
        const int32_t rs = ((real / mag) * scale_q) >> kScaleShift;
        const int32_t is = ((imag / mag) * scale_q) >> kScaleShift;

        /* clamp127_neg is the negation the header describes. */
        soft_out[k] = static_cast<int8_t>(clamp127_neg(rs));
        soft_out[k + count] = static_cast<int8_t>(clamp127_neg(is));
    }
}

} /* namespace dab */
