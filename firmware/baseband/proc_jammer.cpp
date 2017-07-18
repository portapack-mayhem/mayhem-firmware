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
			do {
				current_range++;
				if (current_range == JAMMER_MAX_CH) current_range = 0;
			} while (!jammer_channels[current_range].enabled);
			
			jammer_duration = jammer_channels[current_range].duration;
			jammer_bw = jammer_channels[current_range].width / 2;		// TODO: Exact value
			
			// Ask for retune
			message.freq = jammer_channels[current_range].center;
			message.range = current_range;
			shared_memory.application_queue.push(message);
		} else {
			jammer_duration--;
		}
		
		// Phase noise
		if (!period_counter) {
			period_counter = noise_period;
			
			if (noise_type == JammerType::TYPE_FSK) {
				sample = (sample + lfsr) >> 1;
			} else if (noise_type == JammerType::TYPE_TONE) {
				tone_delta = 150000 + (lfsr >> 9);	// Approx 100Hz to 6kHz
			} else if (noise_type == JammerType::TYPE_SWEEP) {
				sample++;							// This is like saw wave FM
			}
			
			feedback = ((lfsr >> 31) ^ (lfsr >> 29) ^ (lfsr >> 15) ^ (lfsr >> 11)) & 1;
			lfsr = (lfsr << 1) | feedback;
			if (!lfsr) lfsr = 0x1337;				// Shouldn't do this :(
		} else {
			period_counter--;
		}
		
		if (noise_type == JammerType::TYPE_TONE) {
			aphase += tone_delta;
			sample = sine_table_i8[(aphase & 0xFF000000) >> 24];
		}
		
		delta = sample * jammer_bw;
		
		phase += delta;
		sphase = phase + (64 << 24);

		re = (sine_table_i8[(sphase & 0xFF000000) >> 24]);
		im = (sine_table_i8[(phase & 0xFF000000) >> 24]);

		buffer.p[i] = {re, im};
	}
};

void JammerProcessor::on_message(const Message* const msg) {
	if (msg->id == Message::ID::JammerConfigure) {
		const auto message = *reinterpret_cast<const JammerConfigureMessage*>(msg);
		
		if (message.run) {
			jammer_channels = (JammerChannel*)shared_memory.bb_data.data;
			noise_type = message.type;
			noise_period = 3072000 / message.speed;
			if (noise_type == JammerType::TYPE_SWEEP)
				noise_period >>= 8;
			period_counter = 0;
			jammer_duration = 0;
			current_range = 0;
			lfsr = 0xDEAD0012;
			
			configured = true;
		} else {
			configured = false;
		}
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<JammerProcessor>() };
	event_dispatcher.run();
	return 0;
}
