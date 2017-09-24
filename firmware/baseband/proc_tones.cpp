/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "proc_tones.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

// This is called at 1536000/2048 = 750Hz
void TonesProcessor::execute(const buffer_c8_t& buffer) {
	
	if (!configured) return;
	
	ai = 0;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		// Tone generation at full samplerate
		if (silence_count) {
			// Just occupy channel with carrier
			silence_count--;
			if (!silence_count) {
				sample_count = 0;
				tone_a_phase = 0;
				tone_b_phase = 0;
			}
			tone_sample = 0;
			re = 0;
			im = 0;
		} else {
			if (!sample_count) {
				digit = shared_memory.bb_data.tones_data.message[digit_pos];
				if (digit_pos >= message_length) {
					configured = false;
					txprogress_message.done = true;
					shared_memory.application_queue.push(txprogress_message);
					return;
				} else {
					txprogress_message.progress = digit_pos;	// Inform UI about progress
					txprogress_message.done = false;
					shared_memory.application_queue.push(txprogress_message);
				}
				
				digit_pos++;
				
				if (digit >= 32) {	//  || (tone_deltas[digit] == 0)
					sample_count = shared_memory.bb_data.tones_data.silence;
				} else {
					if (!dual_tone) {
						tone_a_delta = tone_deltas[digit];
					} else {
						tone_a_delta = tone_deltas[digit << 1];
						tone_b_delta = tone_deltas[(digit << 1) + 1];
					}
					sample_count = tone_durations[digit];
				}
			} else {
				sample_count--;
			}
			
			// Ugly
			if ((digit >= 32) || (tone_deltas[digit] == 0)) {
				tone_sample = 0;
			} else {
				if (!dual_tone) {
					tone_sample = (sine_table_i8[(tone_a_phase & 0xFF000000U) >> 24]);
					tone_a_phase += tone_a_delta;
				} else {
					tone_sample = sine_table_i8[(tone_a_phase & 0xFF000000U) >> 24] >> 1;
					tone_sample += sine_table_i8[(tone_b_phase & 0xFF000000U) >> 24] >> 1;
					
					tone_a_phase += tone_a_delta;
					tone_b_phase += tone_b_delta;
				}
			}
		
			// FM
			delta = tone_sample * fm_delta;
			
			phase += delta;
			sphase = phase + (64 << 24);

			re = (sine_table_i8[(sphase & 0xFF000000U) >> 24]);
			im = (sine_table_i8[(phase & 0xFF000000U) >> 24]);
		}
		
		// Headphone output sample generation: 1536000/24000 = 64
		if (audio_out) {
			if (!as) {
				as = 64;
				audio_buffer.p[ai++] = tone_sample * 128;
			} else {
				as--;
			}
		}
		
		buffer.p[i] = {re, im};
	}
	
	if (audio_out) audio_output.write(audio_buffer);
}

void TonesProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const TonesConfigureMessage*>(p);
	if (message.id == Message::ID::TonesConfigure) {
		message_length = message.tone_count;
		
		if (message_length) {
			silence_count = message.pre_silence;		// In samples
			for (uint8_t c = 0; c < 32; c++) {
				tone_deltas[c] = shared_memory.bb_data.tones_data.tone_defs[c].delta;
				tone_durations[c] = shared_memory.bb_data.tones_data.tone_defs[c].duration;
			}
			fm_delta = message.fm_delta * (0xFFFFFFULL / 1536000);
			audio_out = message.audio_out;
			dual_tone = message.dual_tone;
			
			if (audio_out) audio_output.configure(false);
			
			txprogress_message.done = false;
			txprogress_message.progress = 0;
			
			digit_pos = 0;
			sample_count = 0;
			tone_a_phase = 0;
			tone_b_phase = 0;
			as = 0;
			
			configured = true;
		} else {
			configured = false;
			txprogress_message.done = true;
			shared_memory.application_queue.push(txprogress_message);
		}
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<TonesProcessor>() };
	event_dispatcher.run();
	return 0;
}
