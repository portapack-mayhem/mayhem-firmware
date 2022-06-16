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

#include "proc_sstvtx.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

// This is called at 3072000/2048 = 1500Hz
void SSTVTXProcessor::execute(const buffer_c8_t& buffer) {
	
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		if (!sample_count) {
			
			// This FSM is a mess. It seems to do a lot where it shouldn't (I/Q loop),
			// but it actually doesn't do much. Used for sequencing the different parts
			// of the scanline. Todo: simplify !
			
			if (state == STATE_CALIBRATION) {
				// Once per picture
				tone_delta = calibration_sequence[substep].first;
				sample_count = calibration_sequence[substep].second;
				if (substep == 2) {
					substep = 0;
					state = STATE_VIS;
				} else
					substep++;
			} else if (state == STATE_VIS) {
				// Once per picture
				if (substep == 10) {
					current_scanline = &scanline_buffer[buffer_flip];
					buffer_flip ^= 1;
					// Ask application for a new scanline
					shared_memory.application_queue.push(sig_message);
					// Do we have to transmit a start tone ?
					if (current_scanline->start_tone.duration) {
						state = STATE_SYNC;
						tone_delta = current_scanline->start_tone.frequency;
						sample_count = current_scanline->start_tone.duration;
					} else {
						state = STATE_PIXELS;
						tone_delta = current_scanline->gap_tone.frequency;
						sample_count = current_scanline->gap_tone.duration;
					}
				} else {
					tone_delta = vis_code_sequence[substep];
					sample_count = SSTV_MS2S(30);	// A VIS code bit is 30ms
					substep++;
				}
			} else if (state == STATE_SYNC) {
				// Once per scanline, optional
				state = STATE_PIXELS;
				tone_delta = current_scanline->gap_tone.frequency;
				sample_count = current_scanline->gap_tone.duration;
			} else if (state == STATE_PIXELS) {
				// Many times per scanline
				tone_delta = SSTV_F2D(1500 + ((current_scanline->luma[pixel_index] * 800) / 256));
				sample_count = pixel_duration;
				pixel_index++;
				
				if (pixel_index >= 320) {
					// Scanline done, (dirty) state jump
					pixel_index = 0;
					state = STATE_VIS;
					substep = 10;
				}
			}
		} else {
			sample_count--;
		}

		// Tone synth
		tone_sample = (sine_table_i8[(tone_phase & 0xFF000000U) >> 24]);
		tone_phase += tone_delta;
		
		// FM
		delta = tone_sample * fm_delta;
		
		phase += delta;
		sphase = phase + (64 << 24);

		re = (sine_table_i8[(sphase & 0xFF000000U) >> 24]);
		im = (sine_table_i8[(phase & 0xFF000000U) >> 24]);
		
		buffer.p[i] = {re, im};
	}
}

void SSTVTXProcessor::on_message(const Message* const msg) {
	const auto message = *reinterpret_cast<const SSTVConfigureMessage*>(msg);
	uint8_t vis_code;
	
	switch(msg->id) {
		case Message::ID::SSTVConfigure:
			pixel_duration = message.pixel_duration;
			
			if (!pixel_duration) {
				configured = false;		// Shutdown
				return;
			}
			
			vis_code = message.vis_code;
			
			// VIS code:
			// 1200, (0=1300, 1=1100), 1200
			vis_code_sequence[0] = SSTV_VIS_SS;
			for (uint32_t c = 0; c < 8; c++)
				vis_code_sequence[c + 1] = ((vis_code >> c) & 1) ? SSTV_VIS_ONE : SSTV_VIS_ZERO;
			vis_code_sequence[9] = SSTV_VIS_SS;
			
			fm_delta = 9000 * (0xFFFFFFULL / 3072000);	// Fixed bw for now
			
			pixel_index = 0;
			sample_count = 0;
			tone_phase = 0;
			state = STATE_CALIBRATION;
			substep = 0;
			
			configured = true;
			break;
		
		case Message::ID::FIFOData:
			memcpy(&scanline_buffer[buffer_flip], static_cast<const FIFODataMessage*>(msg)->data, sizeof(sstv_scanline));
			break;

		default:
			break;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<SSTVTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
