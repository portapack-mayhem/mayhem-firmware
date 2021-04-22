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

void IIRBiquadFilter::configure(const iir_biquad_config_t& new_config) {
	config = new_config;
}

void IIRBiquadFilter::execute(const buffer_f32_t& buffer_in, const buffer_f32_t& buffer_out) {
	const auto a_ = config.a;
	const auto b_ = config.b;
	
	auto x_ = x;
	auto y_ = y;

	// TODO: Assert that buffer_out.count == buffer_in.count.
	for(size_t i=0; i<buffer_out.count; i++) {
		x_[0] = x_[1];
		x_[1] = x_[2];
		x_[2] = buffer_in.p[i];

		y_[0] = y_[1];
		y_[1] = y_[2];
		y_[2] = b_[0] * x_[2] + b_[1] * x_[1] + b_[2] * x_[0]
		                      - a_[1] * y_[1] - a_[2] * y_[0];

		buffer_out.p[i] = y_[2];
	}

	x = x_;
	y = y_;
}

void IIRBiquadFilter::execute_in_place(const buffer_f32_t& buffer) {
	execute(buffer, buffer);
}

void IIRBiquadDF2Filter::configure(const iir_biquad_df2_config_t& config) {
	b0 = config[0] / config[3];
	b1 = config[1] / config[3];
	b2 = config[2] / config[3];
	a1 = config[4] / config[3];
	a2 = config[5] / config[3];
}

//  scipy.signal.sosfilt
//
//  x_n = x[i, n]  # make a temporary copy
//  # Use direct II transposed structure:
//  x[i, n] = b[s, 0] * x_n + zi[i, s, 0]
//  zi[i, s, 0] = (b[s, 1] * x_n - a[s, 0] * x[i, n] + zi[i, s, 1])
//  zi[i, s, 1] = (b[s, 2] * x_n - a[s, 1] * x[i, n])

float IIRBiquadDF2Filter::execute(float x) {
	float y;
	
	y = b0 * x + z0;
	z0 = b1 * x - a1 * y + z1;
	z1 = b2 * x - a2 * y;

	return y;
}
