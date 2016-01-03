/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

void XylosProcessor::execute(buffer_c8_t buffer) {
	
	for (size_t i = 0; i<buffer.count; i++) {
		//Sample generation 2.28M/10 = 228kHz
		if (s >= 9) {
			s = 0;
			
			if (sample_count >= CCIR_TONELENGTH) {
				if (shared_memory.xylos_transmit_done == false) {
					message.n = byte_pos;	// Progress
					shared_memory.application_queue.push(message);
					digit = shared_memory.xylosdata[byte_pos++];
				}
					
				if (!digit) {
					message.n = 25;	// Done code
					shared_memory.xylos_transmit_done = true;
					shared_memory.application_queue.push(message);
					digit = 0;
				}

				if (digit > '9') digit -= 7;
				digit -= 0x30;
				
				sample_count = 0;
			} else {
				sample_count++;
			}
			
			aphase += ccir_phases[digit];
		} else {
			s++;
		}
		
		sample = (sine_table_f32[(aphase & 0x03FF0000)>>18]*255); 
		
		//FM
		frq = sample * 160; // 20kHz wide (?)
		
		phase = (phase + frq);
		sphase = phase + (256<<16);

		re = (sine_table_f32[(sphase & 0x03FF0000)>>18]*127);
		im = (sine_table_f32[(phase & 0x03FF0000)>>18]*127);
		
		buffer.p[i] = {(int8_t)re,(int8_t)im};
	}
}
