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

#include "ax25.hpp"

#include "portapack_shared_memory.hpp"

namespace ax25 {

void AX25Frame::make_extended_field(char * const data, size_t length) {
	size_t i = 0;
	
	for (i = 0; i < length - 1; i++)
		add_data(data[i] << 1);
	
	add_data((data[i] << 1) | 1);
}

void AX25Frame::add_byte(uint8_t byte, bool is_flag, bool is_data) {
	size_t i;
	
	for (i = 0; i < 8; ) {
		
		if (!(byte & 1)) {
			current_bit ^= 1;		// Zero: flip
			ones_counter = 0;
			byte >>= 1;
			i++;
		} else {
			ones_counter++;
			if ((ones_counter == 5) && (!is_flag)) {
				current_bit ^= 1;	// Stuff zero: flip
				ones_counter = 0;
			} else {
				byte >>= 1;
				i++;
			}
		}
		
		if (is_data)
			crc_ccitt.process_bit(current_bit);
		current_byte <<= 1;
		current_byte |= current_bit;
		
		if (bit_counter == 7) {
			bit_counter = 0;
			*bb_data_ptr = current_byte;
			bb_data_ptr++;
		} else {
			bit_counter++;
		}
	}
}

void AX25Frame::flush() {
	if (bit_counter)
		*bb_data_ptr = current_byte << (7 - bit_counter);
};

void AX25Frame::add_flag() {
	add_byte(AX25_FLAG, true, false);
};

void AX25Frame::add_data(uint8_t byte) {
	add_byte(byte, false, true);
};

void AX25Frame::add_checksum() {
	auto checksum = crc_ccitt.checksum();
	add_byte(checksum >> 8, false, false);
	add_byte(checksum, false, false);
}

void AX25Frame::make_ui_frame(char * const address, const uint8_t control,
	const uint8_t protocol, uint8_t * const info, size_t length) {
	
	size_t i;
	
	bb_data_ptr = shared_memory.bb_data.data;
	bit_counter = 0;
	current_bit = 0;
	current_byte = 0;
	ones_counter = 0;
	crc_ccitt.reset();
	
	add_flag();
	
	make_extended_field(address, 14);
	add_data(control);
	add_data(protocol);
	
	for (i = 0; i < length; i++)
		add_data(info[i]);
	
	add_checksum();
	add_flag();
	
	flush();
}

} /* namespace ax25 */
