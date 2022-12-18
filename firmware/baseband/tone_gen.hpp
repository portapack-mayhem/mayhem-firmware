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

class ToneGen {
public:
	enum tone_type { sine, square };    // TODO:  Added for Radio Sonde.cpp PR 376, 381 , we need to check if keep or not.

	/*ToneGen(const size_t sample_rate
	) : sample_rate_ { sample_rate }
	{};*/

	void configure(const uint32_t delta, const float tone_mix_weight);
	void configure(const uint32_t freq, const float tone_mix_weight, const tone_type tone_type, const uint32_t sample_rate);

	int32_t process(const int32_t sample_in);
	int32_t process_square(const int32_t sample_in);

private:
	tone_type current_tone_type_ { sine };

	float input_mix_weight_ { 1 };
	float tone_mix_weight_ { 0 };

	uint32_t delta_ { 0 };
	uint32_t tone_phase_ { 0 };

//	uint8_t delta_ { 0 };		// TODO:  Added for Radio Sonde.cpp PR 376, 381 , we need to check if keep or not.
//	uint8_t tone_phase_ { 0 };  // TODO:  Added for Radio Sonde.cpp PR 376, 381 , we need to check if keep or not.

	/**
	 * Generator function which selects every other sample from the reference sine waveform to the output sample:
	 */
	int32_t tone_sine();// TODO:  Added for Radio Sonde.cpp PR 376, 381 , we need to check if keep or not.


	/**
	 * Generator function for square waves:
	 */	
	int32_t tone_square();		// TODO:  Added for Radio Sonde.cpp PR 376, 381 , we need to check if keep or not.

};

#endif /* __TONE_GEN_H__ */
