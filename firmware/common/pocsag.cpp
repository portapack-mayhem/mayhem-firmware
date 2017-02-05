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

#include "pocsag.hpp"

#include "baseband_api.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "utility.hpp"

namespace pocsag {

std::string bitrate_str(BitRate bitrate) {
	switch (bitrate) {
		case BitRate::FSK512:	return "512 ";
		case BitRate::FSK1200:	return "1200";
		case BitRate::FSK2400:	return "2400";
		default:				return "????";
	}
}

std::string flag_str(PacketFlag packetflag) {
	switch (packetflag) {
		case PacketFlag::NORMAL:	return "OK";
		case PacketFlag::IDLE:		return "IDLE";
		case PacketFlag::TIMED_OUT:	return "TIMED OUT";
		case PacketFlag::TOO_LONG:	return "TOO LONG";
		default:					return "";
	}
}

bool decode_batch(const POCSAGPacket& batch, POCSAGState * const state) {
	uint32_t codeword;
	char ascii_char;
	std::string output_text = "";
	
	state->out_type = EMPTY;
	
	// For each codeword...
	for (size_t i = 0; i < 16; i++) {
		codeword = batch[i];
		
		if (!(codeword & 0x80000000U)) {
			// Address codeword
			if (state->mode == STATE_CLEAR) {
				if (codeword != POCSAG_IDLE) {
					state->function = (codeword >> 11) & 3;
					state->address = (codeword >> 10) & 0x1FFFF8U;	// 18 MSBs are transmitted
					state->mode = STATE_HAVE_ADDRESS;
					state->out_type = ADDRESS;
					
					state->ascii_idx = 0;
					state->ascii_data = 0;
				}
			} else {
				state->mode = STATE_CLEAR;				// New address = new message
			}
		} else {
			// Message codeword
			if (state->mode == STATE_HAVE_ADDRESS) {
				// First message codeword: complete address
				state->address |= (i >> 1);				// Add in the 3 LSBs (frame #)
				state->mode = STATE_GETTING_MSG;
			}
			
			state->out_type = MESSAGE;
			
			state->ascii_data |= ((codeword >> 11) & 0xFFFFF);	// Get 20 message bits
			state->ascii_idx += 20;
			
			// Raw 20 bits to 7 bit reversed ASCII
			while (state->ascii_idx >= 7) {
				state->ascii_idx -= 7;
				ascii_char = ((state->ascii_data) >> (state->ascii_idx)) & 0x7F;
				
				// Bottom's up
				ascii_char = (ascii_char & 0xF0) >> 4 | (ascii_char & 0x0F) << 4;	// 01234567 -> 45670123
				ascii_char = (ascii_char & 0xCC) >> 2 | (ascii_char & 0x33) << 2;	// 45670123 -> 67452301
				ascii_char = (ascii_char & 0xAA) >> 2 | (ascii_char & 0x55);		// 67452301 -> *7654321
   
				// Translate non-printable chars
				if ((ascii_char < 32) || (ascii_char > 126))
					output_text += "[" + to_string_dec_uint(ascii_char) + "]";
				else
					output_text += ascii_char;
			}
			
			state->ascii_data <<= 20;		// Remaining bits are for next time...
		}
	}
	
	state->output = output_text;
	
	if (state->mode == STATE_HAVE_ADDRESS)
		state->mode = STATE_CLEAR;
	
	return true;
}

} /* namespace pocsag */
