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

#include "proc_xylos.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void XylosProcessor::execute(const buffer_c8_t& buffer) {
	
	// This is called at 1536000/2048 = 750Hz
	
	if (!configured) return;
	
	for (size_t i = 0; i<buffer.count; i++) {
		
		// Tone generation at 1536000/5 = 307.2kHz
		if (s >= (5 - 1)) {
			s = 0;
			
			if (silence) {
				// Just occupy channel with carrier
				if (sample_count >= CCIR_SILENCE) {
					silence = false;
					sample_count = samples_per_tone;
				} else {
					sample_count++;
				}
			} else {
				if (sample_count >= samples_per_tone) {
					digit = shared_memory.tx_data[byte_pos++];
					if ((digit == 0xFF) || (byte_pos >= 21)) {
						configured = false;
						message.n = 25;			// End of message code
						shared_memory.application_queue.push(message);
					} else {
						message.n = byte_pos;		// Inform UI about progress (just as eye candy)
						shared_memory.application_queue.push(message);
					}
					
					sample_count = 0;
				} else {
					sample_count++;
				}
				
				tone_phase += ccir_phases[digit];
			}
		} else {
			s++;
		}
		
		if (silence) {
			re = 0;
			im = 0;
		} else {
			tone_sample = (sine_table_i8[(tone_phase & 0x03FC0000) >> 18]);
			
			// Audio preview sample generation: 1536000/48000 = 32
			/*if (as >= 31) {
				as = 0;
				audio[ai++] = sample * 128;
			} else {
				as++;
			}*/
			
			// FM
			// 1<<18 = 262144
			// m = (262144 * BW) / 1536000 / 2
			delta = tone_sample * 853;	// 10kHz BW
			
			phase += delta;
			sphase = phase + (64 << 18);

			re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
			im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
		}
		
		buffer.p[i] = {(int8_t)re, (int8_t)im};
	}
	
	//audio_output.write(audio_buffer);
}

void XylosProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const CCIRConfigureMessage*>(p);
	if (message.id == Message::ID::CCIRConfigure) {
		byte_pos = 0;
		digit = 0;
		samples_per_tone = message.samples_per_tone;
		sample_count = samples_per_tone;
		as = 0;
		silence = true;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<XylosProcessor>() };
	event_dispatcher.run();
	return 0;
}
