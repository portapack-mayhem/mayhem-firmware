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

#include "dsp_decimate.hpp"

#include <hal.h>

namespace dsp {
namespace decimate {

static inline complex32_t mac_fs4_shift(
	const vec2_s16* const z,
	const vec2_s16* const t,
	const size_t index,
	const complex32_t accum
) {
	/* Accumulate sample * tap results for samples already in z buffer.
	 * Multiply using swap/negation to achieve Fs/4 shift.
	 * For iterations where samples are shifting out of z buffer (being discarded).
	 * Expect negated tap t[2] to accomodate instruction set limitations.
	 */
	const bool negated_t2 = index & 1;
	const auto q1_i0 = z[index*2 + 0];
	const auto i1_q0 = z[index*2 + 1];
	const auto t1_t0 = t[index];
	const auto real = negated_t2 ? smlsd(q1_i0, t1_t0, accum.real()) : smlad(q1_i0, t1_t0, accum.real());
	const auto imag = negated_t2 ? smlad(i1_q0, t1_t0, accum.imag()) : smlsd(i1_q0, t1_t0, accum.imag());
	return { real, imag };
}

static inline complex32_t mac_shift(
	const vec2_s16* const z,
	const vec2_s16* const t,
	const size_t index,
	const complex32_t accum
) {
	/* Accumulate sample * tap results for samples already in z buffer.
	 * For iterations where samples are shifting out of z buffer (being discarded).
	 * real += i1 * t1 + i0 * t0
	 * imag += q1 * t1 + q0 * t0
	 */
	const auto i1_i0 = z[index*2 + 0];
	const auto q1_q0 = z[index*2 + 1];
	const auto t1_t0 = t[index];
	const auto real = smlad(i1_i0, t1_t0, accum.real());
	const auto imag = smlad(q1_q0, t1_t0, accum.imag());
	return { real, imag };
}

static inline complex32_t mac_fs4_shift_and_store(
	vec2_s16* const z,
	const vec2_s16* const t,
	const size_t decimation_factor,
	const size_t index,
	const complex32_t accum
) {
	/* Accumulate sample * tap results for samples already in z buffer.
	 * Place new samples into z buffer.
	 * Expect negated tap t[2] to accomodate instruction set limitations.
	 */
	const bool negated_t2 = index & 1;
	const auto q1_i0 = z[decimation_factor + index*2 + 0];
	const auto i1_q0 = z[decimation_factor + index*2 + 1];
	const auto t1_t0 = t[decimation_factor / 2 + index];
	z[index*2 + 0] = q1_i0;
	const auto real = negated_t2 ? smlsd(q1_i0, t1_t0, accum.real()) : smlad(q1_i0, t1_t0, accum.real());
	z[index*2 + 1] = i1_q0;
	const auto imag = negated_t2 ? smlad(i1_q0, t1_t0, accum.imag()) : smlsd(i1_q0, t1_t0, accum.imag());
	return { real, imag };
}

static inline complex32_t mac_shift_and_store(
	vec2_s16* const z,
	const vec2_s16* const t,
	const size_t decimation_factor,
	const size_t index,
	const complex32_t accum
) {
	/* Accumulate sample * tap results for samples already in z buffer.
	 * Place new samples into z buffer.
	 * Expect negated tap t[2] to accomodate instruction set limitations.
	 */
	const auto i1_i0 = z[decimation_factor + index*2 + 0];
	const auto q1_q0 = z[decimation_factor + index*2 + 1];
	const auto t1_t0 = t[decimation_factor / 2 + index];
	z[index*2 + 0] = i1_i0;
	const auto real = smlad(i1_i0, t1_t0, accum.real());
	z[index*2 + 1] = q1_q0;
	const auto imag = smlad(q1_q0, t1_t0, accum.imag());
	return { real, imag };
}

static inline complex32_t mac_fs4_shift_and_store_new_c8_samples(
	vec2_s16* const z,
	const vec2_s16* const t,
	const vec4_s8* const in,
	const size_t decimation_factor,
	const size_t index,
	const size_t length,
	const complex32_t accum
) {
	/* Accumulate sample * tap results for new samples.
	 * Place new samples into z buffer.
	 * Expect negated tap t[2] to accomodate instruction set limitations.
	 */
	const bool negated_t2 = index & 1;
	const auto q1_i1_q0_i0 = in[index];
	const auto t1_t0 = t[(length - decimation_factor) / 2 + index];
	const auto i1_q1_i0_q0 = rev16(q1_i1_q0_i0);
	const auto i1_q1_q0_i0 = pkhbt(q1_i1_q0_i0, i1_q1_i0_q0);
	const auto q1_i0 = sxtb16(i1_q1_q0_i0);
	const auto i1_q0 = sxtb16(i1_q1_q0_i0, 8);
	z[length - decimation_factor * 2 + index*2 + 0] = q1_i0;
	const auto real = negated_t2 ? smlsd(q1_i0, t1_t0, accum.real()) : smlad(q1_i0, t1_t0, accum.real());
	z[length - decimation_factor * 2 + index*2 + 1] = i1_q0;
	const auto imag = negated_t2 ? smlad(i1_q0, t1_t0, accum.imag()) : smlsd(i1_q0, t1_t0, accum.imag());
	return { real, imag };
}

static inline complex32_t mac_shift_and_store_new_c16_samples(
	vec2_s16* const z,
	const vec2_s16* const t,
	const vec2_s16* const in,
	const size_t decimation_factor,
	const size_t index,
	const size_t length,
	const complex32_t accum
) {
	/* Accumulate sample * tap results for new samples.
	 * Place new samples into z buffer.
	 * Expect negated tap t[2] to accomodate instruction set limitations.
	 */
	const auto q0_i0 = in[index*2+0];
	const auto q1_i1 = in[index*2+1];
	const auto i1_i0 = pkhbt(q0_i0, q1_i1, 16);
	const auto q1_q0 = pkhtb(q1_i1, q0_i0, 16);
	const auto t1_t0 = t[(length - decimation_factor) / 2 + index];
	z[length - decimation_factor * 2 + index*2 + 0] = i1_i0;
	const auto real = smlad(i1_i0, t1_t0, accum.real());
	z[length - decimation_factor * 2 + index*2 + 1] = q1_q0;
	const auto imag = smlad(q1_q0, t1_t0, accum.imag());
	return { real, imag };
}

static inline uint32_t scale_round_and_pack(
	const complex32_t value,
	const int32_t scale_factor
) {
	/* Multiply 32-bit components of the complex<int32_t> by a scale factor,
	 * into int64_ts, then round to nearest LSB (1 << 32), saturate to 16 bits,
	 * and pack into a complex<int16_t>.
	 */
	const auto scaled_real = __SMMULR(value.real(), scale_factor);
	const auto saturated_real = __SSAT(scaled_real, 16);

	const auto scaled_imag = __SMMULR(value.imag(), scale_factor);
	const auto saturated_imag = __SSAT(scaled_imag, 16);

	return __PKHBT(saturated_real, saturated_imag, 16);
}

template<typename Tap>
static void taps_copy(
	const Tap* const source,
	Tap* const target,
	const size_t count,
	const bool shift_up
) {
	const uint32_t negate_pattern = shift_up ? 0b1110 : 0b0100;
	for(size_t i=0; i<count; i++) {
		const bool negate = (negate_pattern >> (i & 3)) & 1;
		target[i] = negate ? -source[i] : source[i];
	}
}

// FIRC8xR16x24FS4Decim4 //////////////////////////////////////////////////

void FIRC8xR16x24FS4Decim4::configure(
	const std::array<tap_t, taps_count>& taps,
	const int32_t scale,
	const Shift shift
) {
	taps_copy(taps.data(), taps_.data(), taps_.size(), shift == Shift::Up);
	output_scale = scale;
	z_.fill({});
}

buffer_c16_t FIRC8xR16x24FS4Decim4::execute(
	const buffer_c8_t& src,
	const buffer_c16_t& dst
) {
	vec2_s16* const z = static_cast<vec2_s16*>(__builtin_assume_aligned(z_.data(), 4));
	const vec2_s16* const t = static_cast<vec2_s16*>(__builtin_assume_aligned(taps_.data(), 4));
	uint32_t* const d = static_cast<uint32_t*>(__builtin_assume_aligned(dst.p, 4));

	const auto k = output_scale;

	const size_t count = src.count / decimation_factor;
	for(size_t i=0; i<count; i++) {
		const vec4_s8* const in = static_cast<const vec4_s8*>(__builtin_assume_aligned(&src.p[i * decimation_factor], 4));

		complex32_t accum;

		// Oldest samples are discarded.
		accum = mac_fs4_shift(z, t, 0, accum);
		accum = mac_fs4_shift(z, t, 1, accum);

		// Middle samples are shifted earlier in the "z" delay buffer.
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 0, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 1, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 2, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 3, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 4, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 5, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 6, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 7, accum);

		// Newest samples come from "in" buffer, are copied to "z" delay buffer.
		accum = mac_fs4_shift_and_store_new_c8_samples(z, t, in, decimation_factor, 0, taps_count, accum);
		accum = mac_fs4_shift_and_store_new_c8_samples(z, t, in, decimation_factor, 1, taps_count, accum);

		d[i] = scale_round_and_pack(accum, k);
	}

	return {
		dst.p,
		count,
		src.sampling_rate / decimation_factor
	};
}

// FIRC8xR16x24FS4Decim8 //////////////////////////////////////////////////

void FIRC8xR16x24FS4Decim8::configure(
	const std::array<tap_t, taps_count>& taps,
	const int32_t scale,
	const Shift shift
) {
	taps_copy(taps.data(), taps_.data(), taps_.size(), shift == Shift::Up);
	output_scale = scale;
	z_.fill({});
}

buffer_c16_t FIRC8xR16x24FS4Decim8::execute(
	const buffer_c8_t& src,
	const buffer_c16_t& dst
) {
	vec2_s16* const z = static_cast<vec2_s16*>(__builtin_assume_aligned(z_.data(), 4));
	const vec2_s16* const t = static_cast<vec2_s16*>(__builtin_assume_aligned(taps_.data(), 4));
	uint32_t* const d = static_cast<uint32_t*>(__builtin_assume_aligned(dst.p, 4));

	const auto k = output_scale;

	const size_t count = src.count / decimation_factor;
	for(size_t i=0; i<count; i++) {
		const vec4_s8* const in = static_cast<const vec4_s8*>(__builtin_assume_aligned(&src.p[i * decimation_factor], 4));

		complex32_t accum;

		// Oldest samples are discarded.
		accum = mac_fs4_shift(z, t, 0, accum);
		accum = mac_fs4_shift(z, t, 1, accum);
		accum = mac_fs4_shift(z, t, 2, accum);
		accum = mac_fs4_shift(z, t, 3, accum);

		// Middle samples are shifted earlier in the "z" delay buffer.
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 0, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 1, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 2, accum);
		accum = mac_fs4_shift_and_store(z, t, decimation_factor, 3, accum);

		// Newest samples come from "in" buffer, are copied to "z" delay buffer.
		accum = mac_fs4_shift_and_store_new_c8_samples(z, t, in, decimation_factor, 0, taps_count, accum);
		accum = mac_fs4_shift_and_store_new_c8_samples(z, t, in, decimation_factor, 1, taps_count, accum);
		accum = mac_fs4_shift_and_store_new_c8_samples(z, t, in, decimation_factor, 2, taps_count, accum);
		accum = mac_fs4_shift_and_store_new_c8_samples(z, t, in, decimation_factor, 3, taps_count, accum);

		d[i] = scale_round_and_pack(accum, k);
	}

	return {
		dst.p,
		count,
		src.sampling_rate / decimation_factor
	};
}

// FIRC16xR16x16Decim2 ////////////////////////////////////////////////////

void FIRC16xR16x16Decim2::configure(
	const std::array<tap_t, taps_count>& taps,
	const int32_t scale
) {
	std::copy(taps.cbegin(), taps.cend(), taps_.begin());
	output_scale = scale;
	z_.fill({});
}

buffer_c16_t FIRC16xR16x16Decim2::execute(
	const buffer_c16_t& src,
	const buffer_c16_t& dst
) {
	vec2_s16* const z = static_cast<vec2_s16*>(__builtin_assume_aligned(z_.data(), 4));
	const vec2_s16* const t = static_cast<vec2_s16*>(__builtin_assume_aligned(taps_.data(), 4));
	uint32_t* const d = static_cast<uint32_t*>(__builtin_assume_aligned(dst.p, 4));

	const auto k = output_scale;

	const size_t count = src.count / decimation_factor;
	for(size_t i=0; i<count; i++) {
		const vec2_s16* const in = static_cast<const vec2_s16*>(__builtin_assume_aligned(&src.p[i * decimation_factor], 4));

		complex32_t accum;

		// Oldest samples are discarded.
		accum = mac_shift(z, t, 0, accum);

		// Middle samples are shifted earlier in the "z" delay buffer.
		accum = mac_shift_and_store(z, t, decimation_factor, 0, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 1, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 2, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 3, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 4, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 5, accum);

		// Newest samples come from "in" buffer, are copied to "z" delay buffer.
		accum = mac_shift_and_store_new_c16_samples(z, t, in, decimation_factor, 0, taps_count, accum);

		d[i] = scale_round_and_pack(accum, k);
	}

	return {
		dst.p,
		count,
		src.sampling_rate / decimation_factor
	};
}

// FIRC16xR16x32Decim8 ////////////////////////////////////////////////////

void FIRC16xR16x32Decim8::configure(
	const std::array<tap_t, taps_count>& taps,
	const int32_t scale
) {
	std::copy(taps.cbegin(), taps.cend(), taps_.begin());
	output_scale = scale;
	z_.fill({});
}

buffer_c16_t FIRC16xR16x32Decim8::execute(
	const buffer_c16_t& src,
	const buffer_c16_t& dst
) {
	vec2_s16* const z = static_cast<vec2_s16*>(__builtin_assume_aligned(z_.data(), 4));
	const vec2_s16* const t = static_cast<vec2_s16*>(__builtin_assume_aligned(taps_.data(), 4));
	uint32_t* const d = static_cast<uint32_t*>(__builtin_assume_aligned(dst.p, 4));

	const auto k = output_scale;

	const size_t count = src.count / decimation_factor;
	for(size_t i=0; i<count; i++) {
		const vec2_s16* const in = static_cast<const vec2_s16*>(__builtin_assume_aligned(&src.p[i * decimation_factor], 4));

		complex32_t accum;

		// Oldest samples are discarded.
		accum = mac_shift(z, t, 0, accum);
		accum = mac_shift(z, t, 1, accum);
		accum = mac_shift(z, t, 2, accum);
		accum = mac_shift(z, t, 3, accum);

		// Middle samples are shifted earlier in the "z" delay buffer.
		accum = mac_shift_and_store(z, t, decimation_factor, 0, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 1, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 2, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 3, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 4, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 5, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 6, accum);
		accum = mac_shift_and_store(z, t, decimation_factor, 7, accum);

		// Newest samples come from "in" buffer, are copied to "z" delay buffer.
		accum = mac_shift_and_store_new_c16_samples(z, t, in, decimation_factor, 0, taps_count, accum);
		accum = mac_shift_and_store_new_c16_samples(z, t, in, decimation_factor, 1, taps_count, accum);
		accum = mac_shift_and_store_new_c16_samples(z, t, in, decimation_factor, 2, taps_count, accum);
		accum = mac_shift_and_store_new_c16_samples(z, t, in, decimation_factor, 3, taps_count, accum);

		d[i] = scale_round_and_pack(accum, k);
	}

	return {
		dst.p,
		count,
		src.sampling_rate / decimation_factor
	};
}

buffer_c16_t Complex8DecimateBy2CIC3::execute(const buffer_c8_t& src, const buffer_c16_t& dst) {
	/* Decimates by two using a non-recursive third-order CIC filter.
	 */

	/* CIC filter (decimating by two):
	 * 	D_I0 = i3 * 1 + i2 * 3 + i1 * 3 + i0 * 1
	 * 	D_Q0 = q3 * 1 + q2 * 3 + q1 * 3 + q0 * 1
	 *
	 * 	D_I1 = i5 * 1 + i4 * 3 + i3 * 3 + i2 * 1
	 * 	D_Q1 = q5 * 1 + q4 * 3 + q3 * 3 + q2 * 1
	 */

	uint32_t i1_i0 = _i1_i0;
	uint32_t q1_q0 = _q1_q0;

	/* 3:1 Scaled by 32 to normalize output to +/-32768-ish. */
	constexpr uint32_t scale_factor = 32;
	constexpr uint32_t k_3_1 = 0x00030001 * scale_factor;
	uint32_t* src_p = reinterpret_cast<uint32_t*>(&src.p[0]);
	uint32_t* const src_end = reinterpret_cast<uint32_t*>(&src.p[src.count]);
	uint32_t* dst_p = reinterpret_cast<uint32_t*>(&dst.p[0]);
	while(src_p < src_end) {
		const uint32_t q3_i3_q2_i2 = *(src_p++);						// 3
		const uint32_t q5_i5_q4_i4 = *(src_p++);

		const uint32_t d_i0_partial = __SMUAD(k_3_1, i1_i0);			// 1: = 3 * i1 + 1 * i0
		const uint32_t i3_i2 = __SXTB16(q3_i3_q2_i2,  0);				// 1: (q3_i3_q2_i2 ror  0)[23:16]:(q3_i3_q2_i2 ror  0)[7:0]
		const uint32_t d_i0 = __SMLADX(k_3_1, i3_i2, d_i0_partial);		// 1: + 3 * i2 + 1 * i3

		const uint32_t d_q0_partial = __SMUAD(k_3_1, q1_q0);			// 1: = 3 * q1 * 1 * q0
		const uint32_t q3_q2 = __SXTB16(q3_i3_q2_i2,  8);				// 1: (q3_i3_q2_i2 ror  8)[23:16]:(q3_i3_q2_i2 ror  8)[7:0]
		const uint32_t d_q0 = __SMLADX(k_3_1, q3_q2, d_q0_partial);		// 1: + 3 * q2 + 1 * q3 

		const uint32_t d_q0_i0 = __PKHBT(d_i0, d_q0, 16);				// 1: (Rm<<16)[31:16]:Rn[15:0]

		const uint32_t d_i1_partial = __SMUAD(k_3_1, i3_i2);			// 1: = 3 * i3 + 1 * i2
		const uint32_t i5_i4 = __SXTB16(q5_i5_q4_i4,  0);				// 1: (q5_i5_q4_i4 ror  0)[23:16]:(q5_i5_q4_i4 ror  0)[7:0]
		const uint32_t d_i1 = __SMLADX(k_3_1, i5_i4, d_i1_partial);		// 1: + 1 * i5 + 3 * i4

		const uint32_t d_q1_partial = __SMUAD(k_3_1, q3_q2);			// 1: = 3 * q3 * 1 * q2
		const uint32_t q5_q4 = __SXTB16(q5_i5_q4_i4,  8);				// 1: (q5_i5_q4_i4 ror  8)[23:16]:(q5_i5_q4_i4 ror  8)[7:0]
		const uint32_t d_q1 = __SMLADX(k_3_1, q5_q4, d_q1_partial);		// 1: + 1 * q5 + 3 * q4 

		const uint32_t d_q1_i1 = __PKHBT(d_i1, d_q1, 16);				// 1: (Rm<<16)[31:16]:Rn[15:0]

		*(dst_p++) = d_q0_i0;											// 3
		*(dst_p++) = d_q1_i1;

		i1_i0 = i5_i4;
		q1_q0 = q5_q4;
	}
	_i1_i0 = i1_i0;
	_q1_q0 = q1_q0;

	return { dst.p, src.count / 2, src.sampling_rate / 2 };
}

buffer_c16_t TranslateByFSOver4AndDecimateBy2CIC3::execute(const buffer_c8_t& src, const buffer_c16_t& dst) {
	/* Translates incoming complex<int8_t> samples by -fs/4,
	 * decimates by two using a non-recursive third-order CIC filter.
	 */

	/* Derivation of algorithm:
	 * Original CIC filter (decimating by two):
	 * 	D_I0 = i3 * 1 + i2 * 3 + i1 * 3 + i0 * 1
	 * 	D_Q0 = q3 * 1 + q2 * 3 + q1 * 3 + q0 * 1
	 *
	 * 	D_I1 = i5 * 1 + i4 * 3 + i3 * 3 + i2 * 1
	 * 	D_Q1 = q5 * 1 + q4 * 3 + q3 * 3 + q2 * 1
	 *
	 * Translate -fs/4, phased 180 degrees, accomplished by complex multiplication
	 * of complex length-4 sequence:
	 *
	 * Substitute:
	 *	i0 = -i0, q0 = -q0
	 *	i1 = -q1, q1 =  i1
	 *	i2 =  i2, q2 =  q2
	 *	i3 =  q3, q3 = -i3
	 *	i4 = -i4, q4 = -q4
	 *	i5 = -q5, q5 =  i5
	 *
	 * Resulting taps (with decimation by 2, four samples in, two samples out):
	 *	D_I0 =  q3 * 1 +  i2 * 3 + -q1 * 3 + -i0 * 1
	 *	D_Q0 = -i3 * 1 +  q2 * 3 +  i1 * 3 + -q0 * 1
 	 *
	 *	D_I1 = -q5 * 1 + -i4 * 3 +  q3 * 3 +  i2 * 1
	 *	D_Q1 =  i5 * 1 + -q4 * 3 + -i3 * 3 +  q2 * 1
	 */

	// 6 cycles per complex input sample, not including loop overhead.
	uint32_t q1_i0 = _q1_i0;
	uint32_t q0_i1 = _q0_i1;
	/* 3:1 Scaled by 32 to normalize output to +/-32768-ish. */
	constexpr uint32_t scale_factor = 32;
	const uint32_t k_3_1 = 0x00030001 * scale_factor;
	uint32_t* src_p = reinterpret_cast<uint32_t*>(&src.p[0]);
	uint32_t* const src_end = reinterpret_cast<uint32_t*>(&src.p[src.count]);
	uint32_t* dst_p = reinterpret_cast<uint32_t*>(&dst.p[0]);
	while(src_p < src_end) {
		const uint32_t q3_i3_q2_i2 = *(src_p++);			// 3
		const uint32_t q5_i5_q4_i4 = *(src_p++);

		const uint32_t i2_i3 = __SXTB16(q3_i3_q2_i2, 16);			// 1: (q3_i3_q2_i2 ror 16)[23:16]:(q3_i3_q2_i2 ror 16)[7:0]
		const uint32_t q3_q2 = __SXTB16(q3_i3_q2_i2,  8);			// 1: (q3_i3_q2_i2 ror  8)[23:16]:(q3_i3_q2_i2 ror  8)[7:0]
		const uint32_t i2_q3 = __PKHTB(i2_i3, q3_q2, 16);			// 1: Rn[31:16]:(Rm>>16)[15:0]
		const uint32_t i3_q2 = __PKHBT(q3_q2, i2_i3, 16);			// 1:(Rm<<16)[31:16]:Rn[15:0]

		// D_I0 = 3 * (i2 - q1) + (q3 - i0)
		const uint32_t i2_m_q1_q3_m_i0 = __QSUB16(i2_q3, q1_i0);	// 1: Rn[31:16]-Rm[31:16]:Rn[15:0]-Rm[15:0]
		const uint32_t d_i0 = __SMUAD(k_3_1, i2_m_q1_q3_m_i0);		// 1: Rm[15:0]*Rs[15:0]+Rm[31:16]*Rs[31:16]

		// D_Q0 = 3 * (q2 + i1) - (i3 + q0)
		const uint32_t i3_p_q0_q2_p_i1 = __QADD16(i3_q2, q0_i1);	// 1: Rn[31:16]+Rm[31:16]:Rn[15:0]+Rm[15:0]
		const uint32_t d_q0 = __SMUSDX(i3_p_q0_q2_p_i1, k_3_1);		// 1: Rm[15:0]*Rs[31:16]–Rm[31:16]*RsX[15:0]
		const uint32_t d_q0_i0 = __PKHBT(d_i0, d_q0, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		const uint32_t i5_i4 = __SXTB16(q5_i5_q4_i4,  0);			// 1: (q5_i5_q4_i4 ror  0)[23:16]:(q5_i5_q4_i4 ror  0)[7:0]
		const uint32_t q4_q5 = __SXTB16(q5_i5_q4_i4, 24);			// 1: (q5_i5_q4_i4 ror 24)[23:16]:(q5_i5_q4_i4 ror 24)[7:0]
		const uint32_t q4_i5 = __PKHTB(q4_q5, i5_i4, 16);			// 1: Rn[31:16]:(Rm>>16)[15:0]
		const uint32_t q5_i4 = __PKHBT(i5_i4, q4_q5, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		// D_I1 = (i2 - q5) + 3 * (q3 - i4)
		const uint32_t i2_m_q5_q3_m_i4 = __QSUB16(i2_q3, q5_i4);	// 1: Rn[31:16]-Rm[31:16]:Rn[15:0]-Rm[15:0]
		const uint32_t d_i1 = __SMUADX(i2_m_q5_q3_m_i4, k_3_1);		// 1: Rm[15:0]*Rs[31:16]+Rm[31:16]*Rs[15:0]

		// D_Q1 = (i5 + q2) - 3 * (q4 + i3)
		const uint32_t q4_p_i3_i5_p_q2 = __QADD16(q4_i5, i3_q2);	// 1: Rn[31:16]+Rm[31:16]:Rn[15:0]+Rm[15:0]
		const uint32_t d_q1 = __SMUSD(k_3_1, q4_p_i3_i5_p_q2);		// 1: Rm[15:0]*Rs[15:0]–Rm[31:16]*Rs[31:16]
		const uint32_t d_q1_i1 = __PKHBT(d_i1, d_q1, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]
		*(dst_p++) = d_q0_i0;							// 3
		*(dst_p++) = d_q1_i1;

		q1_i0 = q5_i4;
		q0_i1 = q4_i5;
	}
	_q1_i0 = q1_i0;
	_q0_i1 = q0_i1;

	return { dst.p, src.count / 2, src.sampling_rate / 2 };
}

buffer_c16_t DecimateBy2CIC3::execute(
	const buffer_c16_t& src,
	const buffer_c16_t& dst
) {
	/* Complex non-recursive 3rd-order CIC filter (taps 1,3,3,1).
	 * Gain of 8.
	 * Consumes 16 bytes (4 s16:s16 samples) per loop iteration,
	 * Produces  8 bytes (2 s16:s16 samples) per loop iteration.
	 */
	uint32_t t1 = _iq0;
	uint32_t t2 = _iq1;
	const uint32_t taps = 0x00000003;
	void* s = src.p;
	void* d = dst.p;
	const auto d_end = &dst.p[src.count / 2];
	while(d < d_end) {
		uint32_t i = __SXTH(t1, 0);			/* 1: I0 */
		uint32_t q = __SXTH(t1, 16);			/* 1: Q0 */
		i = __SMLABB(t2, taps, i);	/* 1: I1*3 + I0 */
		q = __SMLATB(t2, taps, q);	/* 1: Q1*3 + Q0 */

		const uint32_t t3 = *__SIMD32(s)++;		/* 3: Q2:I2 */
		const uint32_t t4 = *__SIMD32(s)++;		/*    Q3:I3 */

		i = __SMLABB(t3, taps, i);	/* 1: I2*3 + I1*3 + I0 */
		q = __SMLATB(t3, taps, q);	/* 1: Q2*3 + Q1*3 + Q0 */
		int32_t si0 = __SXTAH(i, t4,  0);		/* 1: I3 + Q2*3 + Q1*3 + Q0 */
		int32_t sq0 = __SXTAH(q, t4, 16);		/* 1: Q3 + Q2*3 + Q1*3 + Q0 */
		i = __BFI(si0 / 8, sq0 / 8, 16, 16);	/* 1: D2_Q0:D2_I0 */
		*__SIMD32(d)++ = i;			/* D2_Q0:D2_I0 */

		i = __SXTH(t3, 0);			/* 1: I2 */
		q = __SXTH(t3, 16);			/* 1: Q2 */
		i = __SMLABB(t4, taps, i);	/* 1: I3*3 + I2 */
		q = __SMLATB(t4, taps, q);	/* 1: Q3*3 + Q2 */

		t1 = *__SIMD32(s)++;		/* 3: Q4:I4 */
		t2 = *__SIMD32(s)++;		/*    Q5:I5 */

		i = __SMLABB(t1, taps, i);	/* 1: I4*3 + I3*3 + I2 */
		q = __SMLATB(t1, taps, q);	/* 1: Q4*3 + Q3*3 + Q2 */
		int32_t si1 = __SXTAH(i, t2, 0) ;		/* 1: I5 + Q4*3 + Q3*3 + Q2 */
		int32_t sq1 = __SXTAH(q, t2, 16);		/* 1: Q5 + Q4*3 + Q3*3 + Q2 */
		i = __BFI(si1 / 8, sq1 / 8, 16, 16);	/* 1: D2_Q1:D2_I1 */
		*__SIMD32(d)++ = i;			/* D2_Q1:D2_I1 */
	}
	_iq0 = t1;
	_iq1 = t2;

	return { dst.p, src.count / 2, src.sampling_rate / 2 };
}

void FIR64AndDecimateBy2Real::configure(
	const std::array<int16_t, taps_count>& new_taps
) {
	std::copy(new_taps.cbegin(), new_taps.cend(), taps.begin());
}

buffer_s16_t FIR64AndDecimateBy2Real::execute(
	const buffer_s16_t& src,
	const buffer_s16_t& dst
) {
	/* int16_t input (sample count "n" must be multiple of 4)
	 * -> int16_t output, decimated by 2.
	 * taps are normalized to 1 << 16 == 1.0.
	 */
	auto src_p = src.p;
	auto dst_p = dst.p;
	int32_t n = src.count;
	for(; n>0; n-=2) {
		z[taps_count-2] = *(src_p++);
		z[taps_count-1] = *(src_p++);

		int32_t t = 0;
		for(size_t j=0; j<taps_count; j+=4) {
			t += z[j+0] * taps[j+0];
			t += z[j+1] * taps[j+1];
			t += z[j+2] * taps[j+2];
			t += z[j+3] * taps[j+3];

			z[j+0] = z[j+0+2];
			z[j+1] = z[j+1+2];
			z[j+2] = z[j+2+2];
			z[j+3] = z[j+3+2];
		}
		*(dst_p++) = t / 65536;
	}

	return { dst.p, src.count / 2, src.sampling_rate / 2 };
}

void FIRAndDecimateComplex::configure_common(
	const size_t taps_count, const size_t decimation_factor
) {
	samples_ = std::make_unique<samples_t>(taps_count);
	taps_reversed_ = std::make_unique<taps_t>(taps_count);
	taps_count_ = taps_count;
	decimation_factor_ = decimation_factor;
}

buffer_c16_t FIRAndDecimateComplex::execute(
	const buffer_c16_t& src,
	const buffer_c16_t& dst
) {
	/* int16_t input (sample count "n" must be multiple of decimation_factor)
	 * -> int16_t output, decimated by decimation_factor.
	 * taps are normalized to 1 << 16 == 1.0.
	 */
	const auto output_sampling_rate = src.sampling_rate / decimation_factor_;
	const size_t output_samples = src.count / decimation_factor_;
	
	void* dst_p = dst.p;
	const buffer_c16_t result { dst.p, output_samples, output_sampling_rate };

	const void* src_p = src.p;
	size_t outer_count = output_samples;
	while(outer_count > 0) {
		/* Put new samples into delay buffer */
		void* z_new_p = &samples_[taps_count_ - decimation_factor_];
		for(size_t i=0; i<decimation_factor_; i++) {
			*__SIMD32(z_new_p)++ = *__SIMD32(src_p)++;
		}

		size_t loop_count = taps_count_ / 8;
		void* t_p = &taps_reversed_[0];
		void* z_p = &samples_[0];

		int64_t t_real = 0;
		int64_t t_imag = 0;

		while(loop_count > 0) {
			const auto tap0 = *__SIMD32(t_p)++;
			const auto sample0 = *__SIMD32(z_p)++;
			const auto tap1 = *__SIMD32(t_p)++;
			const auto sample1 = *__SIMD32(z_p)++;
			t_real = __SMLSLD(sample0, tap0, t_real);
			t_imag = __SMLALDX(sample0, tap0, t_imag);
			t_real = __SMLSLD(sample1, tap1, t_real);
			t_imag = __SMLALDX(sample1, tap1, t_imag);

			const auto tap2 = *__SIMD32(t_p)++;
			const auto sample2 = *__SIMD32(z_p)++;
			const auto tap3 = *__SIMD32(t_p)++;
			const auto sample3 = *__SIMD32(z_p)++;
			t_real = __SMLSLD(sample2, tap2, t_real);
			t_imag = __SMLALDX(sample2, tap2, t_imag);
			t_real = __SMLSLD(sample3, tap3, t_real);
			t_imag = __SMLALDX(sample3, tap3, t_imag);

			const auto tap4 = *__SIMD32(t_p)++;
			const auto sample4 = *__SIMD32(z_p)++;
			const auto tap5 = *__SIMD32(t_p)++;
			const auto sample5 = *__SIMD32(z_p)++;
			t_real = __SMLSLD(sample4, tap4, t_real);
			t_imag = __SMLALDX(sample4, tap4, t_imag);
			t_real = __SMLSLD(sample5, tap5, t_real);
			t_imag = __SMLALDX(sample5, tap5, t_imag);

			const auto tap6 = *__SIMD32(t_p)++;
			const auto sample6 = *__SIMD32(z_p)++;
			const auto tap7 = *__SIMD32(t_p)++;
			const auto sample7 = *__SIMD32(z_p)++;
			t_real = __SMLSLD(sample6, tap6, t_real);
			t_imag = __SMLALDX(sample6, tap6, t_imag);
			t_real = __SMLSLD(sample7, tap7, t_real);
			t_imag = __SMLALDX(sample7, tap7, t_imag);

			loop_count--;
		}

		/* TODO: Re-evaluate whether saturation is performed, normalization,
		 * all that jazz.
		 */
		const int32_t r = t_real >> 16;
		const int32_t i = t_imag >> 16;
		const int32_t r_sat = __SSAT(r, 16);
		const int32_t i_sat = __SSAT(i, 16);
		*__SIMD32(dst_p)++ = __PKHBT(
			r_sat,
			i_sat,
			16
		);

		/* Shift sample buffer left/down by decimation factor. */
		const size_t unroll_factor = 4;
		size_t shift_count = (taps_count_ - decimation_factor_) / unroll_factor;

		void* t = &samples_[0];
		const void* s = &samples_[decimation_factor_];
		
		while(shift_count > 0) {
			*__SIMD32(t)++ = *__SIMD32(s)++;
			*__SIMD32(t)++ = *__SIMD32(s)++;
			*__SIMD32(t)++ = *__SIMD32(s)++;
			*__SIMD32(t)++ = *__SIMD32(s)++;
			shift_count--;
		}

		shift_count = (taps_count_ - decimation_factor_) % unroll_factor;
		while(shift_count > 0) {
			*__SIMD32(t)++ = *__SIMD32(s)++;
			shift_count--;
		}

		outer_count--;
	}

	return result;
}

buffer_s16_t DecimateBy2CIC4Real::execute(
	const buffer_s16_t& src,
	const buffer_s16_t& dst
) {
	auto src_p = src.p;
	auto dst_p = dst.p;
	int32_t n = src.count;
	for(; n>0; n-=2) {
		/* TODO: Probably a lot of room to optimize... */
		z[0] = z[2];
		z[1] = z[3];
		z[2] = z[4];
		z[3] = *(src_p++);
		z[4] = *(src_p++);

		int32_t t = z[0] + z[1] * 4 + z[2] * 6 + z[3] * 4 + z[4];
		*(dst_p++) = t / 16;
	}

	return { dst.p, src.count / 2, src.sampling_rate / 2 };
}

} /* namespace decimate */
} /* namespace dsp */
