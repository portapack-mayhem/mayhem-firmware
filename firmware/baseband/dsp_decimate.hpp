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

#ifndef __DSP_DECIMATE_H__
#define __DSP_DECIMATE_H__

#include <cstdint>
#include <array>
#include <memory>
#include <algorithm>

#include "utility.hpp"

#include "dsp_types.hpp"

#include "simd.hpp"

namespace dsp {
namespace decimate {

class Complex8DecimateBy2CIC3 {
public:
	buffer_c16_t execute(
		const buffer_c8_t& src,
		const buffer_c16_t& dst
	);

private:
	uint32_t _i1_i0 { 0 };
	uint32_t _q1_q0 { 0 };
};

class TranslateByFSOver4AndDecimateBy2CIC3 {
public:
	buffer_c16_t execute(
		const buffer_c8_t& src,
		const buffer_c16_t& dst
	);

private:
	uint32_t _q1_i0 { 0 };
	uint32_t _q0_i1 { 0 };
};

class DecimateBy2CIC3 {
public:
	buffer_c16_t execute(
		const buffer_c16_t& src,
		const buffer_c16_t& dst
	);

private:
	uint32_t _iq0 { 0 };
	uint32_t _iq1 { 0 };
};

class FIR64AndDecimateBy2Real {
public:
	static constexpr size_t taps_count = 64;

	void configure(
		const std::array<int16_t, taps_count>& taps
	);

	buffer_s16_t execute(
		const buffer_s16_t& src,
		const buffer_s16_t& dst
	);

private:
	std::array<int16_t, taps_count + 2> z { };
	std::array<int16_t, taps_count> taps { };
};

class FIRC8xR16x24FS4Decim4 {
public:
	static constexpr size_t taps_count = 24;
	static constexpr size_t decimation_factor = 4;

	using sample_t = complex8_t;
	using tap_t = int16_t;

	enum class Shift : bool {
		Down = true,
		Up = false
	};

	void configure(
		const std::array<tap_t, taps_count>& taps,
		const int32_t scale,
		const Shift shift = Shift::Down
	);

	buffer_c16_t execute(
		const buffer_c8_t& src,
		const buffer_c16_t& dst
	);
	
private:
	std::array<vec2_s16, taps_count - decimation_factor> z_ { };
	std::array<tap_t, taps_count> taps_ { };
	int32_t output_scale = 0;
};

class FIRC8xR16x24FS4Decim8 {
public:
	static constexpr size_t taps_count = 24;
	static constexpr size_t decimation_factor = 8;

	using sample_t = complex8_t;
	using tap_t = int16_t;

	enum class Shift : bool {
		Down = true,
		Up = false
	};

	void configure(
		const std::array<tap_t, taps_count>& taps,
		const int32_t scale,
		const Shift shift = Shift::Down
	);

	buffer_c16_t execute(
		const buffer_c8_t& src,
		const buffer_c16_t& dst
	);
	
private:
	std::array<vec2_s16, taps_count - decimation_factor> z_ { };
	std::array<tap_t, taps_count> taps_ { };
	int32_t output_scale = 0;
};

class FIRC16xR16x16Decim2 {
public:
	static constexpr size_t taps_count = 16;
	static constexpr size_t decimation_factor = 2;

	using sample_t = complex16_t;
	using tap_t = int16_t;

	void configure(
		const std::array<tap_t, taps_count>& taps,
		const int32_t scale
	);

	buffer_c16_t execute(
		const buffer_c16_t& src,
		const buffer_c16_t& dst
	);
	
private:
	std::array<vec2_s16, taps_count - decimation_factor> z_ { };
	std::array<tap_t, taps_count> taps_ { };
	int32_t output_scale = 0;
};

class FIRC16xR16x32Decim8 {
public:
	static constexpr size_t taps_count = 32;
	static constexpr size_t decimation_factor = 8;

	using sample_t = complex16_t;
	using tap_t = int16_t;

	void configure(
		const std::array<tap_t, taps_count>& taps,
		const int32_t scale
	);

	buffer_c16_t execute(
		const buffer_c16_t& src,
		const buffer_c16_t& dst
	);
	
private:
	std::array<vec2_s16, taps_count - decimation_factor> z_ { };
	std::array<tap_t, taps_count> taps_ { };
	int32_t output_scale = 0;
};

class FIRAndDecimateComplex {
public:
	using sample_t = complex16_t;
	using tap_t = complex16_t;

	using taps_t = tap_t[];

	/* NOTE! Current code makes an assumption that block of samples to be
	 * processed will be a multiple of the taps_count.
	 */

	template<typename T>
	void configure(
		const T& taps,
		const size_t decimation_factor
	) {
		configure(taps.data(), taps.size(), decimation_factor);
	}

	buffer_c16_t execute(
		const buffer_c16_t& src,
		const buffer_c16_t& dst
	);
	
private:
	using samples_t = sample_t[];

	std::unique_ptr<samples_t> samples_ { };
	std::unique_ptr<taps_t> taps_reversed_ { };
	size_t taps_count_ { 0 };
	size_t decimation_factor_ { 1 };

	template<typename T>
	void configure(
		const T* const taps,
		const size_t taps_count,
		const size_t decimation_factor
	) {
		configure_common(taps_count, decimation_factor);
		std::reverse_copy(&taps[0], &taps[taps_count], &taps_reversed_[0]);
	}

	void configure_common(
		const size_t taps_count,
		const size_t decimation_factor
	);
};

class DecimateBy2CIC4Real {
public:
	buffer_s16_t execute(
		const buffer_s16_t& src,
		const buffer_s16_t& dst
	);

private:
	int16_t z[5] { };
	int16_t _dummy { };	// TODO: Addresses GCC bug when constructing a class that's not sizeof() % 4 == 0?
};

} /* namespace decimate */
} /* namespace dsp */

#endif/*__DSP_DECIMATE_H__*/
