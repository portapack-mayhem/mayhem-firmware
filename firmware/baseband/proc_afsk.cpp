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

#include "proc_afsk.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void AFSKProcessor::execute(const buffer_c8_t& buffer) {
	
	// This is called at 2.28M/2048 = 1113Hz
	
	if (!configured) return;
	
	for (size_t i = 0; i<buffer.count; i++) {

		if (sample_count >= afsk_samples_per_bit) {
			if (configured) {
				cur_word = *word_ptr;
				
				if (!cur_word) {
					// End of data
					if (repeat_counter < afsk_repeat) {
						// Repeat
						bit_pos = 0;
						word_ptr = (uint16_t*)shared_memory.bb_data.data;
						cur_word = *word_ptr;
						txprogress_message.done = false;
						txprogress_message.progress = repeat_counter + 1;
						shared_memory.application_queue.push(txprogress_message);
						repeat_counter++;
					} else {
						// Stop
						cur_word = 0;
						txprogress_message.done = true;
						shared_memory.application_queue.push(txprogress_message);
						configured = false;
					}
				}
			}
			
			cur_bit = (cur_word >> (symbol_count - bit_pos)) & 1;

			if (bit_pos >= symbol_count) {
				bit_pos = 0;
				word_ptr++;
			} else {
				bit_pos++;
			}
			
			sample_count = 0;
		} else {
			sample_count++;
		}
		
		if (cur_bit)
			tone_phase += afsk_phase_inc_mark;
		else
			tone_phase += afsk_phase_inc_space;

		tone_sample = sine_table_i8[(tone_phase & 0xFF000000U) >> 24];

		delta = tone_sample * fm_delta;
		
		phase += delta;
		sphase = phase + (64 << 24);

		re = (sine_table_i8[(sphase & 0xFF000000U) >> 24]);
		im = (sine_table_i8[(phase & 0xFF000000U) >> 24]);
			
		buffer.p[i] = {re, im};
	}
}

void AFSKProcessor::on_message(const Message* const msg) {
	const auto message = *reinterpret_cast<const AFSKTxConfigureMessage*>(msg);
	
	if (message.id == Message::ID::AFSKTxConfigure) {
		if (message.samples_per_bit) {
			afsk_samples_per_bit = message.samples_per_bit;
			afsk_phase_inc_mark = message.phase_inc_mark * AFSK_DELTA_COEF;
			afsk_phase_inc_space = message.phase_inc_space * AFSK_DELTA_COEF;
			afsk_repeat = message.repeat - 1;
			fm_delta = message.fm_delta * (0xFFFFFFULL / AFSK_SAMPLERATE);
			symbol_count = message.symbol_count - 1;

			sample_count = afsk_samples_per_bit;
			repeat_counter = 0;
			bit_pos = 0;
			word_ptr = (uint16_t*)shared_memory.bb_data.data;
			cur_word = 0;
			cur_bit = 0;
			configured = true;
		} else
			configured = false;		// Kill
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<AFSKProcessor>() };
	event_dispatcher.run();
	return 0;
}
