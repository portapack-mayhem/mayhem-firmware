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

	//ai = 0;
	for (size_t i = 0; i<buffer.count; i++) {
		
		// Audio preview sample generation: 1536000/divider = samplerate
		if (!as) {
			as = divider;
			audio_fifo.out(sample);
			//preview_audio_buffer.p[ai++] = sample << 8;
			
			if (ctcss_enabled) {
				ctcss_sample = sine_table_i8[(ctcss_phase & 0x03FC0000) >> 18];
				int16_t mix = (sample * 218) + (ctcss_sample * 37);		// ~15%
				sample = mix >> 8;
			}
			
			if ((audio_fifo.len() < 1024) && (asked == false)) {
				// Ask application to fill up fifo
				sigmessage.signaltype = 1;
				shared_memory.application_queue.push(sigmessage);
				asked = true;
			}
		} else {
			as--;
		}
		
		ctcss_phase += ctcss_phase_inc;
		
		// FM
		frq = sample * bw;
		
		phase = (phase + frq);
		sphase = phase + (64 << 18);

		re = (sine_table_i8[(sphase & 0x03FC0000) >> 18]);
		im = (sine_table_i8[(phase & 0x03FC0000) >> 18]);
		
		buffer.p[i] = {(int8_t)re, (int8_t)im};
	}
	
	//AudioOutput::fill_audio_buffer(preview_audio_buffer, true);
}

void AudioTXProcessor::on_message(const Message* const msg) {
	const auto message = static_cast<const AudioTXConfigMessage*>(msg);
	
	switch(msg->id) {
		case Message::ID::AudioTXConfig:

			// 1<<18 = 262144
			// m = (262144 * a) / 1536000
			// a = 262144 / 1536000 (*1000 = 171)
			bw = 171 * (message->bw);
			divider = message->divider;
			ctcss_phase_inc = message->ctcss_phase_inc;
			ctcss_enabled = message->ctcss_enabled;
			as = 0;

			configured = true;
			break;
		
		case Message::ID::FIFOData:
			audio_fifo.in(static_cast<const FIFODataMessage*>(msg)->data, 1024);
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
