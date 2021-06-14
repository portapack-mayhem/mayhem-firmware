/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "tone_gen.hpp"
#include "sine_table_int8.hpp"


int32_t ToneGen::tone_from_sine_table() {
	int32_t tone_sample = sine_table_i8[(tone_phase_ & 0xFF000000U) >> 24] << 24;
	tone_phase_ += delta_;

	return tone_sample;
}

int32_t ToneGen::tone_square() {
	int32_t tone_sample = 0;

	if(tone_phase_  < (UINT32_MAX / 2)) {
		tone_sample = INT32_MAX;
	}
	else {
		tone_sample = INT32_MIN;
	}

	tone_phase_ += delta_;

	return tone_sample;
}

void ToneGen::configure(const uint32_t delta, const float tone_mix_weight) {
	delta_ = delta;
	tone_mix_weight_ = tone_mix_weight;
	input_mix_weight_ = 1.0 - tone_mix_weight;
	current_tone_type_ = sine;
}

void ToneGen::configure(const uint32_t delta, const float tone_mix_weight, const tone_type tone_type) {
	delta_ = delta;
	tone_mix_weight_ = tone_mix_weight;
	input_mix_weight_ = 1.0 - tone_mix_weight;
	current_tone_type_ = tone_type;
}

int32_t ToneGen::process(const int32_t sample_in) {
	if (!delta_)
		return sample_in;
	
	int32_t tone_sample = 0;
	
	if(current_tone_type_ == sine) {
		tone_sample = tone_from_sine_table();
	}
	else if(current_tone_type_ == square) {
		tone_sample = tone_square();
	}
	
	return (sample_in * input_mix_weight_) + (tone_sample * tone_mix_weight_);
}
