/*
 * Copyright (C) 2020 Belousov Oleg
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

#include "dsp_modulate.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"
#include "tonesets.hpp"

namespace dsp {
namespace modulate {

Modulator::~Modulator() {
}

Mode Modulator::get_mode() {
	return mode;
}

void Modulator::set_mode(Mode new_mode) {
	mode = new_mode;
}

void Modulator::set_over(uint32_t new_over) {
    over = new_over;
}

void Modulator::set_gain_vumeter_beep(float new_audio_gain , bool new_play_beep ) {
     audio_gain = new_audio_gain ;    
	  play_beep = new_play_beep; 

}

int32_t Modulator::apply_gain_beep(int32_t sample_in, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message ) {

	if (!play_beep) {	// Apply GAIN  Scale factor.
    		sample_in *= audio_gain;   

	} else {			// We need to add audio beep sample.
			if (new_beep_timer) {
				new_beep_timer--;
			} else {
				new_beep_timer = baseband_fs * 0.05;			// 50ms
				
				if (new_beep_index == BEEP_TONES_NB) {
					configured_in = false;
					shared_memory.application_queue.push(new_txprogress_message);
				} else {
					beep_gen.configure(beep_deltas[new_beep_index], 1.0); // config sequentially the audio beep tone.
					new_beep_index++;
				}
			}
		    sample_in = beep_gen.process(0);    // Get sample of the selected sequence of 6  beep tones , and overwrite audio sample. Mix 0%.
		}
	return sample_in;    // Return audio mic scaled with gain , 8 bit sample or audio beep sample.		
}


///

SSB::SSB() : hilbert() {
	mode = Mode::LSB;
}

void SSB::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in,  uint32_t& new_beep_index, uint32_t& new_beep_timer,TXProgressMessage& new_txprogress_message ) {
	int32_t		sample = 0;
	int8_t		re = 0, im = 0;
	
	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % 128 == 0) {
			float	i = 0.0, q = 0.0;

		    sample = audio.p[counter / over] >> 2;
			//switch (mode) {
				//case Mode::LSB:	
			hilbert.execute(sample / 32768.0f, i, q);
				//case Mode::USB:	hilbert.execute(sample / 32768.0f, q, i);
				//default:	break;
			//}

			i *= 256.0f;   // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)	
			q *= 256.0f;   // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)	
			switch (mode) {
				case Mode::LSB:	re = q;	im = i;	break;
				case Mode::USB:	re = i;	im = q;	break;
				default:	re = 0; im = 0;	break;
			}
			//re = q;
			//im = i;
			//break;
		}
		
		buffer.p[counter] = { re, im };
	}
}

///

FM::FM() {
	mode = Mode::FM;
}

void FM::set_fm_delta(uint32_t new_delta) {
	fm_delta = new_delta;
}

void FM::set_tone_gen_configure(const uint32_t set_delta, const float set_tone_mix_weight) {
	 tone_gen.configure(set_delta, set_tone_mix_weight);
}	

void FM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message ) {
	int32_t		sample = 0;
	int8_t		re, im;

	for (size_t counter = 0; counter < buffer.count; counter++) {
	    sample = audio.p[counter / over] >> 8;

	sample = apply_gain_beep(sample, configured_in, new_beep_index, new_beep_timer, new_txprogress_message ); 	// Scale sample by gain , and update vu-meter.

    /*  
	// Update vu-meter bar in the LCD screen.(of both cases , audio mic or beep )
	    power_acc += (sample_in < 0) ? -sample_in : sample_in;	// Power average for UI vu-meter
	
		if (power_acc_count) {
			power_acc_count--;
		} else {
			power_acc_count = divider;
			level_message.value = power_acc / (divider / 4);	// Why ?
			shared_memory.application_queue.push(level_message);
			power_acc = 0;
		}	
    */    

		sample = tone_gen.process(sample);			// Add Key_Tone or CTCSS subtone
	    delta = sample * fm_delta;					// Modulate FM

		phase += delta;
		sphase = phase >> 24;

		re = (sine_table_i8[(sphase + 64) & 255]);
		im = (sine_table_i8[sphase]);
		
		buffer.p[counter] = { re, im };
	}
}

AM::AM() {
	mode = Mode::AM;
}

void AM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message) {
	int32_t         sample = 0;
	int8_t          re = 0, im = 0;
	float		q = 0.0;
	
	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % 128 == 0) {
			sample = audio.p[counter / over] >> 2;
		}

		q = sample / 32768.0f;
		q *= 256.0f;										 // Original 64.0f,now x4 (+12 dB's BB_modulation in AM & DSB)	
		switch (mode) {
			case Mode::AM:	re = q + 80; im = q + 80; break; // Original DC add +20_DC_level=carrier,now x4 (+12dB's AM carrier)
			case Mode::DSB:	re = q; im = q; break;
			default:	break;
		}
		buffer.p[counter] = { re, im };
	}
}

}
}
