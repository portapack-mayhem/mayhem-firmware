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
size_t morse_encode(std::string& message, const uint32_t time_unit_ms, const uint32_t tone) {
	size_t i, c;
	uint8_t code, code_size;
	uint8_t morse_message[256];
	
	ToneDef * tone_defs = shared_memory.bb_data.tones_data.tone_defs;
	
	i = 0;
	for (char& ch : message) {
		if ((ch >= 'a') && (ch <= 'z'))					// Make uppercase
			ch -= 32;
		
		if ((ch >= 'A') && (ch <= 'Z')) {
			code = morse_ITU[ch - 'A' + 10];
		} else if ((ch >= '0') && (ch <= '9')) {
			code = morse_ITU[ch - '0'];
		} else {
			ch = ' ';									// Default to space char
			code = 0;
		}
		
		if (ch == ' ') {
			if (i)
				morse_message[i - 1] = 4;				// Word space
		} else {
			code_size = code & 7;
			
			for (c = 0; c < code_size; c++) {
				morse_message[i++] = ((code << c) & 0x80) ? 1 : 0;	// Dot/dash
				morse_message[i++] = 2;					// Symbol space
			}
			morse_message[i - 1] = 3;					// Letter space
		}
		
	}
	
	if (i > 256) return 0;
	
	memcpy(shared_memory.bb_data.tones_data.message, morse_message, i);
	
	// Setup tone "symbols"
	for (c = 0; c < 5; c++) {
		if (c < 2)
			tone_defs[c].delta = TONES_F2D(tone);	// Dot and dash
		else
			tone_defs[c].delta = 0;					// Pause
		
		tone_defs[c].duration = (TONES_SAMPLERATE * morse_symbols[c] * time_unit_ms) / 1000;
	}
	
	return i;
}

} /* namespace morse */
