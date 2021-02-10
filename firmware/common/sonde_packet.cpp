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

//Following values include the 4 bytes less shift, consumed in detecting the header on proc_sonde
#define block_status  0x35	//0x039  // 40 bytes
#define block_gpspos  0x10E	//0x112  // 21 bytes
#define block_meas    0x61	//0x65  // 42 bytes
#define pos_FrameNb   0x37	//0x03B  // 2 byte
#define pos_SondeID   0x39	//0x03D  // 8 byte
#define pos_Voltage   0x041	//0x045  // 3 bytes (but first one is the important one) voltage x 10 ie: 26 = 2.6v
#define pos_CalData   0x04E	//0x052  // 1 byte, counter 0x00..0x32
#define pos_temp	  0x063	//0x067  // 3 bytes (uint24_t)
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

Timestamp Packet::received_at() const {
	return packet_.timestamp();
}

Packet::Type Packet::type() const {
	return type_;
}

//euquiq here:
//RS41SG 320 bits header, 320bytes frame (or more if it is an "extended frame")
//The raw data is xor-scrambled with the values in the 64 bytes vaisala_mask (see.hpp)
//from 0x008 to 0x037 (48 bytes reed-solomon error correction data)

uint8_t Packet::vaisala_descramble(const uint32_t pos) const
{ //vaisala_descramble(const uint32_t pos) const {
	// packet_[i]; its a bit;  packet_.size the total (should be 2560 bits)
	uint8_t value = 0;
	for (uint8_t i = 0; i < 8; i++)
		value = (value << 1) | packet_[(pos * 8) + (7 - i)]; //get the byte from the bits collection

	//packetReader reader { packet_ };				//This works just as above.
	//value = reader.read(pos * 8,8);
	//shift pos because first 4 bytes are consumed by proc_sonde in finding the vaisala signature
	uint32_t mask_pos = pos + 4;
	value = value ^ vaisala_mask[mask_pos % MASK_LEN]; //descramble with the xor pseudorandom table
	return value;
};

GPS_data Packet::get_GPS_data() const
{
	GPS_data result;
	if ((type_ == Type::Meteomodem_M10) || (type_ == Type::Meteomodem_M2K2))
	{

		result.alt = (reader_bi_m.read(22 * 8, 32) / 1000) - 48;
		result.lat = reader_bi_m.read(14 * 8, 32) / ((1ULL << 32) / 360.0);
		result.lon = reader_bi_m.read(18 * 8, 32) / ((1ULL << 32) / 360.0);
	}
	else if (type_ == Type::Vaisala_RS41_SG)
	{

		uint8_t XYZ_bytes[4];
		int32_t XYZ; // 32bit
		double_t X[3];
		for (int32_t k = 0; k < 3; k++)
		{									//Get X,Y,Z ECEF position from GPS
			for (int32_t i = 0; i < 4; i++) //each one is 4 bytes (32 bits)
				XYZ_bytes[i] = vaisala_descramble(pos_GPSecefX + (4 * k) + i);
			memcpy(&XYZ, XYZ_bytes, 4);
			X[k] = XYZ / 100.0;
		}

		double_t a = 6378137.0;
		double_t b = 6356752.31424518;
		double_t e = sqrt((a * a - b * b) / (a * a));
		double_t ee = sqrt((a * a - b * b) / (b * b));

		double_t lam = atan2(X[1], X[0]);
		double_t p = sqrt(X[0] * X[0] + X[1] * X[1]);
		double_t t = atan2(X[2] * a, p * b);
		double_t phi = atan2(X[2] + ee * ee * b * sin(t) * sin(t) * sin(t),
							 p - e * e * a * cos(t) * cos(t) * cos(t));

		double_t R = a / sqrt(1 - e * e * sin(phi) * sin(phi));

		result.alt = p / cos(phi) - R;
		result.lat = phi * 180 / PI;
		result.lon = lam * 180 / PI;
	}
	return result;
}

uint32_t Packet::battery_voltage() const
{
	if (type_ == Type::Meteomodem_M10)
		return (reader_bi_m.read(69 * 8, 8) + (reader_bi_m.read(70 * 8, 8) << 8)) * 1000 / 150;
	else if (type_ == Type::Meteomodem_M2K2)
		return reader_bi_m.read(69 * 8, 8) * 66; // Actually 65.8
	else if (type_ == Type::Vaisala_RS41_SG)
	{
		uint32_t voltage = vaisala_descramble(pos_Voltage) * 100; //byte 69 = voltage * 10 (check if this value needs to be multiplied)
		return voltage;
	}
	else
	{
		return 0; // Unknown
	}
}

uint32_t Packet::frame() const
{
	if (type_ == Type::Vaisala_RS41_SG)
	{
		uint32_t frame_number = vaisala_descramble(pos_FrameNb) | (vaisala_descramble(pos_FrameNb + 1) << 8);
		return frame_number;
	}
	else
	{
		return 0; // Unknown
	}
}

temp_humid Packet::get_temp_humid() const
{
	temp_humid result;
	result.humid = 0;
	result.temp = 0;

	if ( type_ == Type::Vaisala_RS41_SG && crc_ok_RS41() )  //Only process if packet is healthy
	{
		//memset(calfrchk, 0, 51); // is this necessary ? only if the sondeID changes (new sonde)
		//original code from https://github.com/rs1729/RS/blob/master/rs41/rs41ptu.c
		float Rf1,      // ref-resistor f1 (750 Ohm)
			Rf2,      // ref-resistor f2 (1100 Ohm)
			co1[3],   // { -243.911 , 0.187654 , 8.2e-06 }
			calT1[3], // calibration T1
			co2[3],   // { -243.911 , 0.187654 , 8.2e-06 }
			calT2[3], // calibration T2-Hum
			calH[2];  // calibration Hum

		uint32_t meas[12], i;

		//-------------- get_CalData

		//-------------- populate calibytes (from getFrameConf)

		uint8_t calfr = vaisala_descramble(pos_CalData); //get subframe #slot

		for (i = 0; i < 16; i++) //Load subrfame calibration page (16 bytes) into #slot
			calibytes[calfr * 16 + i] = vaisala_descramble(pos_CalData + 1 + i); //pos = pos_CalData + 1 + i ; vaisala_descramble(pos)

		calfrchk[calfr] = 1; //flag this #slot as populated

		memcpy(&Rf1, calibytes + 61, 4); // 0x03*0x10+13
		memcpy(&Rf2, calibytes + 65, 4); // 0x04*0x10+ 1

		memcpy(co1 + 0, calibytes + 77, 4); // 0x04*0x10+13
		memcpy(co1 + 1, calibytes + 81, 4); // 0x05*0x10+ 1
		memcpy(co1 + 2, calibytes + 85, 4); // 0x05*0x10+ 5

		memcpy(calT1 + 0, calibytes + 89, 4); // 0x05*0x10+ 9
		memcpy(calT1 + 1, calibytes + 93, 4); // 0x05*0x10+13
		memcpy(calT1 + 2, calibytes + 97, 4); // 0x06*0x10+ 1

		memcpy(calH + 0, calibytes + 117, 4); // 0x07*0x10+ 5
		memcpy(calH + 1, calibytes + 121, 4); // 0x07*0x10+ 9

		memcpy(co2 + 0, calibytes + 293, 4); // 0x12*0x10+ 5
		memcpy(co2 + 1, calibytes + 297, 4); // 0x12*0x10+ 9
		memcpy(co2 + 2, calibytes + 301, 4); // 0x12*0x10+13

		memcpy(calT2 + 0, calibytes + 305, 4); // 0x13*0x10+ 1
		memcpy(calT2 + 1, calibytes + 309, 4); // 0x13*0x10+ 5
		memcpy(calT2 + 2, calibytes + 313, 4); // 0x13*0x10+ 9
		//---------------------------------------
		for (i = 0; i < 12; i++)
			meas[i] = vaisala_descramble(pos_temp + (3 * i)) |
					(vaisala_descramble(pos_temp + (3 * i) + 1) << 8) |
					(vaisala_descramble(pos_temp + (3 * i) + 2) << 16);

		//----Check if necessary calibytes are already present for calculation

		if (calfrchk[0x03] && calfrchk[0x04] && calfrchk[0x04] && calfrchk[0x05] && calfrchk[0x05] && calfrchk[0x06]) //Calibites OK for Temperature
		{
			//----------get_Tc------------------------
			float *p = co1;
			float *c = calT1;
			float g = (float)(meas[2] - meas[1]) / (Rf2 - Rf1),					   // gain
				Rb = (meas[1] * Rf2 - meas[2] * Rf1) / (float)(meas[2] - meas[1]), // ofs
				Rc = meas[0] / g - Rb,
				R = Rc * c[0],
				T = (p[0] + p[1] * R + p[2] * R * R + c[1]) * (1.0 + c[2]);
			result.temp = T;
		}

		if (calfrchk[0x07])
		{
			//----------get_RH------------------------
			float a0 = 7.5;				// empirical
			float a1 = 350.0 / calH[0]; // empirical
			float fh = (meas[3] - meas[4]) / (float)(meas[5] - meas[4]);
			float rh = 100.0 * (a1 * fh - a0);
			float T0 = 0.0, T1 = -25.0;	  // T/C
			rh += T0 - result.temp / 5.5; // empir. temperature compensation
			if (result.temp < T1)
				rh *= 1.0 + (T1 - result.temp) / 90.0; // empir. temperature compensation
			if (rh < 0.0)
				rh = 0.0;
			if (rh > 100.0)
				rh = 100.0;
			if (result.temp < -273.0)
				rh = -1.0;
			result.humid = rh;
		}
	}
	return result;
}

std::string Packet::type_string() const
{
	switch (type_)
	{
	case Type::Unknown:
		return "Unknown";
	case Type::Meteomodem_unknown:
		return "Meteomodem ???";
	case Type::Meteomodem_M10:
		return "Meteomodem M10";
	case Type::Meteomodem_M2K2:
		return "Meteomodem M2K2";
	case Type::Vaisala_RS41_SG:
		return "Vaisala RS41-SG";
	default:
		return "? 0x" + symbols_formatted().data.substr(0, 6);
	}
}

std::string Packet::serial_number() const
{
	if (type_ == Type::Meteomodem_M10)
	{
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
	}
	else if (type_ == Type::Vaisala_RS41_SG)
	{
		std::string serial_id = "";
		uint8_t achar;
		for (uint8_t i = 0; i < 8; i++)
		{ //euquiq: Serial ID is 8 bytes long, each byte a char
			achar = vaisala_descramble(pos_SondeID + i);
			if (achar < 32 || achar > 126)
				return "?"; //Maybe there are ids with less than 8 bytes and this is not OK.
			serial_id += (char)achar;
		}
		return serial_id;
	}
	else
	{
		return "?";
	}
}

FormattedSymbols Packet::symbols_formatted() const
{
	if (type_ == Type::Vaisala_RS41_SG)
	{										 //Euquiq: now we distinguish different types
		uint32_t bytes = packet_.size() / 8; //Need the byte amount, which if full, it SHOULD be 320 size() should return 2560
		std::string hex_data;
		std::string hex_error;
		hex_data.reserve(bytes * 2); //2 hexa chars per byte
		hex_error.reserve(1);
		for (uint32_t i = 0; i < bytes; i++) //log will show the packet starting on the last 4 bytes from signature 93DF1A60
			hex_data += to_string_hex(vaisala_descramble(i), 2);
		return {hex_data, hex_error};
	}
	else
	{
		return format_symbols(decoder_);
	}
}

bool Packet::crc_ok() const
{
	switch (type_)
	{
	case Type::Meteomodem_M10:
		return crc_ok_M10();
	case Type::Vaisala_RS41_SG:
		return crc_ok_RS41();
	default:
		return true; //euquiq: it was false, but if no crc routine, then no way to check
	}
}

//each data block has a 2 byte header, data, and 2 byte tail: 
// 1st byte: block ID
// 2nd byte: data length (without header or tail)
// <data>
// 2 bytes CRC16 over the data.
bool Packet::crc_ok_RS41() const	//check CRC for the data blocks we need
{
	if (!crc16rs41(block_status))
		return false;

	if (!crc16rs41(block_gpspos))
		return false;
	
	if (!crc16rs41(block_meas))
		return false;

	return true;
}

//Checks CRC16 on a RS41 field:
bool Packet::crc16rs41(uint32_t field_start) const
{
	int crc16poly = 0x1021;
	int rem = 0xFFFF, b, j;
	int xbyte;
	uint32_t pos = field_start + 1;
	uint8_t length = vaisala_descramble(pos);

	if (pos + length + 2 > packet_.size() / 8)
		return false; //Out of packet!

	for (b = 0; b < length; b++)
	{
		pos++;
		xbyte = vaisala_descramble(pos);
		rem = rem ^ (xbyte << 8);
		for (j = 0; j < 8; j++)
		{
			if (rem & 0x8000)
			{
				rem = (rem << 1) ^ crc16poly;
			}
			else
			{
				rem = (rem << 1);
			}
			rem &= 0xFFFF;
		}
	}
	//Check calculated CRC against packet's one
	pos++;
	int crcok = vaisala_descramble(pos) | (vaisala_descramble(pos + 1) << 8);
	if (crcok != rem)
		return false;
	return true;
}

bool Packet::crc_ok_M10() const
{
	uint16_t cs{0};
	uint32_t c0, c1, t, t6, t7, s, b;

	for (size_t i = 0; i < packet_.size(); i++)
	{
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

		cs = ((c1 << 8) | c0) & 0xFFFF;
	}
	
	return ((cs & 0xFFFF) == ((packet_[0x63] << 8) | (packet_[0x63 + 1])));
}

} /* namespace sonde */
