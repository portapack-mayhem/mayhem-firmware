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
#include "sine_table.hpp"

#include <math.h>

namespace adsb {

void make_frame_adsb(ADSBFrame& frame, const uint32_t ICAO_address) {
	frame.clear();
	frame.push_byte((DF_ADSB << 3) | 5);	// DF and CA
	frame.push_byte(ICAO_address >> 16);
	frame.push_byte(ICAO_address >> 8);
	frame.push_byte(ICAO_address & 0xFF);
}

void encode_frame_id(ADSBFrame& frame, const uint32_t ICAO_address, const std::string& callsign) {
	std::string callsign_formatted(8, '_');
	uint64_t callsign_coded = 0;
	uint32_t c, s;
	char ch;
	
	make_frame_adsb(frame, ICAO_address);
	
	frame.push_byte(TC_IDENT << 3);		// No aircraft category
	
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

std::string decode_frame_id(ADSBFrame& frame) {
	std::string callsign = "";
	uint8_t * raw_data = frame.get_raw_data();
	uint64_t callsign_coded = 0;
	uint32_t c;
	
	// Frame bytes to long
	for (c = 5; c < 11; c++) {
		callsign_coded <<= 8;
		callsign_coded |= raw_data[c];
	}
	
	// Long to 6-bit characters
	for (c = 0; c < 8; c++) {
		callsign.append(1, icao_id_lut[(callsign_coded >> 42) & 0x3F]);
		callsign_coded <<= 6;
	}
	
	return callsign;
}

/*void generate_frame_emergency(ADSBFrame& frame, const uint32_t ICAO_address, const uint8_t code) {
	make_frame_mode_s(frame, ICAO_address);
	
	frame.push_byte((28 << 3) + 1);	// TC = 28 (Emergency), subtype = 1 (Emergency)
	frame.push_byte(code << 5);
	
	frame.make_CRC();
}*/

void encode_frame_squawk(ADSBFrame& frame, const uint32_t squawk) {
	uint32_t squawk_coded;
	
	frame.clear();
	
	frame.push_byte(DF_EHS_SQUAWK << 3);	// DF
	frame.push_byte(0);
	
	// 12 11 10  9  8  7  6  5  4  3  2  1  0
	// 31 30 29 28 27 26 25 24 23 22 21 20 19
	// D4 B4 D2 B2 D1 B1 __ A4 C4 A2 C2 A1 C1
	// ABCD = code (octal, 0000~7777)
	
	// FEDCBA9876543210
	// xAAAxBBBxCCCxDDD
	// x421x421x421x421
	
	squawk_coded = ((squawk << 10) & 0x1000) |	// D4
					((squawk << 1) & 0x0800) |	// B4
					((squawk << 9) & 0x0400) |	// D2
					((squawk << 0) & 0x0200) |	// B2
					((squawk << 8) & 0x0100) |	// D1
					((squawk >> 1) & 0x0080) |	// B1
					
					((squawk >> 9) & 0x0020) |	// A4
					((squawk >> 2) & 0x0010) |	// C4
					((squawk >> 10) & 0x0008) |	// A2
					((squawk >> 3) & 0x0004) |	// C2
					((squawk >> 11) & 0x0002) |	// A1
					((squawk >> 4) & 0x0001);	// C1
	
	frame.push_byte(squawk_coded >> 5);
	frame.push_byte(squawk_coded << 3);
	
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

float cpr_Dlon(float lat, int is_odd) {
    return 360.0 / cpr_N(lat, is_odd);
}

void encode_frame_pos(ADSBFrame& frame, const uint32_t ICAO_address, const int32_t altitude,
	const float latitude, const float longitude, const uint32_t time_parity) {
	
	uint32_t altitude_coded;
	uint32_t lat, lon;
	float delta_lat, yz, rlat, delta_lon, xz;
	
	make_frame_adsb(frame, ICAO_address);
	
	frame.push_byte(TC_AIRBORNE_POS << 3);		// Bits 2~1: Surveillance Status, bit 0: NICsb
	
	altitude_coded = (altitude + 1000) / 25;	// 25ft precision, insert Q-bit (1)
	altitude_coded = ((altitude_coded & 0x7F0) << 1) | 0x10 | (altitude_coded & 0x0F);
	
	frame.push_byte(altitude_coded >> 4);		// Top-most altitude bits
	
	// CPR encoding
	// Info from: http://antena.fe.uni-lj.si/literatura/Razno/Avionika/modes/CPRencoding.pdf
	
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
	
	frame.push_byte((altitude_coded << 4) | ((uint32_t)time_parity << 2) | (lat >> 15));	// T = 0
	frame.push_byte(lat >> 7);
	frame.push_byte((lat << 1) | (lon >> 16));
	frame.push_byte(lon >> 8);
	frame.push_byte(lon);
	
	frame.make_CRC();
}

// Decoding method from dump1090
adsb_pos decode_frame_pos(ADSBFrame& frame_even, ADSBFrame& frame_odd) {
	uint8_t * raw_data;
	uint32_t latcprE, latcprO, loncprE, loncprO;
	float latE, latO, m, Dlon, cpr_lon_odd, cpr_lon_even, cpr_lat_odd, cpr_lat_even;
	int ni;
	adsb_pos position { false, 0, 0, 0 };
	
	uint32_t time_even = frame_even.get_rx_timestamp();
	uint32_t time_odd = frame_odd.get_rx_timestamp();
	uint8_t * frame_data_even = frame_even.get_raw_data();
	uint8_t * frame_data_odd = frame_odd.get_raw_data();
	
	// Return most recent altitude
	if (time_even > time_odd)
		raw_data = frame_data_even;
	else
		raw_data = frame_data_odd;

	// Q-bit must be present
	if (raw_data[5] & 1)
		position.altitude = ((((raw_data[5] & 0xFE) << 3) | ((raw_data[6] & 0xF0) >> 4)) * 25) - 1000;

	// Position
	latcprE = ((frame_data_even[6] & 3) << 15) | (frame_data_even[7] << 7) | (frame_data_even[8] >> 1);
	loncprE = ((frame_data_even[8] & 1) << 16) | (frame_data_even[9] << 8) | frame_data_even[10];
	
	latcprO = ((frame_data_odd[6] & 3) << 15) | (frame_data_odd[7] << 7) | (frame_data_odd[8] >> 1);
	loncprO = ((frame_data_odd[8] & 1) << 16) | (frame_data_odd[9] << 8) | frame_data_odd[10];

	// Calculate the coefficients
	cpr_lon_even = loncprE / CPR_MAX_VALUE;
	cpr_lon_odd = loncprO / CPR_MAX_VALUE;

	cpr_lat_odd = latcprO / CPR_MAX_VALUE;
	cpr_lat_even = latcprE / CPR_MAX_VALUE;

	// Compute latitude index
	float j = floor(((59.0 * cpr_lat_even) - (60.0 * cpr_lat_odd)) + 0.5);
	latE = (360.0 / 60.0) * (cpr_mod(j, 60) + cpr_lat_even);
	latO = (360.0 / 59.0) * (cpr_mod(j, 59) + cpr_lat_odd);

	if (latE >= 270) latE -= 360;
	if (latO >= 270) latO -= 360;

	// Both frames must be in the same latitude zone
	if (cpr_NL(latE) != cpr_NL(latO))
		return position;

	// Compute longitude
	if (time_even > time_odd) {
		// Use even frame
		ni = cpr_N(latE, 0);
		Dlon = 360.0 / ni;
		
		m = floor((cpr_lon_even * (cpr_NL(latE) - 1)) - (cpr_lon_odd * cpr_NL(latE)) + 0.5);
		
		position.longitude = Dlon * (cpr_mod(m, ni) + cpr_lon_even);
		
		position.latitude = latE;
	} else {
		// Use odd frame
		ni = cpr_N(latO, 1);
		Dlon = 360.0 / ni;
		
		m = floor((cpr_lon_even * (cpr_NL(latO) - 1)) - (cpr_lon_odd * cpr_NL(latO)) + 0.5);
		
		position.longitude = Dlon * (cpr_mod(m, ni) + cpr_lon_odd);
		
		position.latitude = latO;
	}
	
	if (position.longitude > 180) position.longitude -= 360;
	
	position.valid = true;

	return position;
}

// speed is in knots
// vertical rate is in ft/min
void encode_frame_velo(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t speed,
	const float angle, const int32_t v_rate) {
	
	int32_t velo_ew, velo_ns, v_rate_coded;
	uint32_t velo_ew_abs, velo_ns_abs, v_rate_coded_abs;
	
	// To get NS and EW speeds from speed and bearing, a polar to cartesian conversion is enough
	velo_ew = static_cast<int32_t>(sin_f32(DEG_TO_RAD(angle) + (pi / 2)) * speed);
	velo_ns = static_cast<int32_t>(sin_f32(DEG_TO_RAD(angle)) * speed);
	
	v_rate_coded = (v_rate / 64) + 1;
	
	velo_ew_abs = abs(velo_ew) + 1; 
	velo_ns_abs = abs(velo_ns) + 1;
	v_rate_coded_abs = abs(v_rate_coded);
	
	make_frame_adsb(frame, ICAO_address);
	
	frame.push_byte((TC_AIRBORNE_VELO << 3) | 1);		// Subtype: 1 (subsonic)
	frame.push_byte(((velo_ew < 0 ? 1 : 0) << 2) | (velo_ew_abs >> 8));
	frame.push_byte(velo_ew_abs);
	frame.push_byte(((velo_ns < 0 ? 1 : 0) << 7) | (velo_ns_abs >> 3));
	frame.push_byte((velo_ns_abs << 5) | ((v_rate_coded < 0 ? 1 : 0) << 3) | (v_rate_coded_abs >> 6));	// VrSrc = 0
	frame.push_byte(v_rate_coded_abs << 2);
	frame.push_byte(0);
	
	frame.make_CRC();
}

// Decoding method from dump1090
adsb_vel decode_frame_velo(ADSBFrame& frame){
	adsb_vel velo {false, 0, 0};

	uint8_t * frame_data = frame.get_raw_data();
	uint8_t velo_type = frame.get_msg_sub();

	if(velo_type >= 1 && velo_type <= 4){ //vertical rate is always present

		velo.v_rate = (((frame_data[8] & 0x07 ) << 6) | ((frame_data[9]) >> 2) - 1) * 64;

		if((frame_data[8] & 0x8) >> 3) velo.v_rate *= -1; //check v_rate sign
	}

	if(velo_type == 1 || velo_type == 2){ //Ground Speed
		int32_t velo_ew = (((frame_data[5] & 0x03) << 8) | frame_data[6]) - 1;
		int32_t velo_ns = ((frame_data[7] & 0x7f) << 3) | ((frame_data[8]) >> 5) - 1;

		if (velo_type == 2){ // supersonic indicator so multiply by 4
			velo_ew = velo_ew << 2;
			velo_ns = velo_ns << 2;
		}

		if((frame_data[5]&4) >> 2) velo_ew *= -1; //check ew direction sign
		if((frame_data[7]&0x80) >> 7) velo_ns *= -1; //check ns direction sign

		velo.speed = sqrt(velo_ns*velo_ns + velo_ew*velo_ew);
		
		if(velo.speed){
			//calculate heading in degrees from ew/ns velocities
			int16_t heading_temp = (int16_t)(atan2(velo_ew,velo_ns) * 180.0 / pi); 
			// We don't want negative values but a 0-360 scale. 
			if (heading_temp < 0) heading_temp += 360.0;
			velo.heading = (uint16_t)heading_temp;
		}
		
	}else if(velo_type == 3 || velo_type == 4){ //Airspeed
		velo.valid = frame_data[5] & (1<<2);
		velo.heading = ((((frame_data[5] & 0x03)<<8) | frame_data[6]) * 45) << 7;
	} 

	return velo;

}

} /* namespace adsb */
