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
#include <cstring>
//#include <complex>

namespace sonde {

//Defines for Vaisala RS41, from https://github.com/rs1729/RS/blob/master/rs41/rs41sg.c
#define MASK_LEN 64
#define pos_FrameNb   0x37	//0x03B  // 2 byte
#define pos_SondeID   0x39	//0x03D  // 8 byte
#define pos_Voltage   0x041	//0x045  // 3 bytes (but first one is the important one) voltage x 10 ie: 26 = 2.6v
#define pos_CalData   0x04E	//0x052  // 1 byte, counter 0x00..0x32
#define pos_GPSweek   0x091	//0x095  // 2 byte
#define pos_GPSTOW    0x093	//0x097  // 4 byte
#define pos_GPSecefX  0x110	//0x114  // 4 byte
#define pos_GPSecefY  0x114	//0x118  // 4 byte (not actually used since Y and Z are following X, and grabbed in that same loop)
#define pos_GPSecefZ  0x118	//0x11C  // 4 byte (same as Y)

#define PI 3.1415926535897932384626433832795  //3.1416 //(3.1415926535897932384626433832795)

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

//euquiq here:
//RS41SG 320 bits header, 320bytes frame (or more if it is an "extended frame")
//The raw data is xor-scrambled with the values in the 64 bytes vaisala_mask (see.hpp)


uint8_t Packet::vaisala_descramble(const uint32_t pos) const {
	//return reader_raw.read(pos * 8, 8) ^ vaisala_mask[pos & 63];  
	// packet_[i]; its a bit;  packet_.size the total (should be 2560 bits)
	uint8_t value = 0;
	for (uint8_t i = 0; i < 8; i++) 
		value = (value << 1) | packet_[(pos * 8) + (7 -i)];	//get the byte from the bits collection

	//packetReader reader { packet_ };				//This works just as above.
	//value = reader.read(pos * 8,8);
	//shift pos because first 4 bytes are consumed by proc_sonde in finding the vaisala signature
	uint32_t mask_pos = pos + 4;
	value = value ^ vaisala_mask[mask_pos % MASK_LEN];	//descramble with the xor pseudorandom table
	return value;
};

GPS_data Packet::get_GPS_data() const {
	GPS_data result;
	if ((type_ == Type::Meteomodem_M10) || (type_ == Type::Meteomodem_M2K2)) {

		result.alt = (reader_bi_m.read(22 * 8, 32) / 1000) - 48;
		result.lat = reader_bi_m.read(14 * 8, 32) / ((1ULL << 32) / 360.0);
		result.lon = reader_bi_m.read(18 * 8, 32) / ((1ULL << 32) / 360.0);

	} else if (type_ == Type::Vaisala_RS41_SG) {

		uint8_t XYZ_bytes[4];
		int32_t XYZ; // 32bit
		double_t X[3];
		for (int32_t k = 0; k < 3; k++) {		//Get X,Y,Z ECEF position from GPS
			for (int32_t i = 0; i < 4; i++)		//each one is 4 bytes (32 bits)
				XYZ_bytes[i] = vaisala_descramble(pos_GPSecefX + (4*k) + i);
			memcpy(&XYZ, XYZ_bytes, 4);
			X[k] = XYZ / 100.0;
		}

		double_t a = 6378137.0;
		double_t b = 6356752.31424518;
		double_t e  = sqrt( (a*a - b*b) / (a*a) );
		double_t ee = sqrt( (a*a - b*b) / (b*b) );

		double_t lam = atan2( X[1] , X[0] );
		double_t p = sqrt( X[0]*X[0] + X[1]*X[1] );
		double_t t = atan2( X[2]*a , p*b );
		double_t phi = atan2( X[2] + ee*ee * b * sin(t)*sin(t)*sin(t) ,
					p - e*e * a * cos(t)*cos(t)*cos(t) );

		double_t R = a / sqrt( 1 - e*e*sin(phi)*sin(phi) );

		result.alt = p / cos(phi) - R;
		result.lat = phi*180/PI;
		result.lon = lam*180/PI;

	}
	return result;
}

uint32_t Packet::battery_voltage() const {
	if (type_ == Type::Meteomodem_M10)
		return (reader_bi_m.read(69 * 8, 8) + (reader_bi_m.read(70 * 8, 8) << 8)) * 1000 / 150;
	else if (type_ == Type::Meteomodem_M2K2)
		return reader_bi_m.read(69 * 8, 8) * 66;	// Actually 65.8
	 else if (type_ == Type::Vaisala_RS41_SG) {
		uint32_t voltage = vaisala_descramble(pos_Voltage) * 100; 	//byte 69 = voltage * 10 (check if this value needs to be multiplied)
		return voltage;
	 }
	else {
		return 0;	// Unknown
	}		
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
	
	} else if(type() == Type::Vaisala_RS41_SG) {
		std::string serial_id = "";
		uint8_t achar;
		for (uint8_t i=0; i<8; i++) {	//euquiq: Serial ID is 8 bytes long, each byte a char
			achar = vaisala_descramble(pos_SondeID + i);
			if (achar < 32 || achar > 126) return "?"; //Maybe there are ids with less than 8 bytes and this is not OK.
			serial_id += (char)achar;
		}
		return serial_id;
	} else 
		return "?";
}

FormattedSymbols Packet::symbols_formatted() const {
	if (type() == Type::Vaisala_RS41_SG) {	//Euquiq: now we distinguish different types
		uint32_t bytes = packet_.size() / 8;  //Need the byte amount, which if full, it SHOULD be 320 size() should return 2560
		std::string hex_data;
		std::string hex_error;
		hex_data.reserve(bytes * 2); //2 hexa chars per byte
		hex_error.reserve(1);				
		for (uint32_t i=0; i < bytes; i++) //log will show the packet starting on the last 4 bytes from signature 93DF1A60
			hex_data += to_string_hex(vaisala_descramble(i),2);
		return { hex_data, hex_error };

	} else {
		return format_symbols(decoder_);
	}
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
