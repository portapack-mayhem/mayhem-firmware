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

#ifndef __TONE_GEN_H__
#define __TONE_GEN_H__

#include <cstdint>
#include <cstddef>
#include <bitset>

static const std::bitset<2048> wave_bits (0xFFFFFF);

class ToneGen {
public:
	enum tone_type { sine, square };

	/*ToneGen(const size_t sample_rate
	) : sample_rate_ { sample_rate }
	{};*/

	void configure(const uint32_t delta, const float tone_mix_weight);
	void configure(const uint32_t delta, const float tone_mix_weight, const tone_type tone_type);

	int32_t process(const int32_t sample_in);

private:
	tone_type current_tone_type_ { sine };

	//size_t sample_rate_;
	float input_mix_weight_ { 1 };
	float tone_mix_weight_ { 0 };
	uint32_t delta_ { 0 };
	uint32_t tone_phase_ { 0 };

	/**
	 * Generator function for sine waves:
	 */
	int32_t tone_from_sine_table();

	/**
	 * Generator function for square waves:
	 */	
	int32_t tone_square();
};

#endif /* __TONE_GEN_H__ */
