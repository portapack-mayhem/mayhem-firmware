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

#include "sonde_packet.hpp"
#include "string_format.hpp"

namespace sonde {

Packet::Packet(
	const baseband::Packet& packet,
	const Type type
) : packet_ { packet },
	decoder_ { packet_ },
	reader_bi_m { decoder_ },
	type_ { type }
{
	if (type_ == Type::Meteomodem_unknown) {
		// Right now we're just sure that the sync is from a Meteomodem sonde, differentiate between models now
		const uint32_t id_byte = reader_bi_m.read(0 * 8, 16);
		
		if (id_byte == 0x649F)
			type_ = Type::Meteomodem_M10;
		else if (id_byte == 0x648F)
			type_ = Type::Meteomodem_M2K2;
	}
}

size_t Packet::length() const {
	return decoder_.symbols_count();
}

bool Packet::is_valid() const {
	return true;	// TODO
}

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

Packet::Type Packet::type() const {
	return type_;
}

/*uint8_t Packet::vaisala_descramble(const uint32_t pos) {
	return reader_raw.read(pos * 8, 8) ^ vaisala_mask[pos & 63];
};*/

uint32_t Packet::GPS_altitude() const {
	if ((type_ == Type::Meteomodem_M10) || (type_ == Type::Meteomodem_M2K2))
		return (reader_bi_m.read(22 * 8, 32) / 1000) - 48;
	else if (type_ == Type::Vaisala_RS41_SG) {
		/*uint32_t altitude_ecef = 0;
		for (uint32_t i = 0; i < 4; i++)
			altitude_ecef = (altitude_ecef << 8) + vaisala_descramble(0x11C + i);*/
		// TODO: and a bunch of maths (see ecef2elli() from RS1729)
		return 0;
	} else
		return 0;	// Unknown
}

float Packet::GPS_latitude() const {
	if ((type_ == Type::Meteomodem_M10) || (type_ == Type::Meteomodem_M2K2))
		return reader_bi_m.read(14 * 8, 32) / ((1ULL << 32) / 360.0);
	//else if (type_ == Type::Vaisala_RS41_SG)
	//	return vaisala_descramble();
	else
		return 0;	// Unknown
}

float Packet::GPS_longitude() const {
	if ((type_ == Type::Meteomodem_M10) || (type_ == Type::Meteomodem_M2K2))
		return reader_bi_m.read(18 * 8, 32) / ((1ULL << 32) / 360.0);
	else
		return 0;	// Unknown
}

uint32_t Packet::battery_voltage() const {
	if (type_ == Type::Meteomodem_M10)
		return (reader_bi_m.read(69 * 8, 8) + (reader_bi_m.read(70 * 8, 8) << 8)) * 1000 / 150;
	else if (type_ == Type::Meteomodem_M2K2)
		return reader_bi_m.read(69 * 8, 8) * 66;	// Actually 65.8
	else
		return 0;	// Unknown
}

std::string Packet::type_string() const {
	switch (type_) {
		case Type::Unknown: 				return "Unknown";
		case Type::Meteomodem_unknown:		return "Meteomodem ???";
		case Type::Meteomodem_M10:			return "Meteomodem M10";
		case Type::Meteomodem_M2K2:			return "Meteomodem M2K2";
		case Type::Vaisala_RS41_SG:			return "Vaisala RS41-SG";
		default:							return "? 0x" + symbols_formatted().data.substr(0, 6);
	}
}

std::string Packet::serial_number() const {
	if (type() == Type::Meteomodem_M10) {
		// See https://github.com/rs1729/RS/blob/master/m10/m10x.c line 606
		// Starting at byte #93: 00000000 11111111 22222222 33333333 44444444
		//                           CCCC          AAAABBBB
		//                                                  44444444 33333333
		//                                                  DDDEEEEE EEEEEEEE
		
		return to_string_hex(reader_bi_m.read(93 * 8 + 16, 4), 1) +
			to_string_dec_uint(reader_bi_m.read(93 * 8 + 20, 4), 2, '0') + " " +
			to_string_hex(reader_bi_m.read(93 * 8 + 4, 4), 1) + " " +
			to_string_dec_uint(reader_bi_m.read(93 * 8 + 24, 3), 1) +
			to_string_dec_uint(reader_bi_m.read(93 * 8 + 27, 13), 4, '0');
	
	} else
		return "?";
}

FormattedSymbols Packet::symbols_formatted() const {
	return format_symbols(decoder_);
}

bool Packet::crc_ok() const {
	switch(type()) {
	case Type::Meteomodem_M10:	return crc_ok_M10();
	default:					return false;
	}
}

bool Packet::crc_ok_M10() const {
    uint16_t cs { 0 };
	uint32_t c0, c1, t, t6, t7, s,b ;
    
	for (size_t i = 0; i < packet_.size(); i++) {
		b = packet_[i];
		c1 = cs & 0xFF;

		// B
		b = (b >> 1) | ((b & 1) << 7);
		b ^= (b >> 2) & 0xFF;

		// A1
		t6 = (cs & 1) ^ ((cs >> 2) & 1) ^ ((cs >> 4) & 1);
		t7 = ((cs >> 1) & 1) ^ ((cs >> 3) & 1) ^ ((cs >> 5) & 1);
		t = (cs & 0x3F) | (t6 << 6) | (t7 << 7);

		// A2
		s = (cs >> 7) & 0xFF;
		s ^= (s >> 2) & 0xFF;

		c0 = b ^ t ^ s;

		cs = ((c1<<8) | c0) & 0xFFFF;
    }

	return ((cs & 0xFFFF) == ((packet_[0x63] << 8) | (packet_[0x63 + 1])));
}

} /* namespace sonde */
