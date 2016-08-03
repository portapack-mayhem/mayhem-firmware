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

#include "proc_ook.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void OOKProcessor::execute(const buffer_c8_t& buffer) {
	
	// This is called at 2.28M/2048 = 1113Hz
	
	if (!configured) return;
	
	for (size_t i = 0; i<buffer.count; i++) {
		
		// Synthesis at 2.28M/2 = 1.14MHz
		if (!s) {
			s = 2 - 1;
			if (sample_count >= samples_per_bit) {
				if (configured) {
					cur_bit = (bitstream[bit_pos >> 3] << (bit_pos & 7)) & 0x80;
					
					bit_pos++;
					
					if (bit_pos >= stream_size) {
						// End of data
						if (repeat_counter < repeat) {
							// Repeat
							bit_pos = 0;
							cur_bit = bitstream[0] & 0x80;
							message.n = repeat_counter + 1;
							shared_memory.application_queue.push(message);
							repeat_counter++;
						} else {
							// Stop
							cur_bit = 0;
							message.n = 0;
							shared_memory.application_queue.push(message);
							configured = false;
						}
					}
				}
				
				sample_count = 0;
			} else {
				sample_count++;
			}
		} else {
			s--;
		}
		
		if (cur_bit) phase += 100;		// ?
		
		sphase = phase + (64 << 18);

		re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
		im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
		
		buffer.p[i] = {(int8_t)re, (int8_t)im};
	}
}

void OOKProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const OOKConfigureMessage*>(p);
	
	if (message.id == Message::ID::OOKConfigure) {
		//memcpy(bitstream, message.ook_bitstream, 64);
		
		samples_per_bit = message.samples_per_bit;
		stream_size = message.stream_length;
		repeat = message.repeat - 1;
	
		s = 0;
		sample_count = samples_per_bit;
		repeat_counter = 0;
		bit_pos = 0;
		cur_bit = 0;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<OOKProcessor>() };
	event_dispatcher.run();
	return 0;
}
