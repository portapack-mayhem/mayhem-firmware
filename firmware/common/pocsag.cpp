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
using namespace portapack;

#include "string_format.hpp"
#include "utility.hpp"

namespace pocsag {

std::string bitrate_str(BitRate bitrate) {
	switch (bitrate) {
		case BitRate::FSK512:	return "512bps ";
		case BitRate::FSK1200:	return "1200bps";
		case BitRate::FSK2400:	return "2400bps";
		default:				return "????";
	}
}

std::string flag_str(PacketFlag packetflag) {
	switch (packetflag) {
		case PacketFlag::NORMAL:	return "OK";
		case PacketFlag::TIMED_OUT:	return "TIMED OUT";
		default:					return "";
	}
}

void insert_BCH(BCHCode& BCH_code, uint32_t * codeword) {
	uint32_t parity = 0;
	int data[21];
	int bit;
	int * bb;
	size_t c;
	
	for (c = 0; c < 21; c++) {
		bit = (((*codeword) << c) & 0x80000000U) ? 1 : 0;
		if (bit) parity++;
		data[c] = bit;
	}
	
	bb = BCH_code.encode(data);
	
	// Make sure ECC bits are cleared
	(*codeword) &= 0xFFFFF801;
	
	for (c = 0; c < 10; c++) {
		bit = bb[c];
		(*codeword) |= (bit << (10 - c));
		if (bit) parity++;
	}
	
	// Even parity
	(*codeword) |= (parity & 1);
}

uint32_t get_digit_code(char code) {
	if ((code >= '0') && (code <= '9')) {
		code -= '0';
	} else {
		if (code == 'S')
			code = 10;
		else if (code == 'U')
			code = 11;
		else if (code == ' ')
			code = 12;
		else if (code == '-')
			code = 13;
		else if (code == ']')
			code = 14;
		else if (code == '[')
			code = 15;
		else
			code = 12;
	}
	
	code = ((code & 0x0C) >> 2) | ((code & 0x03) << 2);		// ----3210 -> ----1032
	code = ((code & 0x0A) >> 1) | ((code & 0x05) << 1);		// ----1032 -> ----0123
	
	return code;
}
	
void pocsag_encode(const MessageType type, BCHCode& BCH_code, const uint32_t function, const std::string message, const uint32_t address,
	std::vector<uint32_t>& codewords) {
	
	size_t b, c, address_slot;
	size_t bit_idx, char_idx = 0;
	uint32_t codeword, digit_code;
	char ascii_char = 0;
	
	size_t message_size = message.size();
	
	// Preamble
	for (b = 0; b < (POCSAG_PREAMBLE_LENGTH / 32); b++) {
		codewords.push_back(0xAAAAAAAA);
	}
	
	// Address
	codeword = (address & 0x1FFFF8U) << 10;
	address_slot = (address & 7) * 2;
	// Function
	codeword |= (function << 11);
	
	insert_BCH(BCH_code, &codeword);
	
	// Address batch
	codewords.push_back(POCSAG_SYNCWORD);
	for (c = 0; c < 16; c++) {
		if (c == address_slot) {
			codewords.push_back(codeword);
			if (type != MessageType::ADDRESS_ONLY) break;
		} else
			codewords.push_back(POCSAG_IDLEWORD);
	}
	
	if (type == MessageType::ADDRESS_ONLY) return;	// Done.
	
	c++;
	codeword = 0;
	bit_idx = 20 + 11;
	
	// Messages batch(es)
	do {
		if (c == 0) codewords.push_back(POCSAG_SYNCWORD);
		
		for ( ; c < 16; c++) {
			
			// Fill up 20 bits
			if (type == MessageType::ALPHANUMERIC) {
				if ((char_idx < message_size) || (ascii_char)) {
					do {
						bit_idx -= 7;
						
						if (char_idx < message_size)
							ascii_char = message[char_idx] & 0x7F;
						else
							ascii_char = 0;		// Codeword padding
						
						// Bottom's up
						ascii_char = (ascii_char & 0xF0) >> 4 | (ascii_char & 0x0F) << 4;	// *6543210 -> 3210*654
						ascii_char = (ascii_char & 0xCC) >> 2 | (ascii_char & 0x33) << 2;	// 3210*654 -> 103254*6
						ascii_char = (ascii_char & 0xAA) >> 2 | (ascii_char & 0x55);		// 103254*6 -> *0123456
						
						codeword |= (ascii_char << bit_idx);
						
						char_idx++;
						
					} while (bit_idx > 11);
					
					codeword &= 0x7FFFF800;		// Trim data
					codeword |= 0x80000000;		// Message type
					insert_BCH(BCH_code, &codeword);
					
					codewords.push_back(codeword);
					
					if (bit_idx != 11) {
						bit_idx = 20 + bit_idx;
						codeword = ascii_char << bit_idx;
					} else {
						bit_idx = 20 + 11;
						codeword = 0;
					}
				} else {
					codewords.push_back(POCSAG_IDLEWORD);	// Batch padding
				}
			} else if (type == MessageType::NUMERIC_ONLY) {
				if (char_idx < message_size) {
					do {
						bit_idx -= 4;
						
						if (char_idx < message_size)
							digit_code = get_digit_code(message[char_idx]);
						else
							digit_code = 3;		// Space (codeword padding)
						
						codeword |= (digit_code << bit_idx);
						
						char_idx++;
						
					} while (bit_idx > 11);
					
					codeword |= 0x80000000;		// Message type
					insert_BCH(BCH_code, &codeword);
					
					codewords.push_back(codeword);
					
					bit_idx = 20 + 11;
					codeword = 0;
				} else {
					codewords.push_back(POCSAG_IDLEWORD);	// Batch padding
				}
			}
		}
		
		c = 0;
		
	} while (char_idx < message_size);
}

void pocsag_decode_batch(const POCSAGPacket& batch, POCSAGState * const state) {
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
				if (codeword != POCSAG_IDLEWORD) {
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
}

} /* namespace pocsag */
