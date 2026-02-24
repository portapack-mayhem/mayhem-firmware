/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "utility.hpp"

#include <cstdint>

#if 0
uint32_t gcd(const uint32_t u, const uint32_t v) {
	/* From http://en.wikipedia.org/wiki/Binary_GCD_algorithm */

	if( u == v ) {
		return u;
	}

	if( u == 0 ) {
		return v;
	}

	if( v == 0 ) {
		return u;
	}

	if( ~u & 1 ) {
		if( v & 1 ) {
			return gcd(u >> 1, v);
		} else {
			return gcd(u >> 1, v >> 1) << 1;
		}
	}

	if( ~v & 1 ) {
		return gcd(u, v >> 1);
	}

	if( u > v ) {
		return gcd((u - v) >> 1, v);
	}

	return gcd((v - u) >> 1, u);
}
#endif

float fast_log2(const float val) {
    // Thank you Stack Overflow!
    // http://stackoverflow.com/questions/9411823/fast-log2float-x-implementation-c
    union {
        float val;
        int32_t x;
    } u = {val};
    float log_2 = (((u.x >> 23) & 255) - 128);
    u.x &= ~(255 << 23);
    u.x += (127 << 23);
    log_2 += ((-0.34484843f) * u.val + 2.02466578f) * u.val - 0.67487759f;
    return log_2;
}

float fast_pow2(const float val) {
    union {
        float f;
        uint32_t n;
    } u;
    u.n = val * 8388608 + (0x3f800000 - 60801 * 8);
    return u.f;
}

float mag2_to_dbv_norm(const float mag2) {
    constexpr float mag2_log2_max = 0.0f;  // std::log2(1.0f);
    constexpr float log_mag2_mag_factor = 0.5f;
    constexpr float log2_log10_factor = 0.3010299956639812f;  // std::log10(2.0f);
    constexpr float log10_dbv_factor = 20.0f;
    constexpr float mag2_to_db_factor = log_mag2_mag_factor * log2_log10_factor * log10_dbv_factor;
    return (fast_log2(mag2) - mag2_log2_max) * mag2_to_db_factor;
}

// Integer in and out approximation
// >40 times faster float sqrt(x*x+y*y) on Cortex M0
// derived from https://dspguru.com/dsp/tricks/magnitude-estimator/
int fast_int_magnitude(int y, int x) {
    if (y < 0) {
        y = -y;
    }
    if (x < 0) {
        x = -x;
    }
    if (x > y) {
        return ((x * 61) + (y * 26) + 32) / 64;
    } else {
        return ((y * 61) + (x * 26) + 32) / 64;
    }
}

// Integer x and y returning an integer bearing in degrees
// Accurate to 0.5 degrees, so output scaled to whole degrees
// >60 times faster than float atan2 on Cortex M0
int int_atan2(int y, int x) {
    // Number of bits to shift up before doing the maths.  A larger shift
    // may beable to gain accuracy, but it would cause the correction
    // entries to be larger than 1 byte
    static const int bits = 10;
    static const int pi4 = (1 << bits);
    static const int pi34 = (3 << bits);

    // Special case
    if (x == 0 && y == 0) {
        return 0;
    }

    // Form an approximate angle
    const int yabs = y >= 0 ? y : -y;
    int angle;
    if (x >= 0) {
        angle = pi4 - pi4 * (x - yabs) / (x + yabs);
    } else {
        angle = pi34 - pi4 * (x + yabs) / (yabs - x);
    }
    // Correct the result using a lookup table
    static const int8_t correct[32] = {0, -23, -42, -59, -72, -83, -89, -92, -92, -88, -81, -71, -58, -43, -27, -9, 9, 27, 43, 58, 71, 81, 88, 92, 92, 89, 83, 72, 59, 42, 23, 0};
    static const int rnd = (1 << (bits - 1)) / 45;  // Minor correction to round to correction values better (add 0.5)
    const int idx = ((angle + rnd) >> (bits - 4)) & 0x1F;
    angle += correct[idx];

    // Scale for output in degrees
    static const int half = (1 << (bits - 1));
    angle = ((angle * 45) + half) >> bits;  // Add on half before rounding
    if (y < 0) {
        angle = -angle;
    }
    return angle;
}

// 16 bit value represents a full cycle in but can handle multiples of this.
// Output in range +/- 16 bit value representing +/- 1.0
// 4th order cosine approximation has very small error
// >200 times faster tan float sin on Cortex M0
// see https://www.coranac.com/2009/07/sines/
int32_t int16_sin_s4(int32_t x) {
    static const int qN = 14, qA = 16, qR = 12, B = 19900, C = 3516;

    const int32_t c = x << (30 - qN);  // Semi-circle info into carry.
    x -= 1 << qN;                      // sine -> cosine calc

    x = x << (31 - qN);          // Mask with PI
    x = x >> (31 - qN);          // Note: SIGNED shift! (to qN)
    x = x * x >> (2 * qN - 14);  // x=x^2 To Q14

    int32_t y = B - (x * C >> 14);  // B - x^2*C
    y = (1 << qA) - (x * y >> qR);  // A - x^2*(B-x^2*C)

    return c >= 0 ? y : -y;
}

/* GCD implementation derived from recursive implementation at
 * http://en.wikipedia.org/wiki/Binary_GCD_algorithm
 */

static constexpr uint32_t gcd_top(const uint32_t u, const uint32_t v);

static constexpr uint32_t gcd_larger(const uint32_t u, const uint32_t v) {
    return (u > v) ? gcd_top((u - v) >> 1, v) : gcd_top((v - u) >> 1, u);
}

static constexpr uint32_t gcd_u_odd_v_even(const uint32_t u, const uint32_t v) {
    return (~v & 1) ? gcd_top(u, v >> 1) : gcd_larger(u, v);
}

static constexpr uint32_t gcd_v_odd(const uint32_t u, const uint32_t v) {
    return (v & 1)
               ? gcd_top(u >> 1, v)
               : (gcd_top(u >> 1, v >> 1) << 1);
}

static constexpr uint32_t gcd_u_even(const uint32_t u, const uint32_t v) {
    return (~u & 1)
               ? gcd_v_odd(u, v)
               : gcd_u_odd_v_even(u, v);
}

static constexpr uint32_t gcd_v_zero(const uint32_t u, const uint32_t v) {
    return (v == 0) ? u : gcd_u_even(u, v);
}

static constexpr uint32_t gcd_u_zero(const uint32_t u, const uint32_t v) {
    return (u == 0) ? v : gcd_v_zero(u, v);
}

static constexpr uint32_t gcd_uv_equal(const uint32_t u, const uint32_t v) {
    return (u == v) ? u : gcd_u_zero(u, v);
}

static constexpr uint32_t gcd_top(const uint32_t u, const uint32_t v) {
    return gcd_uv_equal(u, v);
}

uint32_t gcd(const uint32_t u, const uint32_t v) {
    return gcd_top(u, v);
}

std::string join(char c, std::initializer_list<std::string_view> strings) {
    std::string result;
    size_t total_size = strings.size();

    for (auto s : strings)
        total_size += s.size();

    result.reserve(total_size);
    bool first = true;

    for (auto s : strings) {
        if (!first)
            result += c;
        else
            first = false;

        result += s;
    }

    return result;
}

uint32_t simple_checksum(uint32_t buffer_address, uint32_t length) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < length; i += 4)
        checksum += *(uint32_t*)(buffer_address + i);
    return checksum;
}