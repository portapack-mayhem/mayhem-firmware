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

#include "dsp_iir.hpp"

#include <hal.h>

void IIRBiquadFilter::execute(const buffer_s16_t& buffer_in, const buffer_s16_t& buffer_out) {
	// TODO: Assert that buffer_out.count == buffer_in.count.
	for(size_t i=0; i<buffer_out.count; i++) {
		const int32_t output_sample = execute_sample(buffer_in.p[i]);
		const int32_t output_sample_saturated = __SSAT(output_sample, 16);
		buffer_out.p[i] = output_sample_saturated;
	}
}

void IIRBiquadFilter::execute_in_place(const buffer_s16_t& buffer) {
	execute(buffer, buffer);
}

float IIRBiquadFilter::execute_sample(const float in) {
	x[0] = x[1];
	x[1] = x[2];
	x[2] = in;

	y[0] = y[1];
	y[1] = y[2];
	y[2] = config.b[0] * x[2] + config.b[1] * x[1] + config.b[2] * x[0]
	                          - config.a[1] * y[1] - config.a[2] * y[0];

	return y[2];
}
