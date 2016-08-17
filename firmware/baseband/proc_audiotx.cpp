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
#include "sine_table.hpp"
//#include "audio_output.hpp"
#include "event_m4.hpp"

#include <cstdint>

void AudioTXProcessor::execute(const buffer_c8_t& buffer){

	// This is called at 1536000/2048 = 750Hz
	
	if (!configured) return;

	ai = 0;
	for (size_t i = 0; i<buffer.count; i++) {
		
		//audio_fifo.out(&sample, 1);
		
		// Audio preview sample generation: 1536000/48000 = 32
		if (as >= 31) {
			as = 0;
			//audio_fifo.out(&sample, 1);
			//preview_audio_buffer.p[ai++] = sample << 8;
			
			//if ((audio_fifo.len() < 1024) && (asked == false)) {
				// Ask application to fill up fifo
				sigmessage.signaltype = 1;
				shared_memory.application_queue.push(sigmessage);
				asked = true;
			//}
		} else {
			as++;
		}
		

		// FM
		frq = (int32_t)(sample) * 4 * 2000;
		
		phase = (phase + frq);
		sphase = phase + (256 << 16);

		re = (sine_table_f32[(sphase & 0x03FF0000)>>18]*127);
		im = (sine_table_f32[(phase & 0x03FF0000)>>18]*127);
		
		buffer.p[i] = { (int8_t)re, (int8_t)im };
	}
	
	//AudioOutput::fill_audio_buffer(preview_audio_buffer, true);
}

void AudioTXProcessor::on_message(const Message* const msg) {
	switch(msg->id) {
		case Message::ID::AudioTXConfig:
			//const auto message = static_cast<const AudioTXConfigMessage*>(msg);
			configured = true;
			break;
		
		case Message::ID::FIFOData:
			//audio_fifo.in(static_cast<const FIFODataMessage*>(msg)->data, 1024);
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
