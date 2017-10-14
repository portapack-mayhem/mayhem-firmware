/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "encoders.hpp"

using namespace portapack;

namespace encoders {

size_t make_bitstream(std::string& fragments) {
	uint8_t byte = 0;
	size_t bitstream_length = 0;
	uint8_t * bitstream = shared_memory.bb_data.data;
	
	for (auto c : fragments) {
		byte <<= 1;
		if (c != '0')
			byte |= 1;
		
		if ((bitstream_length & 7) == 7)
			bitstream[bitstream_length >> 3] = byte;
		
		bitstream_length++;
	}
	
	// Finish last byte if needed
	size_t padding =  8 - (bitstream_length & 7);
	if (padding != 8) {
		byte <<= padding;
		bitstream[(bitstream_length + padding - 1) >> 3] = byte;
		padding++;
	}
	
	return bitstream_length;
}

void bitstream_append(size_t& bitstream_length, uint32_t bit_count, uint32_t bits) {
	uint8_t * bitstream = shared_memory.bb_data.data;
	uint32_t bit_mask = 1 << (bit_count - 1);
	uint32_t bit_index;
	uint8_t byte = 0;
	
	if (bitstream_length & 7)
		byte = bitstream[bitstream_length >> 3];
	
	bit_index = 7 - (bitstream_length & 7);
	
	for (size_t i = 0; i < bit_count; i++) {
		if (bits & bit_mask)
			byte |= (1 << bit_index);
		
		if (!bit_index) {
			bitstream[bitstream_length >> 3] = byte;
			byte = 0;
		}
				
		bit_index = (bit_index - 1) & 7;
		bits <<= 1;
		
		bitstream_length++;
	}
	
	bitstream[bitstream_length >> 3] = byte;
}
	
} /* namespace encoders */
