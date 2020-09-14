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

#ifndef __SONDE_PACKET_H__
#define __SONDE_PACKET_H__

#include <cstdint>
#include <cstddef>

#include "field_reader.hpp"
#include "baseband_packet.hpp"
#include "manchester.hpp"

namespace sonde {

	static uint8_t calibytes[51*16];	//need these vars to survive
	static uint8_t calfrchk[51];		//so subframes are preserved while populated

	struct GPS_data {
		uint32_t alt { 0 };
		float lat { 0 };
		float lon { 0 };
	};

	struct temp_humid {
		float temp { 0 };
		float humid { 0 };
	};

class Packet {
public:
	enum class Type : uint32_t {
		Unknown = 0,
		Meteomodem_unknown = 1,
		Meteomodem_M10 = 2,
		Meteomodem_M2K2 = 3,
		Vaisala_RS41_SG = 4,
	};

	Packet(const baseband::Packet& packet, const Type type);

	size_t length() const;
	
	Timestamp received_at() const;

	Type type() const;
	std::string type_string() const;
	
	std::string serial_number() const;
	uint32_t battery_voltage() const;
	GPS_data get_GPS_data() const;
	uint32_t frame() const;	
	temp_humid get_temp_humid() const;

	FormattedSymbols symbols_formatted() const;

	bool crc_ok() const;

private:
	static constexpr uint8_t vaisala_mask[64] = { 
		0x96, 0x83, 0x3E, 0x51, 0xB1, 0x49, 0x08, 0x98, 
		0x32, 0x05, 0x59, 0x0E, 0xF9, 0x44, 0xC6, 0x26,
		0x21, 0x60, 0xC2, 0xEA, 0x79, 0x5D, 0x6D, 0xA1,
		0x54, 0x69, 0x47, 0x0C, 0xDC, 0xE8, 0x5C, 0xF1,
		0xF7, 0x76, 0x82, 0x7F, 0x07, 0x99, 0xA2, 0x2C,
		0x93, 0x7C, 0x30, 0x63, 0xF5, 0x10, 0x2E, 0x61,
		0xD0, 0xBC, 0xB4, 0xB6, 0x06, 0xAA, 0xF4, 0x23,
		0x78, 0x6E, 0x3B, 0xAE, 0xBF, 0x7B, 0x4C, 0xC1
	};

	GPS_data ecef_to_gps() const;
	
	uint8_t vaisala_descramble(uint32_t pos) const;

	const baseband::Packet packet_;
	const BiphaseMDecoder decoder_;
	const FieldReader<BiphaseMDecoder, BitRemapNone> reader_bi_m;
	Type type_;

	using packetReader = FieldReader<baseband::Packet, BitRemapByteReverse>; //baseband::Packet instead of BiphaseMDecoder

	bool crc_ok_M10() const;
	bool crc_ok_RS41() const;
	bool crc16rs41(uint32_t field_start) const;
};

} /* namespace sonde */

#endif/*__SONDE_PACKET_H__*/