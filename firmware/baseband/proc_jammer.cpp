/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_jammer.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

#define POLY_MASK_32 0xB4BCD35C

void JammerProcessor::execute(const buffer_c8_t& buffer) {
	
	if (!configured) return;
	
	for (size_t i = 0; i<buffer.count; i++) {

		if (s >= 10000) {	//shared_memory.jammer_ranges[ir].duration
			s = 0;
			for (;;) {
				ir++;
				if (ir > 15) ir = 0;
				if (shared_memory.jammer_ranges[ir].active == true) break;
			}
			jammer_bw = shared_memory.jammer_ranges[ir].width / 2;
			
			message.freq = shared_memory.jammer_ranges[ir].center;
			shared_memory.application_queue.push(message);
		} else {
			s++;
		}
		
		// Ramp
		/*if (r >= 10) {
			if (sample < 128)
				sample++;
			else
				sample = -127;
			r = 0;
		} else {
			r++;
		}*/
		
		// Phase
		if (r >= 70) {
			aphase += ((aphase>>4) ^ 0x4573) << 14;
			r = 0;
		} else {
			r++;
		}
		
		aphase += 8830;
		sample = sine_table_i8[(sphase & 0x03FC0000) >> 18];
		
		// FM
		frq = sample * jammer_bw;
		
		phase = (phase + frq);
		sphase = phase + (64 << 18);

		re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
		im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
		
		buffer.p[i] = {(int8_t)re, (int8_t)im};
	}
};

void JammerProcessor::on_message(const Message* const msg) {
	/*const auto message = *reinterpret_cast<const DTMFTXConfigMessage*>(msg);
	
	if (message.id == Message::ID::DTMFTXConfig) {
		
		// Translate DTMF message to index in DTMF frequencies table
		tone_ptr = &shared_memory.tx_data[0];
		for (;;) {
			tone_code = *tone_ptr;
			if (tone_code == 0xFF)
				break;				// End of message
			else if (tone_code <= 9)
				// That's already fine bro.
				*tone_ptr = tone_code;
			else if (tone_code == 'A')
				*tone_ptr = 10;
			else if (tone_code == 'B')
				*tone_ptr = 11;
			else if (tone_code == 'C')
				*tone_ptr = 12;
			else if (tone_code == 'D')
				*tone_ptr = 13;
			else if (tone_code == '#')
				*tone_ptr = 14;
			else if (tone_code == '*')
				*tone_ptr = 15;
			else {
				*tone_ptr = 0xFF;	// Invalid character, stop here
			}
			tone_ptr++;
		}
		
		// 1<<18 = 262144
		// m = (262144 * a) / 1536000
		// a = 262144 / 1536000 (*1000 = 171)
		bw = 171 * (message.bw);
		tone_length = message.tone_length * 154;		// 153.6
		pause_length = message.pause_length * 154;		// 153.6
		as = 0;
		tone = false;
		timer = 0;
		tone_idx = 0;

		configured = true;
	}*/
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<JammerProcessor>() };
	event_dispatcher.run();
	return 0;
}
