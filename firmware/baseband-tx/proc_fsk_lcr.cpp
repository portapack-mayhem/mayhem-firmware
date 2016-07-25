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

#include "proc_fsk_lcr.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table.hpp"

#include <cstdint>

void LCRFSKProcessor::execute(const buffer_c8_t& buffer) {
	
	for (size_t i = 0; i<buffer.count; i++) {
		
		//Sample generation 2.28M/10 = 228kHz
		if (s >= 9) {
			s = 0;
			
			if (sample_count >= shared_memory.afsk_samples_per_bit) {
				if (shared_memory.afsk_transmit_done == false) {
					cur_byte = shared_memory.radio_data[byte_pos];
					ext_byte = shared_memory.radio_data[byte_pos + 1];
				}
				if (!cur_byte) {
					if (shared_memory.afsk_repeat) {
						shared_memory.afsk_repeat--;
						bit_pos = 0;
						byte_pos = 0;
						cur_byte = shared_memory.radio_data[0];
						ext_byte = shared_memory.radio_data[1];
						message.n = shared_memory.afsk_repeat;
						shared_memory.application_queue.push(message);
					} else {
						message.n = 0;
						shared_memory.afsk_transmit_done = true;
						shared_memory.application_queue.push(message);
						cur_byte = 0;
						ext_byte = 0;
					}
				}
				
				if (shared_memory.afsk_alt_format) {
					// 0bbbbbbbbp
					// Start, 8-bit data, parity
					gbyte = 0;
					gbyte = cur_byte << 1;
					gbyte |= (ext_byte & 1);
				} else {
					// 0bbbbbbbp1
					// Start, 7-bit data, parity, stop
					gbyte = 0;
					gbyte = cur_byte << 1;
					gbyte |= 1;
				}
				
				cur_bit = (gbyte >> (9 - bit_pos)) & 1;

				if (bit_pos == 9) {
					bit_pos = 0;
					if (!shared_memory.afsk_alt_format)
						byte_pos++;
					else
						byte_pos += 2;
				} else {
					bit_pos++;
				}
				
				sample_count = 0;
			} else {
				sample_count++;
			}
			if (cur_bit)
				aphase += shared_memory.afsk_phase_inc_mark;
			else
				aphase += shared_memory.afsk_phase_inc_space;
		} else {
			s++;
		}
		
		sample = (sine_table_f32[(aphase & 0x03FF0000)>>18]*255); 
		
		//FM
		frq = sample * shared_memory.afsk_fmmod;
		
		phase = (phase + frq);
		sphase = phase + (256<<16);

		re = (sine_table_f32[(sphase & 0x03FF0000)>>18]*127);
		im = (sine_table_f32[(phase & 0x03FF0000)>>18]*127);
		
		buffer.p[i] = {(int8_t)re,(int8_t)im};
	}
}
