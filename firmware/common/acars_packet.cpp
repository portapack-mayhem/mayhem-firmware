/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "acars_packet.hpp"

#include "crc.hpp"

#include <cstdlib>

namespace acars {

size_t Packet::length() const {
	return packet_.size();
}

bool Packet::is_valid() const {
	return true;	//length_valid() && crc_ok();
}

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

uint8_t Packet::block_id() const {
	return field_.read(96, 8);
}

std::string Packet::registration_number() const {
	std::string result;
	result.reserve(7);
	
	const size_t character_length = 8;
	for(size_t i=16; i<(16+7*character_length); i+=character_length) {
		result += (field_.read(i, character_length) & 0x7F);
	}

	return result;
}

uint32_t Packet::read(const size_t start_bit, const size_t length) const {
	return field_.read(start_bit, length);
}

/*std::string Packet::text(
	const size_t start_bit,
	const size_t character_count
) const {
	std::string result;
	result.reserve(character_count);
	
	const size_t character_length = 6;
	const size_t end_bit = start_bit + character_count * character_length;
	for(size_t i=start_bit; i<end_bit; i+=character_length) {
		result += char_to_ascii(field_.read(i, character_length));
	} 

	return result;
}*/

bool Packet::crc_ok() const {
	CRCReader field_crc { packet_ };
	CRC<16> acars_fcs { 0x1021, 0x0000, 0x0000 };
	
	for(size_t i=0; i<data_length(); i+=8) {
		acars_fcs.process_byte(field_crc.read(i, 8));
	}

	return (acars_fcs.checksum() == (unsigned)field_crc.read(data_length(), fcs_length));
}

size_t Packet::data_and_fcs_length() const {
	return length() - 8;
}

size_t Packet::data_length() const {
	return data_and_fcs_length() - fcs_length;
}

bool Packet::length_valid() const {
	const size_t extra_bits = data_and_fcs_length() & 7;
	if( extra_bits != 0 ) {
		return false;
	}

	return true;
}

} /* namespace ais */
