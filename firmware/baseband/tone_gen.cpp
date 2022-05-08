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


/*
int32_t ToneGen::tone_sine() {
	//  TODO :  Added for Sonde App. We keep it by now , but it needs to be reviewed in Sonde
	//  Hoepfully we can manage without it , same as previous fw  1.3.1  
	int32_t tone_sample = sine_table_i8[tone_phase_] * 0x1000000;
	tone_phase_ += delta_;

	return tone_sample;
}
*/ 


int32_t ToneGen::tone_square() {
	//  TODO :  Added for Sonde App. We keep it by now , but it needs to be reviewed in Sonde
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

/*   
void ToneGen::configure(const uint32_t delta, const float tone_mix_weight) {
	//  Confirmed !  It is not working well in the fw 1.4.4  Mic App , CTCSS generation, (but added for Sonde App)
	//  I Think it should be deleted or modified but not use it as it is in Mic App . 	

	delta_ = (uint8_t) ((delta & 0xFF000000U) >> 24);
	delta_ = delta;
	tone_mix_weight_ = tone_mix_weight;
	input_mix_weight_ = 1.0 - tone_mix_weight;
	current_tone_type_ = sine;
}
*/ 



void ToneGen::configure(const uint32_t freq, const float tone_mix_weight, const tone_type tone_type, const uint32_t sample_rate) {
	//  TODO :  Added for Sonde App. We keep it by now to avoid compile errors, but it needs to be reviewed in Sonde
	delta_ = (uint8_t) ((freq * sizeof(sine_table_i8)) / sample_rate);
	tone_mix_weight_ = tone_mix_weight;
	input_mix_weight_ = 1.0 - tone_mix_weight;
	current_tone_type_ = tone_type;
}  




//  ----Original available core SW code from fw 1.3.1 ,  Working also well in Mic App CTCSS Gen from fw 1.4.0 onwards

// Original direct-look-up synthesis algorithm with Fractional delta phase. It is OK
// Delta  and Accumulator fase are stored in 32 bits (4 bytes), 1st top byte used as Modulo-256 Sine look-up table [index]
// the lower 3 bytes (24 bits)  are used as a Fractional Detla and Accumulator phase, to have very finer Fstep control.

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
// -------------------------------------------------------------



int32_t ToneGen::process_square(const int32_t sample_in) {
	//  TODO :  Added for Sonde App. We keep it by now , but it needs to be reviewed in Sonde
	if (!delta_)
		return sample_in;
	
	int32_t tone_sample = 0;
	
		tone_sample = tone_square();
	
	
	return (sample_in * input_mix_weight_) + (tone_sample * tone_mix_weight_);
}
