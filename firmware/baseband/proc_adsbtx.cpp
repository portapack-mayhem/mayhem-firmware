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

#include "proc_adsbtx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void ADSBTXProcessor::execute(const buffer_c8_t& buffer) {
	
	// This is called at 2M/2048 = 977Hz
	
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		// Synthesis at 2M
		/*if (preamble == true) {
			if (bit_pos >= 16) {
				bit_pos = 0;
				preamble = false;
				bit_part = 0;
			} else {
				cur_bit = (preamble_parts << bit_pos) >> 15;
				bit_pos++;
			}
		}*/
		//if (preamble == false) {
			if (!bit_part) {
				if (bit_pos >= 112) {
					// Stop
					message.n = 200;
					shared_memory.application_queue.push(message);
					configured = false;
					cur_bit = 0;
				} else {
					cur_bit = 0; //shared_memory.tx_data[bit_pos];
					bit_pos++;
					bit_part = 1;
				}
			} else {
				bit_part = 0;
				cur_bit ^= 1;
			}
		//}
		
		// 8D48: 10001101 01001000
		//       1001010110100110 0110010110010101
		
		if (cur_bit) {
			phase = (phase + 0x1FE00);			// What ?
			sphase = phase + (64 << 18);

			re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
			im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
		} else {
			re = 0;
			im = 0;
		}
	
		buffer.p[i] = {(int8_t)re, (int8_t)im};
	}
}

void ADSBTXProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const ADSBConfigureMessage*>(p);
	
	if (message.id == Message::ID::ADSBConfigure) {
		bit_part = 0;
		bit_pos = 0;
		cur_bit = 0;
		preamble = true;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ADSBTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
