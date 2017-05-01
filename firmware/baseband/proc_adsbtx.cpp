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
	
	// This is called at 4M/2048 = 1953Hz
	// One pulse = 500ns = 2 samples
	// One bit = 2 pulses = 1us = 4 samples
	
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		if (!sample) {
			sample = 3;
			
			if (active) {
				if (preamble) {
					if (bit_pos >= 16) {
						preamble = false;
						bit_pos = 0;
					} else {
						cur_bit = (preamble_parts << bit_pos) >> 15;
						bit_pos++;
					}
				}
				
				if (!preamble) {
					if (bit_pos >= 112) {
						active = false;	// Stop
						cur_bit = 0;
					} else {
						cur_bit = shared_memory.bb_data.data[bit_pos];
						bit_pos++;
					}
				}
			} else {
				//cur_bit = 0;
				if (bit_pos == 8192) {	// ?
					configured = false;
					message.done = true;
					shared_memory.application_queue.push(message);
				}
				bit_pos++;
			}
		} else
			sample--;
		
		if (sample == 1)
			cur_bit ^= 1;	// Invert
		
		delta = tone_sample * fm_delta;
		tone_sample += 128;
		
		if (cur_bit) {
			phase += delta;
			sphase = phase + (64 << 24);

			re = (sine_table_i8[(sphase & 0xFF000000U) >> 24]);
			im = (sine_table_i8[(phase & 0xFF000000U) >> 24]);
		} else {
			re = 0;
			im = 0;
		}
		
		buffer.p[i] = {re, im};
	}
}

void ADSBTXProcessor::on_message(const Message* const p) {
	const auto message = *reinterpret_cast<const ADSBConfigureMessage*>(p);
	
	if (message.id == Message::ID::ADSBConfigure) {
		bit_pos = 0;
		sample = 0;
		cur_bit = 0;
		preamble = true;
		active = true;
		configured = true;
		
		fm_delta = 50000 * (0xFFFFFFULL / 4000000);	// Fixed bw for now
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ADSBTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
