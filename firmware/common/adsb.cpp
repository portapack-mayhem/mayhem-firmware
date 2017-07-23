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

#include <math.h>

namespace adsb {

void make_frame_mode_s(ADSBFrame& frame, const uint32_t ICAO_address) {
	frame.clear();
	frame.push_byte((17 << 3) | 5);			// DF17 and CA
	frame.push_byte(ICAO_address >> 16);
	frame.push_byte(ICAO_address >> 8);
	frame.push_byte(ICAO_address & 0xFF);
}

void generate_frame_id(ADSBFrame& frame, const uint32_t ICAO_address, const std::string& callsign) {
	std::string callsign_formatted(8, '_');
	uint64_t callsign_coded = 0;
	uint32_t c, s;
	char ch;
	
	make_frame_mode_s(frame, ICAO_address);
	
	frame.push_byte(4 << 3);	// TC = 4: Aircraft ID
	
	// Translate and encode callsign
	for (c = 0; c < 8; c++) {
		ch = callsign[c];
		
		for (s = 0; s < 64; s++)
			if (ch == icao_id_lut[s]) break;
		
		if (s == 64) {
			ch = ' ';		// Invalid character
			s = 32;
		}
		
		callsign_coded <<= 6;
		callsign_coded |= s;
		
		//callsign[c] = ch;
	}
	
	// Insert callsign in frame
	for (c = 0; c < 6; c++)
		frame.push_byte((callsign_coded >> ((5 - c) * 8)) & 0xFF);
	
	frame.make_CRC();
}

/*void generate_frame_emergency(ADSBFrame& frame, const uint32_t ICAO_address, const uint8_t code) {
	make_frame_mode_s(frame, ICAO_address);
	
	frame.push_byte((28 << 3) + 1);	// TC = 28 (Emergency), subtype = 1 (Emergency)
	frame.push_byte(code << 5);
	
	frame.make_CRC();
}*/

void generate_frame_identity(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t code) {
	frame.clear();
	
	frame.push_byte(21 << 3);		// DF = 21
	frame.push_byte(0);
	
	// 1337:
	frame.push_byte(0b00011100);
	frame.push_byte(0b00111101);
	
	frame.make_CRC();
}

float cpr_mod(float a, float b) {
    return a - (b * floor(a / b));
}

int cpr_NL(float lat) {
    if (lat < 0)
		lat = -lat;		// Symmetry
	
	for (size_t c = 0; c < 58; c++) {
		if (lat < adsb_lat_lut[c])
			return 59 - c;
	}
	
    return 1;
}

int cpr_N(float lat, int is_odd) {
    int nl = cpr_NL(lat) - is_odd;
    
    if (nl < 1)
		nl = 1;
	
    return nl;
}

void generate_frame_pos(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t altitude,
	const float latitude, const float longitude, const uint32_t time_parity) {
	uint32_t altitude_coded;
	uint32_t lat, lon;
	float delta_lat, yz, rlat, delta_lon, xz;
	
	make_frame_mode_s(frame, ICAO_address);
	
	frame.push_byte(11 << 3);		// TC = 11 (Airborne position)
	
	altitude_coded = (altitude + 1000) / 25;	// Can be coded in 100ft steps also
	frame.push_byte(altitude_coded >> 4);		// Top-most altitude bits
	
	// Decoding method (from dump1090):
	// index int j = floor(((59 * latcprE - 60 * latcprO) / 131072) + 0.50)
	// latE = DlatE * (cpr_mod(j, 60) + (latcprE / 131072))
	// latO = DlatO * (cpr_mod(j, 59) + (latcprO / 131072))
	// if latE >= 270 -> latE -= 360
	// if latO >= 270 -> latO -= 360
	// if (cpr_NL(latE) != cpr_NL(latO)) return;
	
	// int ni = cpr_N(latE ,0);
	// int m = floor((((loncprE * (cpr_NL(latE) - 1)) - (loncprO * cpr_NL(latE))) / 131072) + 0.5)
	// lon = cpr_Dlon(latE, 0) * (cpr_mod(m, ni) + loncprE / 131072);
	// lat = latE;
	// ... or ...
	// int ni = cpr_N(latO ,0);
	// int m = floor((((loncprE * (cpr_NL(latO) - 1)) - (loncprO * cpr_NL(latO))) / 131072) + 0.5)
	// lon = cpr_Dlon(latO, 0) * (cpr_mod(m, ni) + loncprO / 131072);
	// lat = latO;
	// ... and ...
	// if (lon > 180) lon -= 360;
	
	// CPR encoding
	// Info: http://antena.fe.uni-lj.si/literatura/Razno/Avionika/modes/CPRencoding.pdf
	
	delta_lat = 360.0 / ((4.0 * 15.0) - time_parity);		// NZ = 15
	yz = floor(131072.0 * (cpr_mod(latitude, delta_lat) / delta_lat) + 0.5);
	rlat = delta_lat * ((yz / 131072.0) + floor(latitude / delta_lat));
	
	if ((cpr_NL(rlat) - time_parity) > 0)
		delta_lon = 360.0 / cpr_N(rlat, time_parity);
	else
		delta_lon = 360.0;
	xz = floor(131072.0 * (cpr_mod(longitude, delta_lon) / delta_lon) + 0.5);
	
	lat = cpr_mod(yz, 131072.0);
	lon = cpr_mod(xz, 131072.0);
	
	frame.push_byte((altitude_coded << 4) | ((uint32_t)time_parity << 2) | (lat >> 15));
	frame.push_byte(lat >> 7);
	frame.push_byte((lat << 1) | (lon >> 16));
	frame.push_byte(lon >> 8);
	frame.push_byte(lon);
	
	frame.make_CRC();
}

} /* namespace adsb */
