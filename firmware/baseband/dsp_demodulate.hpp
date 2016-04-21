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

#ifndef __DSP_DEMODULATE_H__
#define __DSP_DEMODULATE_H__

#include "dsp_types.hpp"

namespace dsp {
namespace demodulate {

class AM {
public:
	buffer_f32_t execute(
		const buffer_c16_t& src,
		const buffer_f32_t& dst
	);

private:
	static constexpr float k = 1.0f / 32768.0f;
};

class SSB {
public:
	buffer_f32_t execute(
		const buffer_c16_t& src,
		const buffer_f32_t& dst
	);

private:
	static constexpr float k = 1.0f / 32768.0f;
};

class FM {
public:
	buffer_f32_t execute(
		const buffer_c16_t& src,
		const buffer_f32_t& dst
	);

	buffer_s16_t execute(
		const buffer_c16_t& src,
		const buffer_s16_t& dst
	);

	void configure(const float sampling_rate, const float deviation_hz);

private:
	complex16_t::rep_type z_ { 0 };
	float kf { 0 };
	float ks16 { 0 };
};

} /* namespace demodulate */
} /* namespace dsp */

#endif/*__DSP_DEMODULATE_H__*/
