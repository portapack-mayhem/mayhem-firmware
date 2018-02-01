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

#include "proc_adsbrx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>
#include <cstddef>

using namespace adsb;
	
void ADSBRXProcessor::execute(const buffer_c8_t& buffer) {
	int8_t re, im;
	float mag;
	uint32_t c;
	uint8_t level, bit, byte { };
	//bool confidence;
	bool first_in_window, last_in_window;
	
	// This is called at 2M/2048 = 977Hz
	// One pulse = 500ns = 2 samples
	// One bit = 2 pulses = 1us = 4 samples
	
	if (!configured) return;
	
	for (size_t i = 0; i < buffer.count; i++) {
		
		// Compute sample's magnitude
		re = buffer.p[i].real();
		im = buffer.p[i].imag();
		mag = __builtin_sqrtf((re * re) + (im * im)) * k;
		
		// Only used for preamble detection and visualisation
		level = (mag < 0.3) ? 0 :		// Blank weak signals
					(mag > prev_mag) ? 1 : 0;
		
		if (decoding) {
			// Decode
			
			// 1 bit lasts 2 samples
			if (sample_count & 1) {
				if ((prev_mag < threshold_low) && (mag < threshold_low)) {
					// Both under window, silence.
					if (null_count > 3) {
						const ADSBFrameMessage message(frame);
						shared_memory.application_queue.push(message);
							
						decoding = false;
					} else
						null_count++;
						
					//confidence = false;
					if (prev_mag > mag)
						bit = 1;
					else
						bit = 0;
					
				} else {
					
					null_count = 0;
				
					first_in_window = ((prev_mag >= threshold_low) && (prev_mag <= threshold_high));
					last_in_window = ((mag >= threshold_low) && (mag <= threshold_high));
					
					if ((first_in_window && !last_in_window) || (!first_in_window && last_in_window)) {
						//confidence = true;
						if (prev_mag > mag)
							bit = 1;
						else
							bit = 0;
					} else {
						//confidence = false;
						if (prev_mag > mag)
							bit = 1;
						else
							bit = 0;
					}
				}
				
				byte = bit | (byte << 1);
				bit_count++;
				if (!(bit_count & 7)) {
					// Got one byte
					frame.push_byte(byte);
				}
			}
			sample_count++;
		} else {
			// Look for preamble
			
			// Shift
			for (c = 0; c < (ADSB_PREAMBLE_LENGTH - 1); c++)
				shifter[c] = shifter[c + 1];
			shifter[15] = std::make_pair(mag, level);
			
			// Compare
			for (c = 0; c < ADSB_PREAMBLE_LENGTH; c++) {
				if (shifter[c].second != adsb_preamble[c])
					break;
			}
				
			if (c == ADSB_PREAMBLE_LENGTH) {
				decoding = true;
				sample_count = 0;
				null_count = 0;
				bit_count = 0;
				frame.clear();
				
				// Compute preamble pulses power to set thresholds
				threshold = (shifter[0].first + shifter[2].first + shifter[7].first + shifter[9].first) / 4;
				threshold_high = threshold * 1.414;		// +3dB
				threshold_low = threshold * 0.707;		// -3dB
			}
		}
		
		prev_mag = mag;
	}
}

void ADSBRXProcessor::on_message(const Message* const message) {
	if (message->id == Message::ID::ADSBConfigure) {
		null_count = 0;
		bit_count = 0;
		sample_count = 0;
		decoding = false;
		configured = true;
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ADSBRXProcessor>() };
	event_dispatcher.run();
	return 0;
}
