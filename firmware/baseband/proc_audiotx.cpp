/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_audiotx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
//#include "audio_output.hpp"
#include "event_m4.hpp"

#include <cstdint>

void AudioTXProcessor::execute(const buffer_c8_t& buffer){

	// This is called at 1536000/2048 = 750Hz
	
	if (!configured) return;

	for (size_t i = 0; i<buffer.count; i++) {
		
		// Audio preview sample generation @ 1536000/divider
		if (!as) {
			as = divider;
			audio_fifo.out(out_sample);
			sample = (int32_t)out_sample;
			//preview_audio_buffer.p[ai++] = sample << 8;
			
			if ((audio_fifo.len() < 512) && (asked == false)) {
				// Ask application to fill up fifo
				shared_memory.application_queue.push(sig_message);
				asked = true;
			}
		} else {
			as--;
		}
		
		sample = tone_gen.process(sample);
		
		// FM
		delta = sample * fm_delta;
		
		phase += delta;
		sphase = phase + (64 << 24);

		re = (sine_table_i8[(sphase & 0xFF000000U) >> 24]);
		im = (sine_table_i8[(phase & 0xFF000000U) >> 24]);
		
		buffer.p[i] = {re, im};
	}
	
	//AudioOutput::fill_audio_buffer(preview_audio_buffer, true);
}

void AudioTXProcessor::on_message(const Message* const msg) {
	const auto message = *reinterpret_cast<const AudioTXConfigMessage*>(msg);
	
	switch(msg->id) {
		case Message::ID::AudioTXConfig:
			fm_delta = message.fm_delta * (0xFFFFFFULL / 1536000);
			divider = message.divider;
			as = 0;
			
			tone_gen.configure(message.tone_key_delta, message.tone_key_mix_weight);

			configured = true;
			break;
		
		case Message::ID::FIFOData:
			audio_fifo.in(static_cast<const FIFODataMessage*>(msg)->data, 512);
			asked = false;
			break;

		default:
			break;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<AudioTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
