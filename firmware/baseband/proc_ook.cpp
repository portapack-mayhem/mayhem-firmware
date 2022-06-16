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
	int8_t re, im;
	
	// This is called at 2.28M/2048 = 1113Hz
	
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		// Synthesis at 2.28M/10 = 228kHz
		if (!s) {
			s = 10 - 1;
			if (sample_count >= samples_per_bit) {
				if (configured) {
					if (bit_pos >= length) {
						// End of data
						if (pause_counter == 0) {
							pause_counter = pause;
							cur_bit = 0;
						} else if (pause_counter == 1) {
							if (repeat_counter < repeat) {
								// Repeat
								bit_pos = 0;
								cur_bit = shared_memory.bb_data.data[0] & 0x80;
								txprogress_message.progress = repeat_counter + 1;
								txprogress_message.done = false;
								shared_memory.application_queue.push(txprogress_message);
								repeat_counter++;
							} else {
								// Stop
								cur_bit = 0;
								txprogress_message.done = true;
								shared_memory.application_queue.push(txprogress_message);
								configured = false;
							}
							pause_counter = 0;
						} else {
							pause_counter--;
						}
					} else {
						cur_bit = (shared_memory.bb_data.data[bit_pos >> 3] << (bit_pos & 7)) & 0x80;
						bit_pos++;
					}
				}
				
				sample_count = 0;
			} else {
				sample_count++;
			}
		} else {
			s--;
		}
		
		if (cur_bit) {
			phase = (phase + 200);			// What ?
			sphase = phase + (64 << 18);

			re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
			im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
		} else {
			re = 0;
			im = 0;
		}
	
		buffer.p[i] = {re, im};
	}
}

void OOKProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const OOKConfigureMessage*>(p);
	
	if (message.id == Message::ID::OOKConfigure) {
		samples_per_bit = message.samples_per_bit / 10;
		repeat = message.repeat - 1;
		length = message.stream_length;
		pause = message.pause_symbols + 1;
	
		pause_counter = 0;
		s = 0;
		sample_count = samples_per_bit;
		repeat_counter = 0;
		bit_pos = 0;
		cur_bit = 0;
		txprogress_message.progress = 0;
		txprogress_message.done = false;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<OOKProcessor>() };
	event_dispatcher.run();
	return 0;
}
