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

void JammerProcessor::execute(const buffer_c8_t& buffer) {
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {

		if (!jammer_duration) {
			// Find next enabled range
			for (ir = 0; ir < 9; ir++) {
				current_range++;
				if (current_range == 9) current_range = 0;
				if (jammer_ranges[current_range].enabled)
					break;
			}
			
			jammer_duration = jammer_ranges[current_range].duration;
			jammer_bw = jammer_ranges[current_range].width / 6;		// TODO: Exact value
			
			// Ask for retune
			message.freq = jammer_ranges[current_range].center;
			message.range = current_range;
			shared_memory.application_queue.push(message);
		} else {
			jammer_duration--;
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
		
		// Phase noise
		if (r >= 70) {
			aphase += ((aphase>>4) ^ 0x4573) << 14;
			r = 0;
		} else {
			r++;
		}
		
		aphase += 8830;
		sample = sine_table_i8[(aphase & 0x03FC0000) >> 18];
		
		// FM
		delta = sample * jammer_bw;
		
		phase += delta;
		sphase = phase + (64 << 18);

		re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
		im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);

		buffer.p[i] = {(int8_t)re, (int8_t)im};
		//buffer.p[i] = {re, im};
	}
};

void JammerProcessor::on_message(const Message* const msg) {
	const auto message = *reinterpret_cast<const JammerConfigureMessage*>(msg);
	
	if (message.id == Message::ID::JammerConfigure) {
		jammer_ranges = (JammerRange*)shared_memory.bb_data.data;
		jammer_duration = 0;
		current_range = -1;
		
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<JammerProcessor>() };
	event_dispatcher.run();
	return 0;
}
