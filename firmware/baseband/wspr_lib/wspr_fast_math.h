/*
 * Copyright (C) 2026 Ivan
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

#ifndef _WSPR_FAST_MATH_H_
#define _WSPR_FAST_MATH_H_

/*
 * WSPR-local libm-free helpers for Goertzel and the decoder.
 * The cosine and mantissa approximations also exist in the FT8 app.
 * Every routine is a static inline function with no global state, so each
 * translation unit gets its own copy and nothing is exported. C and C++ safe.
 *
 * These are EXTRACTED VERBATIM from their original sites: the constants and
 * the evaluation order are preserved bit-for-bit, which the host test suite
 * pins with exact-output checks. Do NOT substitute common/utility's fast_log2():
 * that is a different (second-order mantissa) polynomial, and using it here
 * would change the quantized byte scale and the decoder's SNR scores - an
 * algorithm change, not a refactor.
 */

#include <stdint.h>

/*
 * Cosine over [0, pi/2], the whole range the Goertzel coefficient tables need.
 * cosf() would drag in newlib's argument reduction (__kernel_rem_pio2f plus
 * __ieee754_rem_pio2f, 2,304 bytes of the 32 KB app budget) for a routine used
 * once at startup. The truncation error stays within 5e-10 across the WSPR coefficient sweep.
 */
static inline float cos_unit_quadrant(float x) {
    const float x2 = x * x;
    return 1.0f + x2 * (-1.0f / 2.0f + x2 * (1.0f / 24.0f + x2 * (-1.0f / 720.0f + x2 * (1.0f / 40320.0f + x2 * (-1.0f / 3628800.0f + x2 * (1.0f / 479001600.0f))))));
}

/*
 * log2 from the exponent plus a series on the mantissa. Calling log10f costs
 * ~146 instructions per bin and drags newlib's log tables into the external
 * image; WSPR quantizes 137 bins inside the row-closing chunk. The Goertzel output
 * uses a monotonic affine map of dB.
 */
static inline float log2_from_bits(float x) {
    union {
        float f;
        uint32_t i;
    } u;
    u.f = x;
    int e = (int)((u.i >> 23) & 0xFFu) - 127;
    u.i = (u.i & 0x007FFFFFu) | 0x3F800000u;
    float m = u.f;
    // Centring the mantissa on 1 keeps |t| <= 0.1716, where the first dropped term of the
    // series is 7e-7, against a quantisation step of 0.166 in log2 units.
    if (m > 1.4142136f) {
        m *= 0.5f;
        e++;
    }
    const float t = (m - 1.0f) / (m + 1.0f);
    const float t2 = t * t;
    return (float)e + t * (2.8853900f + t2 * (0.9617967f + t2 * 0.5770780f));
}

/*
 * The decoder's power_log2(): the SAME exponent/mantissa series as
 * log2_from_bits, so it shares that one implementation instead of carrying a
 * second copy. The original form returned `exponent + <float expression>`;
 * the implicit int->float conversion there is exact for every exponent the
 * decoder feeds it (byte-scale ratios, all tiny), so the result is bit-identical.
 */
static inline float power_log2(float value) {
    return log2_from_bits(value);
}

#endif  // _WSPR_FAST_MATH_H_
