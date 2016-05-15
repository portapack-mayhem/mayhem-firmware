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

#include "proc_epar.hpp"

#include "dsp_iir_config.hpp"
#include "audio_output.hpp"

#include "portapack_shared_memory.hpp"
#include "sine_table.hpp"

#include <cstdint>

void EPARProcessor::execute(const buffer_c8_t& buffer) {
	
	// This is called at 1536000/2048 = 750Hz
	
	for (size_t i = 0; i<buffer.count; i++) {
		
		// Sample generation rate: 1536000/10 = 153kHz
		if (s >= 9) {
			s = 0;
			
			if (sample_count >= EPAR_TU) {
				
				if (state) {
					// Send code
					if (current_tu == 2) {
						current_bit = shared_memory.epardata[bit_pos];
						if (bit_pos == 12) {
							bit_pos = 0;
							current_tu = 0;
							state = 0;
						} else {
							bit_pos++;
						}

						current_tu = 0;
					} else {
						current_tu++;
					}
					
					sample = bitdef[current_bit][current_tu];
				} else {
					// Pause
					if (current_tu == EPAR_SPACE) {
						if (repeat_count == EPAR_REPEAT) {
							message.n = 100;			// End of transmission code
							shared_memory.transmit_done = true;
							shared_memory.application_queue.push(message);
						}
						if (shared_memory.transmit_done == false) {
							message.n = repeat_count;	// Inform UI about progress (just as eye candy)
							shared_memory.application_queue.push(message);
							state = 1;
						}
						repeat_count++;
						current_tu = 0;
					} else {
						current_tu++;
					}
					sample = -127;
				}
				
				sample_count = 0;
			} else {
				sample_count++;
			}
			
		} else {
			s++;
		}
		
		//FM
		frq = sample * shared_memory.excursion; // 500=~3kHz wide
		
		phase = (phase + frq);
		sphase = phase + (256<<16);

		re = (sine_table_f32[(sphase & 0x03FF0000)>>18]*127);
		im = (sine_table_f32[(phase & 0x03FF0000)>>18]*127);
		
		buffer.p[i] = {(int8_t)re,(int8_t)im};
	}
}
