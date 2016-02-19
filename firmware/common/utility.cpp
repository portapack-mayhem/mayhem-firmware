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
	} u = { val };
	float log_2 = (((u.x >> 23) & 255) - 128);
	u.x &= ~(255 << 23);
	u.x +=  (127 << 23);
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
	constexpr float mag2_log2_max = 0.0f; //std::log2(1.0f);
	constexpr float log_mag2_mag_factor = 0.5f;
	constexpr float log2_log10_factor = 0.3010299956639812f; //std::log10(2.0f);
	constexpr float log10_dbv_factor = 20.0f;
	constexpr float mag2_to_db_factor = log_mag2_mag_factor * log2_log10_factor * log10_dbv_factor;
	return (fast_log2(mag2) - mag2_log2_max) * mag2_to_db_factor;
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
		: gcd_u_odd_v_even(u, v)
		;
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
