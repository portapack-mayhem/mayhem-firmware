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

#include "proc_playaudio.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table.hpp"

#include <cstdint>

// This is diry :(
void PlayAudioProcessor::fill_buffer(int8_t * inptr) {
	memcpy(&audio_fifo[fifo_put], inptr, 1024);
	fifo_put = (fifo_put + 1024) & 0x0FFF;
	asked = false;
}

void PlayAudioProcessor::execute(const buffer_c8_t& buffer){

	// This is called at 1536000/2048 = 750Hz

	ai = 0;
	for (size_t i = 0; i<buffer.count; i++) {
		// Audio preview sample generation: 1536000/48000 = 32
		if (as >= 31) {
			as = 0;
			sample = audio_fifo[fifo_get];
			preview_audio_buffer.p[ai++] = sample << 8;
			fifo_get = (fifo_get + 1) & 0x0FFF;
			if ((((fifo_put - fifo_get) & 0x0FFF) < 3072) && (asked == false)) {
				// Ask application to fill up fifo
				sigmessage.signaltype = 1;
				shared_memory.application_queue.push(sigmessage);
				asked = true;
			}
		} else {
			as++;
		}
		
		sample = ((int8_t)audio_fifo[fifo_get] * 4);

		//FM
		frq = sample * 2000;
		
		phase = (phase + frq);
		sphase = phase + (256<<16);

		re = (sine_table_f32[(sphase & 0x03FF0000)>>18]*127);
		im = (sine_table_f32[(phase & 0x03FF0000)>>18]*127);
		
		buffer.p[i] = {(int8_t)re,(int8_t)im};
	}
	
	//fill_audio_buffer(preview_audio_buffer);
}
