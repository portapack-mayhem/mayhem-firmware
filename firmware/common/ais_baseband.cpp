/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ais_baseband.hpp"

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <string>

#include "crc.hpp"

// TODO: Move string formatting elsewhere!!!
#include "ui_widget.hpp"

namespace baseband {

template<typename T, typename BitRemap>
class FieldReader {
public:
	constexpr FieldReader(
		const T& data
	) : data { data }
	{
	}

	uint32_t read(const size_t start_bit, const size_t length) const {
		uint32_t value = 0;
		for(size_t i=start_bit; i<(start_bit + length); i++) {
			value = (value << 1) | data[bit_remap(i)];
		}
		return value;
	}

private:
	const T& data;
	const BitRemap bit_remap { };
};

namespace ais {

struct BitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index ^ 7;
	}
};

struct CRCBitRemap {
	size_t operator()(const size_t bit_index) const {
		return bit_index;
	}
};

using FieldReader = baseband::FieldReader<std::bitset<1024>, BitRemap>;
using CRCFieldReader = baseband::FieldReader<std::bitset<1024>, CRCBitRemap>;

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

	bool contains(const size_t bit_count) const {
		return !is_above(bit_count) && !is_below(bit_count);
	}

	bool is_above(const size_t bit_count) const {
		return (min() > bit_count);
	}

	bool is_below(const size_t bit_count) const {
		return (max() < bit_count);
	}

	size_t min() const {
		return min_bytes * 8;
	}
	
	size_t max() const {
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
	bool operator()(const uint_fast8_t message_id, const size_t length) {
		return packet_length_range[message_id].contains(length);
	}
};

struct PacketTooLong {
	bool operator()(const uint_fast8_t message_id, const size_t length) {
		return packet_length_range[message_id].is_below(length);
	}
};

struct CRCCheck {
	bool operator()(const std::bitset<1024>& payload, const size_t data_length) {
		CRCFieldReader field_crc { payload };
		CRC<uint16_t> ais_fcs { 0x1021 };
		
		uint16_t crc_calculated = 0xffff;
		
		for(size_t i=0; i<data_length + 16; i+=8) {
			if( i == data_length ) {
				crc_calculated ^= 0xffff;
			}
			const uint8_t byte = field_crc.read(i, 8);
			crc_calculated = ais_fcs.calculate_byte(crc_calculated, byte);
		}

		return crc_calculated == 0x0000;
	}
};

static int32_t ais_latitude_normalized(
	const FieldReader& field,
	const size_t start_bit
) {
	// Shifting and dividing is to sign-extend the source field.
	// TODO: There's probably a more elegant way to do it.
	return static_cast<int32_t>(field.read(start_bit, 27) << 5) / 32;
}

static int32_t ais_longitude_normalized(
	const FieldReader& field,
	const size_t start_bit
) {
	// Shifting and dividing is to sign-extend the source field.
	// TODO: There's probably a more elegant way to do it.
	return static_cast<int32_t>(field.read(start_bit, 28) << 4) / 16;
}

static std::string ais_format_latlon_normalized(const int32_t normalized) {
	const int32_t t = (normalized * 5) / 3;
	const int32_t degrees = t / (100 * 10000);
	const int32_t fraction = std::abs(t) % (100 * 10000);
	return ui::to_string_dec_int(degrees) + "." + ui::to_string_dec_int(fraction, 6, '0');
}

static std::string ais_format_latitude(
	const FieldReader& field,
	const size_t start_bit
) {
	const auto value = static_cast<int32_t>(field.read(start_bit, 27) << 5) / 32;
	return ais_format_latlon_normalized(value);
}

static std::string ais_format_longitude(
	const FieldReader& field,
	const size_t start_bit
) {
	const auto value = static_cast<int32_t>(field.read(start_bit, 28) << 4) / 16;
	return ais_format_latlon_normalized(value);
}

struct ais_datetime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

static ais_datetime ais_get_datetime(
	const FieldReader& field,
	const size_t start_bit
) {
	return {
		static_cast<uint16_t>(field.read(start_bit +  0, 14)),
		static_cast<uint8_t >(field.read(start_bit + 14,  4)),
		static_cast<uint8_t >(field.read(start_bit + 18,  5)),
		static_cast<uint8_t >(field.read(start_bit + 23,  5)),
		static_cast<uint8_t >(field.read(start_bit + 28,  6)),
		static_cast<uint8_t >(field.read(start_bit + 34,  6)),
	};
}

static std::string ais_format_datetime(
	const FieldReader& field,
	const size_t start_bit
) {
	const auto datetime = ais_get_datetime(field, start_bit);
	return ui::to_string_dec_uint(datetime.year, 4) + "/" +
		ui::to_string_dec_uint(datetime.month, 2, '0') + "/" +
		ui::to_string_dec_uint(datetime.day, 2, '0') + " " +
		ui::to_string_dec_uint(datetime.hour, 2, '0') + ":" +
		ui::to_string_dec_uint(datetime.minute, 2, '0') + ":" +
		ui::to_string_dec_uint(datetime.second, 2, '0');
}

static char ais_char_to_ascii(const uint8_t c) {
	return (c ^ 32) + 32;
}

static std::string ais_read_text(
	const FieldReader& field,
	const size_t start_bit,
	const size_t character_count
) {
	std::string result;
	const size_t character_length = 6;
	const size_t end_bit = start_bit + character_count * character_length;
	for(size_t i=start_bit; i<end_bit; i+=character_length) {
		result += ais_char_to_ascii(field.read(i, character_length));
	} 

	return result;
}

static std::string ais_format_navigational_status(const unsigned int value) {
	switch(value) {
		case 0: return "under way w/engine";
		case 1: return "at anchor";
		case 2: return "not under command";
		case 3: return "restricted maneuv";
		case 4: return "constrained draught";
		case 5: return "moored";
		case 6: return "aground";
		case 7: return "fishing";
		case 8: return "sailing";
		case 9: case 10: case 13: return "reserved";
		case 11: return "towing astern";
		case 12: return "towing ahead/along";
		case 14: return "SART/MOB/EPIRB";
		case 15: return "undefined";
		default: return "unexpected";
	}
}

decoded_packet packet_decode(const std::bitset<1024>& payload, const size_t payload_length) {
	// TODO: Unstuff here, not in baseband!

	const size_t data_and_fcs_length = payload_length;

	if( data_and_fcs_length < 38 ) {
		return { "short " + ui::to_string_dec_uint(data_and_fcs_length, 3), "" };
	}

	const size_t extra_bits = data_and_fcs_length & 7;
	if( extra_bits != 0 ) {
		return { "extra bits " + ui::to_string_dec_uint(data_and_fcs_length, 3), "" };
	}

	FieldReader field { payload };

	const auto message_id = field.read(0, 6);

	const size_t data_length = data_and_fcs_length - 16;
	PacketLengthValidator packet_length_valid;
	if( !packet_length_valid(message_id, data_length) ) {
		return { "bad length " + ui::to_string_dec_uint(data_length, 3), "" };
	}

	CRCCheck crc_ok;
	if( !crc_ok(payload, data_length) ) {
		return { "crc", "" };
	}

	const auto source_id = field.read(8, 30);
	std::string result { ui::to_string_dec_uint(message_id, 2) + " " + ui::to_string_dec_uint(source_id, 10) };

	switch(message_id) {
	case 1:
	case 2:
	case 3:
		{
			const auto navigational_status = field.read(38, 4);
			result += " " + ais_format_navigational_status(navigational_status);
			result += " " + ais_format_latlon_normalized(ais_latitude_normalized(field, 89));
			result += " " + ais_format_latlon_normalized(ais_longitude_normalized(field, 61));
		}
		break;

	case 4:
		{
			result += " " + ais_format_datetime(field, 38);
			result += " " + ais_format_latlon_normalized(ais_latitude_normalized(field, 107));
			result += " " + ais_format_latlon_normalized(ais_longitude_normalized(field, 79));
		}
		break;

	case 5:
		{
			const auto call_sign = ais_read_text(field, 70, 7);
			const auto name = ais_read_text(field, 112, 20);
			const auto destination = ais_read_text(field, 302, 20);
			result += " \"" + call_sign + "\" \"" + name + "\" \"" + destination + "\""; 
		}
		break;

	case 21:
		{
			const auto name = ais_read_text(field, 43, 20);
			result += " \"" + name + "\" " + ais_format_latitude(field, 192) + " " + ais_format_longitude(field, 164);
		}
		break;

	default:
		break;
	}

	return { "OK", result };
}

} /* namespace ais */
} /* namespace baseband */
