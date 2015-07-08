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

#include "dsp_demodulate.hpp"

#include "complex.hpp"
#include "fxpt_atan2.hpp"

#include <hal.h>

namespace dsp {
namespace demodulate {

buffer_s16_t AM::execute(
	buffer_c16_t src,
	buffer_s16_t dst
) {
	/* Intermediate maximum value: 46341 (when input is -32768,-32768). */
	/* Normalized to maximum 32767 for int16_t representation. */

	const auto src_p = src.p;
	const auto src_end = &src.p[src.count];
	auto dst_p = dst.p;
	while(src_p < src_end) {
		// const auto s = *(src_p++);
		// const uint32_t r_sq = s.real() * s.real();
		// const uint32_t i_sq = s.imag() * s.imag();
		// const uint32_t mag_sq = r_sq + i_sq;
		const uint32_t sample0 = *__SIMD32(src_p)++;
		const uint32_t sample1 = *__SIMD32(src_p)++;
		const uint32_t mag_sq0 = __SMUAD(sample0, sample0);
		const uint32_t mag_sq1 = __SMUAD(sample1, sample1);
		const int32_t mag0_int = __builtin_sqrtf(mag_sq0);
		const int32_t mag0_sat = __SSAT(mag0_int, 16);
		const int32_t mag1_int = __builtin_sqrtf(mag_sq1);
		const int32_t mag1_sat = __SSAT(mag1_int, 16);
		*__SIMD32(dst_p)++ = __PKHBT(
			mag0_sat,
			mag1_sat,
			16
		);
	}

	return { dst.p, src.count, src.sampling_rate };
}
/*
static inline float angle_approx_4deg0(const complex32_t t) {
	const auto x = static_cast<float>(t.imag()) / static_cast<float>(t.real());
	return 16384.0f * x;
}
*/
static inline float angle_approx_0deg27(const complex32_t t) {
	const auto x = static_cast<float>(t.imag()) / static_cast<float>(t.real());
	return x / (1.0f + 0.28086f * x * x);
}
/*
static inline float angle_precise(const complex32_t t) {
	return atan2f(t.imag(), t.real());
}
*/
buffer_s16_t FM::execute(
	buffer_c16_t src,
	buffer_s16_t dst
) {
	auto z = z_;

	const auto src_p = src.p;
	const auto src_end = &src.p[src.count];
	auto dst_p = dst.p;
	while(src_p < src_end) {
		const auto s0 = *__SIMD32(src_p)++;
		const auto s1 = *__SIMD32(src_p)++;
		const auto t0 = multiply_conjugate_s16_s32(s0, z);
		const auto t1 = multiply_conjugate_s16_s32(s1, s0);
		z = s1;
		const int32_t theta0_int = angle_approx_0deg27(t0) * k;
		const int32_t theta0_sat = __SSAT(theta0_int, 16);
		const int32_t theta1_int = angle_approx_0deg27(t1) * k;
		const int32_t theta1_sat = __SSAT(theta1_int, 16);
		*__SIMD32(dst_p)++ = __PKHBT(
			theta0_sat,
			theta1_sat,
			16
		);
	}
	z_ = z;

	return { dst.p, src.count, src.sampling_rate };
}

}
}
