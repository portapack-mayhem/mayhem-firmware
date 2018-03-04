/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "dsp_goertzel.hpp"

#include "complex.hpp"
#include "sine_table.hpp"

namespace dsp {

GoertzelDetector::GoertzelDetector(
	const float frequency,
	const uint32_t sample_rate
) {
	coefficient = 2.0 * sin_f32((2.0 * pi * frequency / sample_rate) - pi / 2.0);
}

float GoertzelDetector::execute(
	const buffer_s16_t& src
) {

	const size_t count = src.count;
	
	for (size_t i = 0; i < count; i++) {
		s[2] = s[1];
		s[1] = s[0];
		s[0] = src.p[i] + coefficient * s[1] - s[2];
	}

	const uint32_t sq0 = s[0] * s[0];
	const uint32_t sq1 = s[1] * s[1];
	float magnitude = __builtin_sqrtf(sq0 + sq1 - s[0] * s[1] * coefficient);

	return magnitude;
}

} /* namespace dsp */
