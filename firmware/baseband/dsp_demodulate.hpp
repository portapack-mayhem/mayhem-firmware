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
	buffer_s16_t execute(
		buffer_c16_t src,
		buffer_s16_t dst
	);
};

class FM {
public:
	constexpr FM(
	) : z_ { 0 },
		k { 0 }
	{
	}
	
	/*
	 * angle: -pi to pi. output range: -32768 to 32767.
	 * Maximum delta-theta (output of atan2) at maximum deviation frequency:
	 * delta_theta_max = 2 * pi * deviation / sampling_rate
	 */
	constexpr FM(
		const float sampling_rate,
		const float deviation_hz
	) : z_ { 0 },
		k { static_cast<float>(32767.0f / (2.0 * pi * deviation_hz / sampling_rate)) }
	{
	}

	buffer_s16_t execute(
		buffer_c16_t src,
		buffer_s16_t dst
	);

	void configure(const float sampling_rate, const float deviation_hz) {
		k = static_cast<float>(32767.0f / (2.0 * pi * deviation_hz / sampling_rate));
	}

private:
	complex16_t::rep_type z_;
	float k;
};

} /* namespace demodulate */
} /* namespace dsp */

#endif/*__DSP_DEMODULATE_H__*/
