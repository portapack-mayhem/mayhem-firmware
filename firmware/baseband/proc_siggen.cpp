/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "proc_siggen.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void SigGenProcessor::execute(const buffer_c8_t& buffer) {
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		if (!sample_count && auto_off) {
			txprogress_message.done = true;
			shared_memory.application_queue.push(txprogress_message);
		} else
			sample_count--;
		
		if (tone_shape == 0) {
			// CW
			re = 0;
			im = 0;
		} else {
			
			if (tone_shape == 1) {
				// Sine
				sample = (sine_table_i8[(tone_phase & 0xFF000000) >> 24]);
			} else if (tone_shape == 2) {
				// Tri
				int8_t a = (tone_phase & 0xFF000000) >> 24;
				sample = (a & 0x80) ? ((a << 1) ^ 0xFF) - 0x80 : (a << 1) + 0x80;
			} else if (tone_shape == 3) {
				// Saw up
				sample = ((tone_phase & 0xFF000000) >> 24);
			} else if (tone_shape == 4) {
				// Saw down
				sample = ((tone_phase & 0xFF000000) >> 24) ^ 0xFF;
			} else if (tone_shape == 5) {
				// Square
				sample = (((tone_phase & 0xFF000000) >> 24) & 0x80) ? 127 : -128;
			} else if (tone_shape == 6) {
				// Noise
				sample = (lfsr & 0xFF000000) >> 24;
				feedback = ((lfsr >> 31) ^ (lfsr >> 29) ^ (lfsr >> 15) ^ (lfsr >> 11)) & 1;
				lfsr = (lfsr << 1) | feedback;
				if (!lfsr) lfsr = 0x1337;				// Shouldn't do this :(
			}
			
			tone_phase += tone_delta;
			
			// Do FM
			delta = sample * fm_delta;
			
			phase += delta;
			sphase = phase + (64 << 24);

			re = (sine_table_i8[(sphase & 0xFF000000) >> 24]);
			im = (sine_table_i8[(phase & 0xFF000000) >> 24]);
		}

		buffer.p[i] = {re, im};
	}
};

void SigGenProcessor::on_message(const Message* const msg) {
	const auto message = *reinterpret_cast<const SigGenConfigMessage*>(msg);
	
	switch(msg->id) {
		case Message::ID::SigGenConfig:
			if (!message.bw) {
				configured = false;
				return;
			}
			
			if (message.duration) {
				sample_count = message.duration;
				auto_off = true;
			} else
				auto_off = false;
			
			fm_delta = message.bw * (0xFFFFFFULL / 1536000);
			tone_shape = message.shape;
			
			lfsr = 0x54DF0119;

			configured = true;
			break;
		
		case Message::ID::SigGenTone:
			tone_delta = reinterpret_cast<const SigGenToneMessage*>(msg)->tone_delta;
			break;

		default:
			break;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<SigGenProcessor>() };
	event_dispatcher.run();
	return 0;
}
