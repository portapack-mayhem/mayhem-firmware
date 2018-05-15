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

#include "morse.hpp"

#include "baseband_api.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "utility.hpp"

namespace morse {

// Returns 0 if message is too long
size_t morse_encode(std::string& message, const uint32_t time_unit_ms,
	const uint32_t tone, uint32_t * const time_units) {
	
	size_t i, c;
	uint16_t code, code_size;
	uint8_t morse_message[256];
	uint32_t delta;
	
	*time_units = 0;
	
	i = 0;
	for (char& ch : message) {
		if (i > 256) return 0;						// Message too long
		
		if ((ch >= 'a') && (ch <= 'z'))				// Make uppercase
			ch -= 32;
		
		if ((ch >= '!') && (ch <= '_')) {
			code = morse_ITU[ch - '!'];
		} else {								
			code = 0;								// Default to space char
		}
		
		if (!code) {
			if (i)
				morse_message[i - 1] = 4;			// Word space
		} else {
			code_size = code & 7;
			
			for (c = 0; c < code_size; c++) {
				morse_message[i++] = ((code << c) & 0x8000) ? 1 : 0;	// Dot/dash
				morse_message[i++] = 2;				// Symbol space
			}
			morse_message[i - 1] = 3;				// Letter space
		}
	}
	
	// Count time units
	for (c = 0; c < i; c++) {
		*time_units += morse_symbols[morse_message[c]];
	}
	
	memcpy(shared_memory.bb_data.tones_data.message, morse_message, i);
	
	// Setup tone "symbols"
	for (c = 0; c < 5; c++) {
		if (c < 2)
			delta = TONES_F2D(tone, TONES_SAMPLERATE);	// Dot and dash
		else
			delta = 0;					// Pause
		
		baseband::set_tone(c, delta, (TONES_SAMPLERATE * morse_symbols[c] * time_unit_ms) / 1000);
	}
	
	return i;
}

} /* namespace morse */
