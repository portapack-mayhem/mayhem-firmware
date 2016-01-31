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

#include "dsp_squelch.hpp"

#include <cstdint>
#include <array>

bool FMSquelch::execute(const buffer_f32_t& audio) {
	if( threshold_squared == 0.0f ) {
		return true;
	}

	// TODO: No hard-coded array size.
	std::array<float, N> squelch_energy_buffer;
	const buffer_f32_t squelch_energy {
		squelch_energy_buffer.data(),
		squelch_energy_buffer.size()
	};
	non_audio_hpf.execute(audio, squelch_energy);

	float non_audio_max_squared = 0;
	for(const auto sample : squelch_energy_buffer) {
		const float sample_squared = sample * sample;
		if( sample_squared > non_audio_max_squared ) {
			non_audio_max_squared = sample_squared;
		}
	}

	return (non_audio_max_squared < threshold_squared);
}

void FMSquelch::set_threshold(const float new_value) {
	threshold_squared = new_value * new_value;
}
