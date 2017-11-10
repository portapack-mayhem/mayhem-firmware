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

void ToneGen::configure(const uint32_t delta, const float tone_mix_weight) {
	delta_ = delta;
	tone_mix_weight_ = tone_mix_weight;
	input_mix_weight_ = 1.0 - tone_mix_weight;
}

int32_t ToneGen::process(const int32_t sample_in) {
	if (!delta_)
		return sample_in;
	
	int32_t tone_sample = sine_table_i8[(tone_phase_ & 0xFF000000U) >> 24];
	tone_phase_ += delta_;
	
	return (sample_in * input_mix_weight_) + (tone_sample * tone_mix_weight_);
}
