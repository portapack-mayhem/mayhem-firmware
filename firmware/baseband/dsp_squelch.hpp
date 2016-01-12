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

#ifndef __DSP_SQUELCH_H__
#define __DSP_SQUELCH_H__

#include "buffer.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"

#include <cstdint>
#include <cstddef>

class FMSquelch {
public:
	bool execute(const buffer_f32_t& audio);

	void set_threshold(const float new_value);

private:
	static constexpr size_t N = 32;
	float threshold_squared { 0.0f };

	IIRBiquadFilter non_audio_hpf { non_audio_hpf_config };
};

#endif/*__DSP_SQUELCH_H__*/
