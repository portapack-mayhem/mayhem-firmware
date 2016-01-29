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
#include "sine_table.hpp"

#include <cstdint>

// 1990 1131 1201 1275 1357 ...

// 14 13 12 11:
// 2108 989 2259 931

void XylosProcessor::execute(buffer_c8_t buffer) {
	
	// This is called at 1536000/2048 = 750Hz
	
	ai = 0;
	
	for (size_t i = 0; i<buffer.count; i++) {
		
		// Sample generation rate: 1536000/10 = 153kHz
		if (s >= 9) {
			s = 0;
			
			if (sample_count >= CCIR_TONELENGTH) {
				if (shared_memory.xylos_transmit_done == false) {
					message.n = byte_pos;	// Inform UI about progress (just as eye candy)
					shared_memory.application_queue.push(message);
					digit = shared_memory.xylosdata[byte_pos++];
				}
					
				if (digit == 0xFF) {
					message.n = 25;	// End of message code
					shared_memory.xylos_transmit_done = true;
					shared_memory.application_queue.push(message);
				}
				
				sample_count = 0;
			} else {
				sample_count++;
			}
			
			aphase += ccir_phases[digit];
		} else {
			s++;
		}
		
		sample = (sine_table_f32[(aphase & 0x03FF0000)>>18]*255);
		
		// Audio preview sample generation: 1536000/48000 = 32
		if (as >= 31) {
			as = 0;
			preview_audio_buffer.p[ai++] = sample * 128;
		} else {
			as++;
		}
		
		//FM
		frq = sample * 300; // ~10kHz wide
		
		phase = (phase + frq);
		sphase = phase + (256<<16);

		re = (sine_table_f32[(sphase & 0x03FF0000)>>18]*127);
		im = (sine_table_f32[(phase & 0x03FF0000)>>18]*127);
		
		buffer.p[i] = {(int8_t)re,(int8_t)im};
	}
	
	fill_audio_buffer(preview_audio_buffer);
}
