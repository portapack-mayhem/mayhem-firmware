/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __DSP_IIR_H__
#define __DSP_IIR_H__

#include <array>

#include "dsp_types.hpp"

class IIRBiquadFilter {
public:
	// http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt

	// Assume all coefficients are normalized so that a0=1.0
	constexpr IIRBiquadFilter(
		std::array<float, 3> b,
		std::array<float, 3> a
	) : b(b),
		a(a)
	{
	}

	void execute(buffer_s16_t buffer_in, buffer_s16_t buffer_out) {
		// TODO: Assert that buffer_out.count == buffer_in.count.
		for(size_t i=0; i<buffer_out.count; i++) {
			buffer_out.p[i] = execute_sample(buffer_in.p[i]);
		}
	}

	void execute_in_place(buffer_s16_t buffer) {
		execute(buffer, buffer);
	}

private:
	const std::array<float, 3> b;
	const std::array<float, 3> a;
	std::array<float, 3> x { { 0.0f, 0.0f, 0.0f } };
	std::array<float, 3> y { { 0.0f, 0.0f, 0.0f } };

	float execute_sample(const float in) {
		x[0] = x[1];
		x[1] = x[2];
		x[2] = in;

		y[0] = y[1];
		y[1] = y[2];
		y[2] = b[0] * x[2] + b[1] * x[1] + b[2] * x[0]
		                   - a[1] * y[1] - a[2] * y[0];

		return y[2];
	}
};

#endif/*__DSP_IIR_H__*/
