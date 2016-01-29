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

struct iir_biquad_config_t {
	std::array<float, 3> b;
	std::array<float, 3> a;
};

constexpr iir_biquad_config_t iir_config_passthrough {
	{ { 1.0f, 0.0f, 0.0f } },
	{ { 0.0f, 0.0f, 0.0f } },
};

constexpr iir_biquad_config_t iir_config_no_pass {
	{ { 0.0f, 0.0f, 0.0f } },
	{ { 0.0f, 0.0f, 0.0f } },
};

class IIRBiquadFilter {
public:
	// http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
	constexpr IIRBiquadFilter(
	) : IIRBiquadFilter(iir_config_no_pass)
	{
	}

	// Assume all coefficients are normalized so that a0=1.0
	constexpr IIRBiquadFilter(
		const iir_biquad_config_t& config
	) : config(config)
	{
	}

	void configure(const iir_biquad_config_t& new_config);

	void execute(const buffer_f32_t& buffer_in, const buffer_f32_t& buffer_out);
	void execute_in_place(const buffer_f32_t& buffer);

private:
	iir_biquad_config_t config;
	std::array<float, 3> x { { 0.0f, 0.0f, 0.0f } };
	std::array<float, 3> y { { 0.0f, 0.0f, 0.0f } };
};

#endif/*__DSP_IIR_H__*/
