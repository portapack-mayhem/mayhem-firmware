/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ais_packet.hpp"

#include "crc.hpp"

#include <cstdlib>

namespace ais {

struct PacketLengthRange {
	constexpr PacketLengthRange(
	) : min_bytes { 0 },
		max_bytes { 0 }
	{
	}

	constexpr PacketLengthRange(
		const uint16_t min_bits,
		const uint16_t max_bits
	) : min_bytes { static_cast<uint8_t>(min_bits / 8U) },
		max_bytes { static_cast<uint8_t>(max_bits / 8U) }
	{
		// static_assert((min_bits & 7) == 0, "minimum bits not a multiple of 8");
		// static_assert((max_bits & 7) == 0, "minimum bits not a multiple of 8");
	}

	constexpr bool contains(const size_t bit_count) const {
		return !is_above(bit_count) && !is_below(bit_count);
	}

	constexpr bool is_above(const size_t bit_count) const {
		return (min() > bit_count);
	}

	constexpr bool is_below(const size_t bit_count) const {
		return (max() < bit_count);
	}

	constexpr size_t min() const {
		return min_bytes * 8;
	}
	
	constexpr size_t max() const {
		return max_bytes * 8;
	}

private:
	const uint8_t min_bytes;
	const uint8_t max_bytes;
};

static constexpr std::array<PacketLengthRange, 64> packet_length_range { {
	{    0,    0 },	// 0
	{  168,  168 },	// 1
	{  168,  168 },	// 2
	{  168,  168 }, // 3
	{  168,  168 }, // 4
	{  424,  424 }, // 5
	{    0,    0 }, // 6
	{    0,    0 }, // 7
	{    0, 1008 }, // 8
	{    0,    0 }, // 9
	{    0,    0 }, // 10
	{    0,    0 }, // 11
	{    0,    0 }, // 12
	{    0,    0 }, // 13
	{    0,    0 }, // 14
	{    0,    0 }, // 15
	{    0,    0 }, // 16
	{    0,    0 }, // 17
	{  168,  168 }, // 18
	{    0,    0 }, // 19
	{   72,  160 }, // 20
	{  272,  360 }, // 21
	{  168,  168 }, // 22
	{  160,  160 }, // 23
	{  160,  168 }, // 24
	{    0,  168 }, // 25
	{    0,    0 }, // 26
	{    0,    0 }, // 27
	{    0,    0 }, // 28
	{    0,    0 }, // 29
	{    0,    0 }, // 30
	{    0,    0 }, // 31
} };

struct PacketLengthValidator {
	constexpr bool operator()(const uint_fast8_t message_id, const size_t length) const {
		return packet_length_range[message_id].contains(length);
	}
};

struct PacketTooLong {
	constexpr bool operator()(const uint_fast8_t message_id, const size_t length) const {
		return packet_length_range[message_id].is_below(length);
	}
};

static constexpr char char_to_ascii(const uint8_t c) {
	return (c ^ 32) + 32;
}

size_t Packet::length() const {
	return packet_.size();
}

bool Packet::is_valid() const {
	return length_valid() && crc_ok();
}

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

uint32_t Packet::message_id() const {
	return field_.read(0, 6);
}

MMSI Packet::user_id() const {
	return field_.read(8, 30);
}

MMSI Packet::source_id() const {
	return field_.read(8, 30);
}

uint32_t Packet::read(const size_t start_bit, const size_t length) const {
	return field_.read(start_bit, length);
}

std::string Packet::text(
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
}

DateTime Packet::datetime(const size_t start_bit) const {
	return {
		static_cast<uint16_t>(field_.read(start_bit +  0, 14)),
		static_cast<uint8_t >(field_.read(start_bit + 14,  4)),
		static_cast<uint8_t >(field_.read(start_bit + 18,  5)),
		static_cast<uint8_t >(field_.read(start_bit + 23,  5)),
		static_cast<uint8_t >(field_.read(start_bit + 28,  6)),
		static_cast<uint8_t >(field_.read(start_bit + 34,  6)),
	};
}

Latitude Packet::latitude(const size_t start_bit) const {
	return field_.read(start_bit, 27);
}

Longitude Packet::longitude(const size_t start_bit) const {
	return field_.read(start_bit, 28);
}

bool Packet::crc_ok() const {
	CRCReader field_crc { packet_ };
	CRC<16> ais_fcs { 0x1021, 0xffff, 0xffff };
	
	for(size_t i=0; i<data_length(); i+=8) {
		ais_fcs.process_byte(field_crc.read(i, 8));
	}

	return (ais_fcs.checksum() == (unsigned)field_crc.read(data_length(), fcs_length));
}

size_t Packet::data_and_fcs_length() const {
	// Subtract end flag (8 bits) - one unstuffing bit (occurs during end flag).
	return length() - 7;
}

size_t Packet::data_length() const {
	return data_and_fcs_length() - fcs_length;
}

bool Packet::length_valid() const {
	const size_t extra_bits = data_and_fcs_length() & 7;
	if( extra_bits != 0 ) {
		return false;
	}

	const PacketLengthValidator packet_length_valid;
	if( !packet_length_valid(message_id(), data_length()) ) {
		return false;
	}

	return true;
}

} /* namespace ais */
