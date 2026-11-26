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

#include "dab_aac_lc.hpp"
#include "dab_aac_tables.hpp"
#include "dab_aac_pow43.hpp"
#include "dab_aac_windows.hpp"

#include <cmath>
#include <cstring>

/* nano.specs' <cmath> does not define M_PI in a strict C++ mode. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -Os will not inline a leaf returning a Cplx; cmul() and pre_rotation() run
 * ~1,900 times a unit. */
#if defined(__GNUC__) || defined(__clang__)
#define DAB_AAC_INLINE inline __attribute__((always_inline))
#else
#define DAB_AAC_INLINE inline
#endif

namespace dab {

namespace {

/* Syntactic element ids. */
constexpr uint8_t kIdSce = 0, kIdCpe = 1, kIdDse = 4, kIdFil = 6, kIdEnd = 7;

constexpr uint8_t kOnlyLong = 0, kLongStart = 1, kEightShort = 2, kLongStop = 3;

constexpr uint8_t kZeroHcb = 0, kNoiseHcb = 13, kIntensityHcb2 = 14, kIntensityHcb = 15;

/* Errors, reported through last_error(). */
enum : uint8_t {
    kErrNone = 0,
    kErrOverrun = 1,
    kErrElement = 2,
    kErrWindow = 3,
    kErrMaxSfb = 4,
    kErrSection = 5,
    kErrHuffman = 6,
    kErrPredictor = 7,
    kErrGainControl = 8,
    kErrScaleFactor = 9,
};

/* 960-sample frames at 24 kHz: the 1024 table truncated to 46 bands, the last
 * running to 960. */
constexpr uint16_t kSwbLong24[] = {
    0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68,
    76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220,
    240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704,
    768, 832, 896, 960};
constexpr uint8_t kNumSwbLong24 = 46;

/* The 128-sample table with its last band clipped to 120. */
constexpr uint16_t kSwbShort24[] = {0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 120};
constexpr uint8_t kNumSwbShort24 = 15;

constexpr uint8_t kTnsMaxBandsLong24 = 46;
constexpr uint8_t kTnsMaxBandsShort24 = 14;
constexpr uint8_t kTnsMaxOrderLong = 12;
constexpr uint8_t kTnsMaxOrderShort = 7;

/* TNS reflection coefficients by transmitted value; the table is chosen by
 * (coef_compress, coef_res + 3). */
constexpr float kTnsCoef[4][16] = {
    {0.0f, 0.4338837391f, 0.7818314825f, 0.9749279122f,
     -0.9848077530f, -0.8660254038f, -0.6427876097f, -0.3420201433f,
     -0.4338837391f, -0.7818314825f, -0.9749279122f, -0.9749279122f,
     -0.9848077530f, -0.8660254038f, -0.6427876097f, -0.3420201433f},
    {0.0f, 0.2079116908f, 0.4067366431f, 0.5877852523f,
     0.7431448255f, 0.8660254038f, 0.9510565163f, 0.9945218954f,
     -0.9957341763f, -0.9618256432f, -0.8951632914f, -0.7980172273f,
     -0.6736956436f, -0.5264321629f, -0.3612416662f, -0.1837495178f},
    {0.0f, 0.4338837391f, -0.6427876097f, -0.3420201433f,
     0.9749279122f, 0.7818314825f, -0.6427876097f, -0.3420201433f,
     -0.4338837391f, -0.7818314825f, -0.6427876097f, -0.3420201433f,
     -0.7818314825f, -0.4338837391f, -0.6427876097f, -0.3420201433f},
    {0.0f, 0.2079116908f, 0.4067366431f, 0.5877852523f,
     -0.6736956436f, -0.5264321629f, -0.3612416662f, -0.1837495178f,
     0.9945218954f, 0.9510565163f, 0.8660254038f, 0.7431448255f,
     -0.6736956436f, -0.5264321629f, -0.3612416662f, -0.1837495178f},
};

struct BookInfo {
    uint8_t values; /* 4 = quad, 2 = pair */
    uint8_t radix;
    int8_t bias;         /* signed books shift each value down by this */
    uint8_t unsigned_cb; /* magnitudes in the codeword, signs appended */
};

/* Indexed by codebook - 1. */
constexpr BookInfo kBooks[11] = {
    {4, 3, 1, 0},
    {4, 3, 1, 0},
    {4, 3, 0, 1},
    {4, 3, 0, 1},
    {2, 9, 4, 0},
    {2, 9, 4, 0},
    {2, 8, 0, 1},
    {2, 8, 0, 1},
    {2, 13, 0, 1},
    {2, 13, 0, 1},
    {2, 17, 0, 1},
};

struct Codebook {
    const uint8_t* counts;
    uint8_t maxlen;
    const uint16_t* symbols;
};

#define BOOK(n) {aac::kHcb##n##_counts, uint8_t(sizeof(aac::kHcb##n##_counts) - 1), aac::kHcb##n##_symbols}
constexpr Codebook kCodebooks[11] = {
    BOOK(1),
    BOOK(2),
    BOOK(3),
    BOOK(4),
    BOOK(5),
    BOOK(6),
    BOOK(7),
    BOOK(8),
    BOOK(9),
    BOOK(10),
    BOOK(11),
};
constexpr Codebook kScaleFactorBook = BOOK(Sf);
#undef BOOK

/* No cbrtf in the nano libc: a bit-twiddled estimate and two Newton steps,
 * exact enough for 16-bit output. */
inline float fast_cbrt(float x) {
    union {
        float f;
        uint32_t i;
    } u{x};
    /* u.i / 3 as an exact multiply-and-shift ((x * 0xAAAAAAAB) >> 33 == x / 3
     * for every uint32): umull is 1 cycle, the udiv -Os prefers 2-12. */
    u.i = uint32_t((uint64_t(u.i) * 0xAAAAAAABull) >> 33) + 709921077u;
    float y = u.f;
    y = y - (y - x / (y * y)) * (1.0f / 3.0f);
    y = y - (y - x / (y * y)) * (1.0f / 3.0f);
    return y;
}

/* |q|^(4/3): aac::kPow43 for the 256 magnitudes codebooks and pulses produce,
 * fast_cbrt for codebook 11's rare escapes. The table holds fast_cbrt's own
 * floats, so both paths agree. */
inline float pow43(int32_t q) {
    if (q == 0) return 0.0f;
    /* Rather than -q: well defined for INT32_MIN. */
    const uint32_t a = q < 0 ? uint32_t(0u - uint32_t(q)) : uint32_t(q);
    const float af = float(a);
    const float r = (a < 256) ? aac::kPow43[a] : af * fast_cbrt(af);
    return q < 0 ? -r : r;
}

/* 2^(e/4) from the float exponent field: newlib's exp2f is powf, ~2.4 KB. The
 * exponent is clamped: apply_pulses can reach a scalefactor from the unbounded
 * noise path, and an out-of-range shift builds a NaN or an infinity. */
inline float exp2_quarter(int32_t e) {
    static constexpr float kQuarterStep[4] = {
        1.000000000e+00f, 1.189207077e+00f, 1.414213538e+00f, 1.681792855e+00f};
    int32_t q = e >> 2; /* arithmetic shift: floor division, negatives included */
    const int32_t r = e & 3;
    if (q > 100)
        q = 100;
    else if (q < -100)
        q = -100;
    union {
        uint32_t i;
        float f;
    } p{uint32_t(127 + q) << 23};
    return p.f * kQuarterStep[r];
}

/* x^(3/4) as sqrt(x) * sqrt(sqrt(x)): two hardware square roots instead of
 * powf, and x^3 is never formed so nothing overflows. */
inline float pow34(float x) {
    const float r = std::sqrt(x);
    return r * std::sqrt(r);
}

}  // namespace

/* Bit by bit on purpose: the average call reads ~1 bit, and -Os outlines a
 * shift-and-mask window behind a trampoline. decode_symbol() inlines its own. */
uint32_t AacLcDecoder::BitReader::read(uint8_t n) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < n; i++) {
        const size_t b = bit + i;
        const uint32_t x = (b < bytes * 8) ? ((data[b >> 3] >> (7 - (b & 7))) & 1u) : 0u;
        v = (v << 1) | x;
    }
    bit += n;
    return v;
}

namespace {

/* Canonical Huffman: walk lengths with a running codeword and the first symbol
 * index of the current length. The bit fetch is inlined: -Os will not inline
 * br.read(1), ~40 cycles a bit. `bit` ends where read(1) would leave it. */
int32_t decode_symbol(AacLcDecoder::BitReader& br, const Codebook& cb) {
    const uint8_t* const data = br.data;
    const size_t nbits = br.bytes * 8;
    size_t pos = br.bit;
    int32_t code = 0, first = 0, index = 0;
    for (uint8_t len = 1; len <= cb.maxlen; len++) {
        const uint32_t x = (pos < nbits) ? ((data[pos >> 3] >> (7 - (pos & 7))) & 1u) : 0u;
        pos++;
        code |= int32_t(x);
        const int32_t count = cb.counts[len];
        if (code - first < count) {
            br.bit = pos;
            return cb.symbols[index + code - first];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    br.bit = pos;
    return -1;
}

}  // namespace

void AacLcDecoder::reset() {
    std::memset(overlap_, 0, sizeof(overlap_));
    primed_ = false;
    prev_window_shape_ = 0;
    error_ = kErrNone;
}

bool AacLcDecoder::configure(uint32_t sample_rate, uint8_t channels, float* scratch, size_t scratch_floats) {
    if (sample_rate != 24000 || channels < 1 || channels > 2)
        return false;
    if (scratch == nullptr || scratch_floats < kScratchFloats)
        return false;
    scratch_ = scratch;
    configured_ = true;
    reset();
    return true;
}

bool AacLcDecoder::decode_ics_info(BitReader& br, Ics& ics) {
    br.skip(1);  // ics_reserved_bit
    ics.window_sequence = uint8_t(br.read(2));
    ics.window_shape = uint8_t(br.read(1));

    if (ics.window_sequence == kEightShort) {
        ics.max_sfb = uint8_t(br.read(4));
        const uint8_t grouping = uint8_t(br.read(7));

        ics.num_windows = 8;
        ics.num_window_groups = 1;
        ics.window_group_length[0] = 1;
        for (int i = 0; i < 7; i++) {
            if ((grouping >> (6 - i)) & 1) {
                ics.window_group_length[ics.num_window_groups - 1]++;
            } else {
                ics.window_group_length[ics.num_window_groups] = 1;
                ics.num_window_groups++;
            }
        }
        ics.num_swb = kNumSwbShort24;
        for (uint8_t i = 0; i <= kNumSwbShort24; i++)
            ics.swb_offset[i] = kSwbShort24[i];
    } else {
        ics.max_sfb = uint8_t(br.read(6));
        if (br.read(1) != 0) {  // predictor_data_present: main profile only
            error_ = kErrPredictor;
            return false;
        }
        ics.num_windows = 1;
        ics.num_window_groups = 1;
        ics.window_group_length[0] = 1;
        ics.num_swb = kNumSwbLong24;
        for (uint8_t i = 0; i <= kNumSwbLong24; i++)
            ics.swb_offset[i] = kSwbLong24[i];
    }

    if (ics.max_sfb > ics.num_swb) {
        error_ = kErrMaxSfb;
        return false;
    }
    return true;
}

bool AacLcDecoder::decode_section_data(BitReader& br, Ics& ics) {
    const uint8_t bits = (ics.window_sequence == kEightShort) ? 3 : 5;
    const uint32_t esc = (1u << bits) - 1;

    std::memset(ics.sfb_cb, 0, sizeof(ics.sfb_cb));
    for (uint8_t g = 0; g < ics.num_window_groups; g++) {
        uint8_t k = 0;
        while (k < ics.max_sfb) {
            const uint8_t cb = uint8_t(br.read(4));
            uint32_t len = 0, incr = br.read(bits);
            while (incr == esc) {
                len += esc;
                incr = br.read(bits);
            }
            len += incr;
            if (k + len > ics.max_sfb || br.overrun()) {
                error_ = kErrSection;
                return false;
            }
            for (uint32_t i = 0; i < len; i++)
                ics.sfb_cb[g][k + i] = cb;
            k = uint8_t(k + len);
        }
    }
    return true;
}

bool AacLcDecoder::decode_scale_factors(BitReader& br, Ics& ics, uint8_t global_gain) {
    int32_t sf = global_gain;
    int32_t noise = int32_t(global_gain) - 90;
    bool noise_started = false;

    for (uint8_t g = 0; g < ics.num_window_groups; g++) {
        for (uint8_t sfb = 0; sfb < ics.max_sfb; sfb++) {
            const uint8_t cb = ics.sfb_cb[g][sfb];
            if (cb == kZeroHcb) {
                ics.scale_factor[g][sfb] = 0;
                continue;
            }
            if (cb == kIntensityHcb || cb == kIntensityHcb2) {
                /* Intensity stereo cannot occur in a single channel element. */
                error_ = kErrScaleFactor;
                return false;
            }
            if (cb == kNoiseHcb) {
                /* Perceptual noise substitution: the first band carries a
                 * 9-bit absolute level, later ones a Huffman-coded delta. */
                if (!noise_started) {
                    noise += int32_t(br.read(9));
                    noise_started = true;
                } else {
                    const int32_t d = decode_symbol(br, kScaleFactorBook);
                    if (d < 0) {
                        error_ = kErrHuffman;
                        return false;
                    }
                    noise += d - 60;
                }
                ics.scale_factor[g][sfb] = int16_t(noise);
                continue;
            }
            const int32_t d = decode_symbol(br, kScaleFactorBook);
            if (d < 0) {
                error_ = kErrHuffman;
                return false;
            }
            sf += d - 60;
            if (sf < 0 || sf > 255) {
                error_ = kErrScaleFactor;
                return false;
            }
            ics.scale_factor[g][sfb] = int16_t(sf);
        }
    }
    return true;
}

bool AacLcDecoder::decode_pulse_data(BitReader& br, const Ics& ics) {
    pulse_count_ = uint8_t(br.read(2) + 1);
    pulse_start_sfb_ = uint8_t(br.read(6));
    if (pulse_start_sfb_ > ics.num_swb) {
        error_ = kErrSection;
        return false;
    }
    for (uint8_t i = 0; i < pulse_count_; i++) {
        pulse_offset_[i] = uint16_t(br.read(5));
        pulse_amp_[i] = uint8_t(br.read(4));
    }
    return true;
}

bool AacLcDecoder::decode_tns_data(BitReader& br, Ics& ics) {
    const bool is_short = ics.window_sequence == kEightShort;
    const uint8_t n_filt_bits = is_short ? 1 : 2;
    const uint8_t length_bits = is_short ? 4 : 6;
    const uint8_t order_bits = is_short ? 3 : 5;
    const uint8_t max_order = is_short ? kTnsMaxOrderShort : kTnsMaxOrderLong;

    for (uint8_t w = 0; w < ics.num_windows; w++) {
        ics.tns_n_filt[w] = uint8_t(br.read(n_filt_bits));
        if (ics.tns_n_filt[w] == 0)
            continue;
        ics.tns_coef_res[w] = uint8_t(br.read(1));
        for (uint8_t f = 0; f < ics.tns_n_filt[w] && f < 3; f++) {
            ics.tns_length[w][f] = uint8_t(br.read(length_bits));
            const uint8_t order = uint8_t(br.read(order_bits));
            ics.tns_order[w][f] = order;
            if (order > max_order) {
                error_ = kErrSection;
                return false;
            }
            if (order == 0)
                continue;
            ics.tns_direction[w][f] = uint8_t(br.read(1));
            ics.tns_coef_compress[w][f] = uint8_t(br.read(1));
            const uint8_t coef_bits = uint8_t(ics.tns_coef_res[w] + 3 - ics.tns_coef_compress[w][f]);
            for (uint8_t i = 0; i < order; i++)
                ics.tns_coef[w][f][i] = int8_t(br.read(coef_bits));
        }
    }
    return true;
}

bool AacLcDecoder::decode_spectral_data(BitReader& br, const Ics& ics) {
    std::memset(spec_, 0, sizeof(spec_));

    const uint16_t window_len = (ics.window_sequence == kEightShort) ? uint16_t(kAacShortLength)
                                                                     : uint16_t(kAacFrameLength);
    uint8_t window_base = 0; /* first window of the current group */

    for (uint8_t g = 0; g < ics.num_window_groups; g++) {
        const uint8_t group_windows = ics.window_group_length[g];
        for (uint8_t sfb = 0; sfb < ics.max_sfb; sfb++) {
            const uint8_t cb = ics.sfb_cb[g][sfb];
            if (cb == kZeroHcb || cb >= kNoiseHcb)
                continue;
            if (cb > 11) {
                error_ = kErrSection;
                return false;
            }
            const BookInfo& info = kBooks[cb - 1];
            const Codebook& book = kCodebooks[cb - 1];
            const uint16_t lo = ics.swb_offset[sfb];
            const uint16_t hi = ics.swb_offset[sfb + 1];
            /* Dequantise here: keeping quantised values would cost 1.9 KB RAM. */
            const float gain = exp2_quarter(int32_t(ics.scale_factor[g][sfb]) - 100);

            for (uint8_t w = 0; w < group_windows; w++) {
                float* out = &spec_[size_t(window_base + w) * window_len];
                for (uint16_t k = lo; k < hi; k += info.values) {
                    const int32_t sym = decode_symbol(br, book);
                    if (sym < 0 || br.overrun()) {
                        error_ = kErrHuffman;
                        return false;
                    }
                    /* The symbol is the tuple in the book's radix, most
                     * significant value first. */
                    int32_t v[4];
                    int32_t rest = sym;
                    for (int i = info.values - 1; i >= 0; i--) {
                        v[i] = rest % info.radix;
                        rest /= info.radix;
                    }
                    /* An unsigned book sends every non-zero value's sign bit
                     * first, then codebook 11's escapes; per value loses sync. */
                    if (info.unsigned_cb) {
                        uint8_t neg = 0;
                        for (uint8_t i = 0; i < info.values; i++)
                            if (v[i] != 0 && br.read(1))
                                neg |= uint8_t(1u << i);
                        if (cb == 11) {
                            for (uint8_t i = 0; i < info.values; i++) {
                                if (v[i] != 16)
                                    continue;
                                /* Ones terminated by a zero give the exponent,
                                 * then that many plus four value bits. */
                                uint8_t n = 0;
                                while (br.read(1) == 1 && n < 16)
                                    n++;
                                v[i] = int32_t((1u << (n + 4)) | br.read(uint8_t(n + 4)));
                                if (v[i] > 8191)
                                    v[i] = 8191;
                            }
                        }
                        for (uint8_t i = 0; i < info.values; i++)
                            if (neg & (1u << i))
                                v[i] = -v[i];
                    } else {
                        for (uint8_t i = 0; i < info.values; i++)
                            v[i] -= info.bias;
                    }
                    for (uint8_t i = 0; i < info.values; i++)
                        if (k + i < hi)
                            out[k + i] = pow43(v[i]) * gain;
                }
            }
        }
        window_base = uint8_t(window_base + group_windows);
    }
    return !br.overrun();
}

/* Pulses adjust the quantised magnitude of at most four coefficients: undoing
 * and redoing the power law is cheaper than keeping every quantised value. */
void AacLcDecoder::apply_pulses(const Ics& ics) {
    if (!pulse_present_ || ics.window_sequence == kEightShort)
        return;
    uint16_t k = ics.swb_offset[pulse_start_sfb_];
    uint8_t sfb = pulse_start_sfb_;
    for (uint8_t i = 0; i < pulse_count_; i++) {
        k = uint16_t(k + pulse_offset_[i]);
        if (k >= kAacFrameLength)
            break;
        while (sfb + 1 < ics.max_sfb && ics.swb_offset[sfb + 1] <= k)
            sfb++;
        const float gain = exp2_quarter(int32_t(ics.scale_factor[0][sfb]) - 100);
        if (gain <= 0.0f)
            continue;
        const float scaled = spec_[k] / gain;
        const float mag = scaled < 0 ? -scaled : scaled;
        int32_t q = int32_t(std::lround(pow34(mag)));
        if (scaled < 0)
            q = -q;
        q = (q > 0) ? q + pulse_amp_[i] : q - pulse_amp_[i];
        spec_[k] = pow43(q) * gain;
    }
}

void AacLcDecoder::apply_tns(const Ics& ics) {
    const bool is_short = ics.window_sequence == kEightShort;
    const uint16_t window_len = is_short ? uint16_t(kAacShortLength) : uint16_t(kAacFrameLength);
    const uint8_t max_band = is_short ? kTnsMaxBandsShort24 : kTnsMaxBandsLong24;
    const uint8_t max_order = is_short ? kTnsMaxOrderShort : kTnsMaxOrderLong;

    for (uint8_t w = 0; w < ics.num_windows; w++) {
        uint16_t bottom = ics.num_swb;
        for (uint8_t f = 0; f < ics.tns_n_filt[w] && f < 3; f++) {
            const uint16_t top = bottom;
            bottom = (top > ics.tns_length[w][f]) ? uint16_t(top - ics.tns_length[w][f]) : 0;
            uint8_t order = ics.tns_order[w][f];
            if (order == 0)
                continue;
            if (order > max_order)
                order = max_order;

            /* Reflection coefficients to LPC, by the usual step-up. */
            float lpc[kTnsMaxOrderLong + 1];
            const float* tbl = kTnsCoef[2 * (ics.tns_coef_compress[w][f] != 0) +
                                        (ics.tns_coef_res[w] != 0)];
            lpc[0] = 1.0f;
            for (uint8_t m = 1; m <= order; m++) {
                float b[kTnsMaxOrderLong + 1];
                lpc[m] = tbl[uint8_t(ics.tns_coef[w][f][m - 1]) & 0x0F];
                for (uint8_t i = 1; i < m; i++)
                    b[i] = lpc[i] + lpc[m] * lpc[m - i];
                for (uint8_t i = 1; i < m; i++)
                    lpc[i] = b[i];
            }

            uint16_t start = bottom < max_band ? bottom : max_band;
            start = start < ics.max_sfb ? start : ics.max_sfb;
            start = ics.swb_offset[start];
            uint16_t end = top < max_band ? top : max_band;
            end = end < ics.max_sfb ? end : ics.max_sfb;
            end = ics.swb_offset[end];
            if (end <= start)
                continue;

            const uint16_t size = uint16_t(end - start);
            int step = 1;
            uint16_t at = start;
            if (ics.tns_direction[w][f]) {
                step = -1;
                at = uint16_t(end - 1);
            }

            /* All-pole synthesis over the band. */
            float state[kTnsMaxOrderLong] = {0.0f};
            float* s = &spec_[size_t(w) * window_len];
            for (uint16_t i = 0; i < size; i++) {
                float y = s[at];
                for (uint8_t j = 0; j < order; j++)
                    y -= state[j] * lpc[j + 1];
                for (uint8_t j = uint8_t(order - 1); j > 0; j--)
                    state[j] = state[j - 1];
                state[0] = y;
                s[at] = y;
                at = uint16_t(int(at) + step);
            }
        }
    }
}

namespace {

/* Complex FFT for n = 15 * 2^m (480 long, 60 short): a fifteen-point DFT and a
 * radix-2 FFT. Unnormalised, +i direction, as the IMDCT expects. */
constexpr size_t kFftMax = 480;
constexpr size_t kRadix1 = 15;

struct Cplx {
    float re, im;
};

/* exp(+2*pi*i*m/480), first quadrant only: the rest by symmetry, smaller sizes
 * by stride. In flash, as RAM is scarcer (dab_aac_windows.hpp). */
constexpr size_t kQuadrant = kFftMax / 4;

constexpr Cplx twiddle_quadrant(size_t m) {
    return {aac::kTwiddleQuadrant[m][0], aac::kTwiddleQuadrant[m][1]};
}

/* Compile time only: the two tables below are its whole call graph. Do not
 * call it from the transform; at -Os it is a real call with a udiv. */
constexpr Cplx twiddle_raw(size_t m) {
    m %= kFftMax;
    if (m <= kQuadrant)
        return twiddle_quadrant(m);
    if (m <= 2 * kQuadrant) {
        const Cplx q = twiddle_quadrant(2 * kQuadrant - m);
        return {-q.re, q.im};
    }
    if (m <= 3 * kQuadrant) {
        const Cplx q = twiddle_quadrant(m - 2 * kQuadrant);
        return {-q.re, -q.im};
    }
    const Cplx q = twiddle_quadrant(kFftMax - m);
    return {q.re, -q.im};
}

/* Built at compile time: 3,960 bytes of flash, no RAM. kW15 copies every 32nd
 * kW480 entry rather than striding: the fifteen-point DFT reads it ~7,200 times
 * a unit, and 120 contiguous bytes stay in the flash accelerator's line. */
template <size_t N, size_t Stride>
struct TwiddleTable {
    Cplx w[N];
    constexpr TwiddleTable()
        : w{} {
        for (size_t m = 0; m < N; m++)
            w[m] = twiddle_raw(m * Stride);
    }
};

constexpr TwiddleTable<kFftMax, 1> kW480{};
constexpr TwiddleTable<kRadix1, kFftMax / kRadix1> kW15{};

DAB_AAC_INLINE Cplx cmul(const Cplx& a, const Cplx& b) {
    return {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
}

/* exp(2*pi*i*(k + 1/8)/n), n = 1920 or 240, without cosf/sinf (~3 KB). In 1/480
 * units the angle is a quadrant index plus one of five remainders, (k >> 2) +
 * (k & 3 + 1/8)/4 or 2k + 1/4, so one complex multiply does it (~2 ulp).
 * `is_short`, not n, so a third transform size cannot take the wrong branch. */
constexpr Cplx kRotLong[4] = {
    /* exp(2*pi*i*(r + 1/8)/1920) */
    {9.999999404e-01f, 4.090615257e-04f},
    {9.999932051e-01f, 3.681545611e-03f},
    {9.999758005e-01f, 6.953990087e-03f},
    {9.999477267e-01f, 1.022636052e-02f},
};
constexpr Cplx kRotShort = {9.999946356e-01f, 3.272486618e-03f}; /* exp(2*pi*i/1920) */

static_assert(2 * kAacFrameLength == 4 * kFftMax,
              "the long transform's angles must be quarters of the twiddle grid");
static_assert(4 * kAacShortLength == kFftMax,
              "the short transform's angles must be doubles of the twiddle grid");
static_assert((kAacFrameLength / 2 - 1) / 4 <= kQuadrant, "long index leaves the quadrant");
static_assert(2 * (kAacShortLength / 2 - 1) <= kQuadrant, "short index leaves the quadrant");
static_assert((kAacFrameLength / 2) % 2 == 0 && (kAacShortLength / 2) % 2 == 0,
              "n4 must be even, or the paired in-place pre-rotation meets itself");

DAB_AAC_INLINE Cplx pre_rotation(size_t k, bool is_short) {
    if (is_short)
        return cmul(twiddle_quadrant(2 * k), kRotShort);
    return cmul(twiddle_quadrant(k >> 2), kRotLong[k & 3]);
}

void fft_pow2(Cplx* x, size_t n) {
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            const Cplx t = x[i];
            x[i] = x[j];
            x[j] = t;
        }
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const size_t half = len / 2;
        /* twiddle(k*(n/len), n) is kW480[k * kFftMax/len] whatever n is, and
         * k < len/2 keeps the index inside the table: no modulo needed. */
        const size_t wstep = kFftMax / len;
        for (size_t i = 0; i < n; i += len) {
            size_t wi = 0;
            for (size_t k = 0; k < half; k++) {
                const Cplx w = kW480.w[wi];
                wi += wstep;
                const Cplx u = x[i + k];
                const Cplx v = cmul(x[i + k + half], w);
                x[i + k] = {u.re + v.re, u.im + v.im};
                x[i + k + half] = {u.re - v.re, u.im - v.im};
            }
        }
    }
}

/* The IMDCT's fifteen-point DFT as 3 x 5 Cooley-Tukey, j = 5*a + b,
 * k = k1 + 3*k2: ~100 real multiplies against the direct sum's 900.
 * W3 = kW15[5], W5 = kW15[3], W5^2 = kW15[6]. Not bit-identical to the direct
 * sum (summation order) but no less accurate: worst error 6.9e-7 of the
 * vector's rms. */
void dft15_ct(const Cplx* in, size_t in_stride, Cplx* out) {
    constexpr float c3 = kW15.w[5].re, s3 = kW15.w[5].im; /* exp(2*pi*i/3) */
    constexpr float c1 = kW15.w[3].re, s1 = kW15.w[3].im; /* exp(2*pi*i/5) */
    constexpr float c2 = kW15.w[6].re, s2 = kW15.w[6].im; /* exp(4*pi*i/5) */

    /* Five radix-3 DFTs, twiddled on the way out. */
    Cplx a[5][3];
    for (size_t b = 0; b < 5; b++) {
        const Cplx t0 = in[in_stride * b];
        const Cplx t1 = in[in_stride * (b + 5)];
        const Cplx t2 = in[in_stride * (b + 10)];
        const float sr = t1.re + t2.re, si = t1.im + t2.im;
        const float dr = t1.re - t2.re, di = t1.im - t2.im;
        const float br = t0.re + c3 * sr, bi = t0.im + c3 * si;
        a[b][0] = {t0.re + sr, t0.im + si};
        a[b][1] = {br - s3 * di, bi + s3 * dr};
        a[b][2] = {br + s3 * di, bi - s3 * dr};
        if (b != 0) {
            a[b][1] = cmul(a[b][1], kW15.w[b]);
            a[b][2] = cmul(a[b][2], kW15.w[(2 * b) % kRadix1]);
        }
    }

    /* Three radix-5 DFTs, which land the output directly at k = k1 + 3*k2. */
    for (size_t k1 = 0; k1 < 3; k1++) {
        const Cplx y0 = a[0][k1], y1 = a[1][k1], y2 = a[2][k1];
        const Cplx y3 = a[3][k1], y4 = a[4][k1];
        const float u1r = y1.re + y4.re, u1i = y1.im + y4.im;
        const float u2r = y2.re + y3.re, u2i = y2.im + y3.im;
        const float v1r = y1.re - y4.re, v1i = y1.im - y4.im;
        const float v2r = y2.re - y3.re, v2i = y2.im - y3.im;
        out[k1] = {y0.re + u1r + u2r, y0.im + u1i + u2i};
        const float p1r = y0.re + c1 * u1r + c2 * u2r;
        const float p1i = y0.im + c1 * u1i + c2 * u2i;
        const float q1r = s1 * v1r + s2 * v2r, q1i = s1 * v1i + s2 * v2i;
        out[k1 + 3] = {p1r - q1i, p1i + q1r};
        out[k1 + 12] = {p1r + q1i, p1i - q1r};
        const float p2r = y0.re + c2 * u1r + c1 * u2r;
        const float p2i = y0.im + c2 * u1i + c1 * u2i;
        const float q2r = s2 * v1r - s1 * v2r, q2i = s2 * v1i - s1 * v2i;
        out[k1 + 6] = {p2r - q2i, p2i + q2r};
        out[k1 + 9] = {p2r + q2i, p2i - q2r};
    }
}

void fft15x2(Cplx* x, size_t n, Cplx* scratch) {
    const size_t n2 = n / kRadix1;

    /* twiddle((c*k) % n, n) is kW480[(c*k*stride) % kFftMax] since
     * stride * n == kFftMax, so the index walks by c*stride. */
    const size_t stride = kFftMax / n;
    /* One fifteen-point DFT per column, twiddled on the way out. */
    for (size_t c = 0; c < n2; c++) {
        Cplx col[kRadix1];
        dft15_ct(&x[c], n2, col);
        /* c*stride < 32 cannot leave the table; the wrap guards a third size. */
        const size_t out_step = c * stride;
        size_t out_idx = 0;
        for (size_t k = 0; k < kRadix1; k++) {
            scratch[k * n2 + c] = cmul(col[k], kW480.w[out_idx]);
            out_idx += out_step;
            if (out_idx >= kFftMax)
                out_idx -= kFftMax;
        }
    }

    /* Power-of-two FFT per row, landing the output at X[k1 + 15*k2]. */
    for (size_t k1 = 0; k1 < kRadix1; k1++) {
        fft_pow2(&scratch[k1 * n2], n2);
        for (size_t k2 = 0; k2 < n2; k2++)
            x[k1 + kRadix1 * k2] = scratch[k1 * n2 + k2];
    }
}

/* N/4-point-FFT IMDCT, `half` coefficients into 2*half samples, `z` in place.
 * Iteration k reads z[k].re and z[n4-1-k].im, iteration n4-1-k the other
 * halves, so a pair reads exactly the four floats it writes (n4 is even).
 * Addressed through Cplx only; the one type pun is the caller's. */
void imdct(Cplx* z, size_t half, float* out, bool is_short) {
    /* `out` is the transform's scratch until the final reorder; it must not
     * overlap `z`. */
    Cplx* const scratch = reinterpret_cast<Cplx*>(out);
    const size_t n = half * 2;
    const size_t n2 = half;
    const size_t n4 = n / 4;
    const size_t n8 = n / 8;

    for (size_t k = 0; k < n4 / 2; k++) {
        const size_t j = n4 - 1 - k;
        /* All four read before either word is written back. */
        const float a_re = z[k].re, a_im = z[k].im;
        const float b_re = z[j].re, b_im = z[j].im;

        const Cplx wk = pre_rotation(k, is_short);
        const Cplx wj = pre_rotation(j, is_short);
        const float ck = wk.re, sk = wk.im;
        const float cj = wj.re, sj = wj.im;

        /* iteration k: x1 = in[2k] = a_re, x2 = in[n2-1-2k] = in[2j+1] = b_im
         * iteration j: x1 = in[2j] = b_re, x2 = in[n2-1-2j] = in[2k+1] = a_im */
        z[k].re = b_im * ck - a_re * sk;
        z[k].im = a_re * ck + b_im * sk;
        z[j].re = a_im * cj - b_re * sj;
        z[j].im = b_re * cj + a_im * sj;
    }

    fft15x2(z, n4, scratch);

    for (size_t k = 0; k < n4; k++) {
        const Cplx w = pre_rotation(k, is_short);
        const float c = w.re, s = w.im;
        const float re = z[k].re, im = z[k].im;
        z[k].im = im * c + re * s;
        z[k].re = re * c - im * s;
    }

    for (size_t k = 0; k < n8; k += 2) {
        out[2 * k] = z[n8 + k].im;
        out[2 + 2 * k] = z[n8 + 1 + k].im;
        out[1 + 2 * k] = -z[n8 - 1 - k].re;
        out[3 + 2 * k] = -z[n8 - 2 - k].re;

        out[n4 + 2 * k] = z[k].re;
        out[n4 + 2 + 2 * k] = z[1 + k].re;
        out[n4 + 1 + 2 * k] = -z[n4 - 1 - k].im;
        out[n4 + 3 + 2 * k] = -z[n4 - 2 - k].im;

        out[n2 + 2 * k] = z[n8 + k].re;
        out[n2 + 2 + 2 * k] = z[n8 + 1 + k].re;
        out[n2 + 1 + 2 * k] = -z[n8 - 1 - k].im;
        out[n2 + 3 + 2 * k] = -z[n8 - 2 - k].im;

        out[n2 + n4 + 2 * k] = -z[k].im;
        out[n2 + n4 + 2 + 2 * k] = -z[1 + k].im;
        out[n2 + n4 + 1 + 2 * k] = z[n4 - 1 - k].re;
        out[n2 + n4 + 3 + 2 * k] = z[n4 - 2 - k].re;
    }
}

/* Q15 windows in flash, rising half only (gen_aac_windows.cpp). Both shapes
 * stay resident: the rising edge takes the previous unit's shape, the falling
 * edge this one's, and shapes switch too often to generate on demand. */
constexpr float kQ15 = 1.0f / 32768.0f;

inline const int16_t* rise_window(uint8_t shape, bool is_short) {
    if (is_short)
        return shape ? aac::kKbdShort : aac::kSineShort;
    return shape ? aac::kKbdLong : aac::kSineLong;
}

}  // namespace

void AacLcDecoder::filterbank(const Ics& ics, int16_t* pcm) {
    const bool is_short = ics.window_sequence == kEightShort;
    const size_t half = kAacFrameLength;
    float* const time = scratch_; /* 2*kAacFrameLength floats */

    /* The IMDCT's 2/N uses each transform's own N (1920 long, 240 short); one
     * factor for the frame leaves short blocks eight times too small, a hole
     * wherever the encoder switches. */
    const float norm_long = kQ15 * 2.0f / float(2 * kAacFrameLength);
    const float norm_short = kQ15 * 2.0f / float(2 * kAacShortLength);

    /* A start or stop window replaces part of an edge with a flat run and a
     * short slope, so a long block can sit next to eight short ones. */
    const int16_t* rise = rise_window(prev_window_shape_, false);
    const int16_t* fall = rise_window(ics.window_shape, false);
    const int16_t* rise_s = rise_window(prev_window_shape_, true);
    const int16_t* fall_s = rise_window(ics.window_shape, true);
    const size_t flat = (kAacFrameLength - kAacShortLength) / 2;

    if (!is_short) {
        imdct(reinterpret_cast<Cplx*>(spec_), half, time, false);

        /* In place: the second half becomes the next frame's overlap. */
        for (size_t i = 0; i < half; i++) {
            float w;
            if (ics.window_sequence == kLongStop) {
                if (i < flat)
                    w = 0.0f;
                else if (i < flat + kAacShortLength)
                    w = float(rise_s[i - flat]);
                else
                    w = 32767.0f;
            } else {
                w = float(rise[i]);
            }
            time[i] *= w * norm_long;
        }
        for (size_t i = 0; i < half; i++) {
            float w;
            if (ics.window_sequence == kLongStart) {
                if (i < flat)
                    w = 32767.0f;
                else if (i < flat + kAacShortLength)
                    w = float(fall_s[kAacShortLength - 1 - (i - flat)]);
                else
                    w = 0.0f;
            } else {
                w = float(fall[half - 1 - i]);
            }
            time[half + i] *= w * norm_long;
        }
    } else {
        /* Eight 120 -> 240 transforms at 120-sample steps from 420. Each lands
         * in the last 240 floats of `time`, beyond the accumulation (420 +
         * 7*120 + 239 = 1499 of 1920), saving 960 bytes; cleared after, since
         * the overlap-add reads all of `time`. */
        static_assert(2 * kAacFrameLength - 2 * kAacShortLength >
                          (kAacFrameLength - kAacShortLength) / 2 + 9 * kAacShortLength,
                      "the block buffer must sit past everything the accumulation writes");
        float* const block = &time[2 * kAacFrameLength - 2 * kAacShortLength];
        std::memset(time, 0, 2 * kAacFrameLength * sizeof(float));
        for (size_t w = 0; w < 8; w++) {
            imdct(reinterpret_cast<Cplx*>(&spec_[w * kAacShortLength]),
                  kAacShortLength, block, true);
            const int16_t* r = (w == 0) ? rise_s : fall_s;
            const size_t base = flat + w * kAacShortLength;
            for (size_t i = 0; i < kAacShortLength; i++) {
                time[base + i] += block[i] * float(r[i]) * norm_short;
                time[base + kAacShortLength + i] +=
                    block[kAacShortLength + i] * float(fall_s[kAacShortLength - 1 - i]) * norm_short;
            }
        }
        std::memset(block, 0, 2 * kAacShortLength * sizeof(float));
    }

    /* Each overlap sample is used and replaced before the next; coefficients
     * are already in output units. */
    for (size_t i = 0; i < half; i++) {
        float v = overlap_[i] + time[i];
        overlap_[i] = time[half + i];
        if (v > 32767.0f)
            v = 32767.0f;
        else if (v < -32768.0f)
            v = -32768.0f;
        else if (v != v)
            v = 0.0f; /* a CRC-valid but hostile unit can drive TNS past float range */
        pcm[i] = int16_t(v);
    }

    prev_window_shape_ = ics.window_shape;
}

/* ics_info is read here unless a CPE's common window supplied it. */
bool AacLcDecoder::decode_channel_stream(BitReader& br, Ics& ics, bool read_info) {
    const uint8_t global_gain = uint8_t(br.read(8));
    if (read_info && !decode_ics_info(br, ics))
        return false;
    if (!decode_section_data(br, ics))
        return false;
    if (!decode_scale_factors(br, ics, global_gain))
        return false;
    pulse_present_ = uint8_t(br.read(1));
    if (pulse_present_ && !decode_pulse_data(br, ics))
        return false;
    ics.tns_present = uint8_t(br.read(1));
    std::memset(ics.tns_n_filt, 0, sizeof(ics.tns_n_filt));
    if (ics.tns_present && !decode_tns_data(br, ics))
        return false;
    if (br.read(1) != 0) {  // gain_control_data_present
        error_ = kErrGainControl;
        return false;
    }
    return decode_spectral_data(br, ics);
}

size_t AacLcDecoder::decode(const uint8_t* au, size_t length, int16_t* pcm, bool* ok) {
    if (ok != nullptr)
        *ok = false;
    dse_bytes_ = 0;
    error_ = kErrNone;
    if (!configured_ || au == nullptr || length == 0 || pcm == nullptr)
        return 0;

    BitReader br{au, length, 0};
    Ics& ics = ics_;
    ics = Ics{};
    bool got_channel = false;

    for (;;) {
        const uint8_t id = uint8_t(br.read(3));
        if (br.overrun()) {
            error_ = kErrOverrun;
            return 0;
        }
        if (id == kIdEnd)
            break;

        if (id == kIdSce) {
            if (got_channel) {
                error_ = kErrElement;
                return 0;
            }
            br.skip(4);  // element_instance_tag
            if (!decode_channel_stream(br, ics, true))
                return 0;
            got_channel = true;
            continue;
        }

        if (id == kIdCpe) {
            /* Played as its first channel: M = (L + R) / 2 in an M/S band, L
             * otherwise. Intensity bands live in channel 1, which is never
             * needed, so parsing stops before it. */
            if (got_channel) {
                error_ = kErrElement;
                return 0;
            }
            br.skip(4);  // element_instance_tag
            const bool common_window = br.read(1) != 0;
            if (common_window) {
                if (!decode_ics_info(br, ics))
                    return 0;
                const uint32_t ms_mask_present = br.read(2);
                if (ms_mask_present == 3) {
                    error_ = kErrElement;
                    return 0;
                }
                if (ms_mask_present == 1)
                    br.bit += size_t(ics.num_window_groups) * ics.max_sfb;
            }
            if (!decode_channel_stream(br, ics, !common_window))
                return 0;
            got_channel = true;
            break;
        }

        if (id == kIdFil) {
            uint32_t count = br.read(4);
            if (count == 15)
                count += br.read(8) - 1;
            br.bit += size_t(count) * 8;
            if (br.overrun()) {
                error_ = kErrOverrun;
                return 0;
            }
            continue;
        }

        if (id == kIdDse) {
            br.skip(4);  // element_instance_tag
            const uint32_t align = br.read(1);
            uint32_t count = br.read(8);
            if (count == 255)
                count += br.read(8);
            if (align)
                br.bit = (br.bit + 7) & ~size_t(7);
            dse_bit_ = br.bit;
            dse_bytes_ = static_cast<uint16_t>(count);
            br.bit += size_t(count) * 8;
            if (br.overrun()) {
                error_ = kErrOverrun;
                return 0;
            }
            continue;
        }

        /* CCE, LFE and PCE do not occur in DAB+; skipping one blind would
         * desynchronise the rest of the unit. */
        error_ = kErrElement;
        return 0;
    }

    if (!got_channel) {
        error_ = kErrElement;
        return 0;
    }

    last_window_sequence_ = ics.window_sequence;
    last_window_shape_ = ics.window_shape;
    last_had_tns_ = ics.tns_present != 0;

    apply_pulses(ics);
    if (ics.tns_present)
        apply_tns(ics);
    filterbank(ics, pcm);

    if (ok != nullptr)
        *ok = true;
    if (!primed_) {
        primed_ = true;
        return 0;
    }
    return kAacFrameLength;
}

}  // namespace dab
