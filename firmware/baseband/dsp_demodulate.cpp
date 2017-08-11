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
#include "utility_m4.hpp"

#include <hal.h>

namespace dsp {
namespace demodulate {

buffer_f32_t AM::execute(
	const buffer_c16_t& src,
	const buffer_f32_t& dst
) {
	const void* src_p = src.p;
	const auto src_end = &src.p[src.count];
	auto dst_p = dst.p;
	while(src_p < src_end) {
		const uint32_t sample0 = *__SIMD32(src_p)++;
		const uint32_t sample1 = *__SIMD32(src_p)++;
		const uint32_t mag_sq0 = __SMUAD(sample0, sample0);
		const uint32_t mag_sq1 = __SMUAD(sample1, sample1);
		*(dst_p++) = __builtin_sqrtf(mag_sq0) * k;
		*(dst_p++) = __builtin_sqrtf(mag_sq1) * k;
	}

	return { dst.p, src.count, src.sampling_rate };
}

buffer_f32_t SSB::execute(
	const buffer_c16_t& src,
	const buffer_f32_t& dst
) {
	const complex16_t* src_p = src.p;
	const auto src_end = &src.p[src.count];
	auto dst_p = dst.p;
	while(src_p < src_end) {
		*(dst_p++) = (src_p++)->real() * k;
		*(dst_p++) = (src_p++)->real() * k;
		*(dst_p++) = (src_p++)->real() * k;
		*(dst_p++) = (src_p++)->real() * k;
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
	if( t.real() ) {
		const auto x = static_cast<float>(t.imag()) / static_cast<float>(t.real());
		return x / (1.0f + 0.28086f * x * x);
	} else {
		return (t.imag() < 0) ? -1.5707963268f : 1.5707963268f;
	}
}

static inline float angle_precise(const complex32_t t) {
	return atan2f(t.imag(), t.real());
}

buffer_f32_t FM::execute(
	const buffer_c16_t& src,
	const buffer_f32_t& dst
) {
	auto z = z_;

	const void* src_p = src.p;
	const auto src_end = &src.p[src.count];
	auto dst_p = dst.p;
	while(src_p < src_end) {
		const auto s0 = *__SIMD32(src_p)++;
		const auto s1 = *__SIMD32(src_p)++;
		const auto t0 = multiply_conjugate_s16_s32(s0, z);
		const auto t1 = multiply_conjugate_s16_s32(s1, s0);
		z = s1;
		*(dst_p++) = angle_precise(t0) * kf;
		*(dst_p++) = angle_precise(t1) * kf;
	}
	z_ = z;

	return { dst.p, src.count, src.sampling_rate };
}

buffer_s16_t FM::execute(
	const buffer_c16_t& src,
	const buffer_s16_t& dst
) {
	auto z = z_;

	const void* src_p = src.p;
	const auto src_end = &src.p[src.count];
	void* dst_p = dst.p;
	while(src_p < src_end) {
		const auto s0 = *__SIMD32(src_p)++;
		const auto s1 = *__SIMD32(src_p)++;
		const auto t0 = multiply_conjugate_s16_s32(s0, z);
		const auto t1 = multiply_conjugate_s16_s32(s1, s0);
		z = s1;
		const int32_t theta0_int = angle_approx_0deg27(t0) * ks16;
		const int32_t theta0_sat = __SSAT(theta0_int, 16);
		const int32_t theta1_int = angle_approx_0deg27(t1) * ks16;
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

void FM::configure(const float sampling_rate, const float deviation_hz) {
	/*
	 * angle: -pi to pi. output range: -32768 to 32767.
	 * Maximum delta-theta (output of atan2) at maximum deviation frequency:
	 * delta_theta_max = 2 * pi * deviation / sampling_rate
	 */
	kf = static_cast<float>(1.0f / (2.0 * pi * deviation_hz / sampling_rate));
	ks16 = 32767.0f * kf;
}

}
}
