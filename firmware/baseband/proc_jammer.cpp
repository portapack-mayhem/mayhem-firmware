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

#include "proc_jammer.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table.hpp"
#include "event_m4.hpp"

#include <cstdint>

#define POLY_MASK_32 0xB4BCD35C

void JammerProcessor::execute(const buffer_c8_t& buffer) {
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
		sample = sine_table_f32[(aphase & 0x03FF0000)>>18]*256;
		
		//FM
		frq = sample * jammer_bw;		// Bandwidth
		
		phase = (phase + frq);
		sphase = phase + (256<<16);

		re = sine_table_f32[(sphase & 0x03FF0000)>>18]*127;
		im = sine_table_f32[(phase & 0x03FF0000)>>18]*127;
		
		buffer.p[i] = {(int8_t)re,(int8_t)im};
	}
};

int main() {
	EventDispatcher event_dispatcher { std::make_unique<JammerProcessor>() };
	event_dispatcher.run();
	return 0;
}
