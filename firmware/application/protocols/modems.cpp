/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "modems.hpp"
#include "serializer.hpp"

#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace modems {

void generate_data(const std::string& in_message, uint16_t * out_data) {
	uint8_t parity_init, parity, bits, bit, cur_byte;
	uint16_t ordered_word;
	size_t bytes;
	
	serial_format_t serial_format = persistent_memory::serial_format();
	size_t message_length = in_message.length();
	
	if (serial_format.parity == ODD)
		parity_init = 1;
	else
		parity_init = 0;

	uint8_t data_bits = serial_format.data_bits;
	
	for (bytes = 0; bytes < message_length; bytes++) {
		parity = parity_init;
		cur_byte = in_message[bytes];
		bit = 0;
		
		if (serial_format.bit_order == MSB_FIRST) {
			ordered_word = cur_byte;
			for (bits = 0; bits < data_bits; bits++) {
				bit = (cur_byte >> bits) & 1;	// Get LSB
				parity += bit;
			}
		} else {
			ordered_word = 0;
			for (bits = 0; bits < data_bits; bits++) {
				bit = (cur_byte >> bits) & 1;	// Get LSB
				parity += bit;
				ordered_word |= (bit << ((data_bits - 1) - bits));	// Set MSB
			}
		}
		
		if (serial_format.parity) {
			ordered_word <<= 1;
			ordered_word |= (parity & 1);
		}
		
		for (bits = 0; bits < serial_format.stop_bits; bits++) {
			ordered_word <<= 1;
			ordered_word |= 1;
		}
		
		out_data[bytes] = ordered_word;
	}
	
	out_data[bytes] = 0;	// End marker
}

// This accepts a word with start and stop bits removed !
uint32_t deframe_word(uint32_t raw_word) {
	uint32_t cur_bit, deframed_word { 0 };
	size_t bit;
	
	serial_format_t serial_format = persistent_memory::serial_format();
	
	/*if (serial_format.parity == ODD)
		parity = 1;
	else
		parity = 0;*/

	size_t data_bits = serial_format.data_bits;
	
	// Ignore parity for now
	if (serial_format.parity)
		raw_word >>= 1;
		
	if (serial_format.bit_order == LSB_FIRST) {
		// Reverse data bits
		for (bit = 0; bit < data_bits; bit++) {
			cur_bit = raw_word & 1;
			
			deframed_word <<= 1;
			deframed_word |= cur_bit;
			
			//parity += cur_bit;
			
			raw_word >>= 1;
		}
		
		return deframed_word;
	} else
		return raw_word;
}

} /* namespace modems */
