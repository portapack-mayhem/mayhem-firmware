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

#include "adsb.hpp"

#include "portapack_persistent_memory.hpp"

namespace adsb {

void make_frame_mode_s(uint8_t * const adsb_frame, const uint32_t ICAO_address) {
	adsb_frame[0] = (17 << 3) | 5;		// DF and CA
	adsb_frame[1] = ICAO_address >> 16;
	adsb_frame[2] = (ICAO_address >> 8) & 0xFF;
	adsb_frame[3] = ICAO_address & 0xFF;
	
	memset(&adsb_frame[4], 0, 10);
}

void generate_frame_id(uint8_t * const adsb_frame, const uint32_t ICAO_address, std::string & callsign) {
	std::string callsign_formatted(8, '_');
	uint64_t callsign_coded = 0;
	uint32_t c, s;
	char ch;
	
	make_frame_mode_s(adsb_frame, ICAO_address);
	
	adsb_frame[4] = 4 << 3;	// TC = 4: Aircraft ID
		
	// Translate and encode callsign
	for (c = 0; c < 8; c++) {
		ch = callsign[c];
		
		for (s = 0; s < 64; s++)
			if (ch == icao_id_lut[s]) break;
		
		if (s >= 64) {
			ch = ' ';		// Invalid character
			s = 32;
		}
		callsign_coded |= ((uint64_t)s << ((7 - c) * 6));
		callsign[c] = ch;
	}
	
	// Insert callsign in frame
	for (c = 0; c < 6; c++)
		adsb_frame[c + 5] = (callsign_coded >> ((5 - c) * 8)) & 0xFF;
	
	ADSB_generate_CRC(adsb_frame);
}

void generate_frame_emergency(uint8_t * const adsb_frame, const uint32_t ICAO_address, const uint8_t code) {
	make_frame_mode_s(adsb_frame, ICAO_address);
	
	adsb_frame[4] = (28 << 3) + 1;	// TC = 28 (Emergency), subtype = 1 (Emergency)
	adsb_frame[5] = code << 5;
	
	ADSB_generate_CRC(adsb_frame);
}

void generate_frame_pos(uint8_t * const adsb_frame, const uint32_t ICAO_address, const uint32_t altitude,
	const float latitude, const float longitude) {
	uint8_t c, time_parity;
	uint32_t altitude_coded;
	uint32_t LAT, LON;
	float delta_lat, yz, rlat, delta_lon, xz;
	
	LAT = 0;
	
	make_frame_mode_s(adsb_frame, ICAO_address);
	
	adsb_frame[4] = 11 << 3;	// TC = 11: Airborne position
	
	altitude_coded = (altitude + 1000) / 25;		// Can be coded in 100ft steps also
	
	// LAT:
	// index j = floor(59*latcprE-60*latcprO+0.50)
	// latE = DlatE*(mod(j,60)+latcprE)
	// latO = DlatO*(mod(j,59)+latcprO)
	// if latE >= 270 -> latE -= 360
	// if latO >= 270 -> latO -= 360
	//time_parity = 0;	// 0~1
	//delta_lat = 90.0 / (60.0 - (time_parity / 4.0));
	//yz = 524288.0 * (mod(lat, delta_lat) / delta_lat);	// Round to int !
	//rlat = delta_lat * ((yz / 524288.0) + int(lat / delta_lat));
	//delta_lon = 360.0 / (NL(rlat) - time_parity);
	//xz = 524288.0 * (mod(lon, delta_lon) / delta_lon);	// Round to int !
	/*if (time_parity) {
		A = sign(rlat0)[NL(rlat0) - NL(rlat1)];
	}*/
	// int xz and yz, then:
	// xz >>= 2;
	// yz >>= 2;
	// To get 17 bits
	
	// aaaaaaa Q bbbb
	adsb_frame[5] = ((altitude_coded & 0x7F0) >> 3) | 1;
	adsb_frame[6] = ((altitude_coded & 0x00F) << 4) | (LAT >> 15);		// Then 0, even/odd, and the 2 LAT-CPR MSBs
}

void ADSB_generate_CRC(uint8_t * const in_frame) {
	uint8_t adsb_crc[14];	// Temp buffer
	uint8_t b, c, s, bitn;
	const uint32_t crc_poly = 0x1205FFF;
	
	in_frame[11] = 0x00;	// Clear CRC
	in_frame[12] = 0x00;
	in_frame[13] = 0x00;
	
	// Compute CRC
	memcpy(adsb_crc, in_frame, 14);
	for (c = 0; c < 11; c++) {
		for (b = 0; b < 8; b++) {
			if ((adsb_crc[c] << b) & 0x80) {
				for (s = 0; s < 25; s++) {
					bitn = (c * 8) + b + s;
					if ((crc_poly >> s) & 1) adsb_crc[bitn >> 3] ^= (0x80 >> (bitn & 7));
				}
			}
		}
	}
	
	// Insert CRC in frame
	memcpy(&in_frame[11], &adsb_crc[11], 3);
	
}

} /* namespace adsb */
