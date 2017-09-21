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

#include "bht.hpp"
#include "portapack_persistent_memory.hpp"

size_t gen_message_ep(uint8_t city_code, size_t family_code_ep, uint32_t relay_number, uint32_t relay_state) {
	size_t c;
	const encoder_def_t * um3750_def;
	uint8_t bits[12];
	std::string ep_fragments;
	//char ep_message[13] = { 0 };
	
	// Repeated 2x 26 times
	// Whole frame + space = 128ms, data only = 64ms
	
	um3750_def = &encoder_defs[ENCODER_UM3750];

	// City code is bit-reversed
	for (c = 0; c < 8; c++)
		bits[c] = (city_code >> c) & 1;
	
	bits[8] = (family_code_ep >> 1) & 1;
	bits[9] = family_code_ep & 1;
	bits[10] = relay_number & 1;
	bits[11] = relay_state ? 1 : 0;
	
	// Text for display
	//for (c = 0; c < 12; c++)
	//	ep_message[c] = bits[c] + '0';

	c = 0;
	for (auto ch : um3750_def->word_format) {
		if (ch == 'S')
			ep_fragments += um3750_def->sync;
		else
			ep_fragments += um3750_def->bit_format[bits[c++]];
	}
	
	// Return bitstream length
	return make_bitstream(ep_fragments);
}

std::string gen_message_xy(const std::string& ascii_code) {
	std::string local_code = ascii_code;
	uint8_t ccir_message[XY_TONE_COUNT];
	uint8_t translate;
	uint32_t c;
	
	// Replace repeats with E code
	for (c = 1; c < XY_TONE_COUNT; c++)
		if (local_code[c] == local_code[c - 1]) local_code[c] = 'E';
		
	for (c = 0; c < XY_TONE_COUNT; c++) {
		if (local_code[c] <= '9')
			translate = local_code[c] - '0';
		else
			translate = local_code[c] - 'A' + 10;
		ccir_message[c] = (translate < 16) ? translate : 0;		// Sanitize
	}
	
	// Copy for baseband
	memcpy(shared_memory.bb_data.tones_data.message, ccir_message, XY_TONE_COUNT);
	
	// Return as text for display
	return local_code;
}

std::string gen_message_xy(size_t header_code_a, size_t header_code_b, size_t city_code, size_t family_code,
							bool subfamily_wc, size_t subfamily_code, bool id_wc, size_t receiver_code,
							size_t relay_state_A, size_t relay_state_B, size_t relay_state_C, size_t relay_state_D) {
	uint8_t ccir_message[XY_TONE_COUNT];
	size_t c;

	// Header
	ccir_message[0] = (header_code_a / 10);
	ccir_message[1] = (header_code_a % 10);
	ccir_message[2] = (header_code_b / 10);
	ccir_message[3] = (header_code_b % 10);
	
	// Addresses
	ccir_message[4] = (city_code / 10);
	ccir_message[5] = (city_code % 10);
	ccir_message[6] = family_code;
	
	if (subfamily_wc)
		ccir_message[7] = 0xA;		// Wildcard
	else
		ccir_message[7] = subfamily_code;
	
	if (id_wc) {
		ccir_message[8] = 0xA;		// Wildcard
		ccir_message[9] = 0xA;		// Wildcard
	} else {
		ccir_message[8] = (receiver_code / 10);
		ccir_message[9] = (receiver_code % 10);
	}
	
	ccir_message[10] = 0xB;
	
	// Relay states
	ccir_message[11] = relay_state_A;
	ccir_message[12] = relay_state_B;
	ccir_message[13] = relay_state_C;
	ccir_message[14] = relay_state_D;
	
	ccir_message[15] = 0xB;
	
	// End
	for (c = 16; c < XY_TONE_COUNT; c++)
		ccir_message[c] = 0;
	
	// Replace repeats with E code
	for (c = 1; c < XY_TONE_COUNT; c++)
		if (ccir_message[c] == ccir_message[c - 1]) ccir_message[c] = 0xE;
	
	// Copy for baseband
	memcpy(shared_memory.bb_data.tones_data.message, ccir_message, XY_TONE_COUNT);
	
	// Return as text for display
	return ccir_to_ascii(ccir_message);
}

std::string ccir_to_ascii(uint8_t * ccir) {
	std::string ascii;
	
	for (size_t c = 0; c < XY_TONE_COUNT; c++) {
		if (ccir[c] > 9)
			ascii += (char)(ccir[c] - 10 + 'A');
		else
			ascii += (char)(ccir[c] + '0');
	}
	
	return ascii;
}
