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

#ifndef __ADSB_H__
#define __ADSB_H__

#include "adsb_frame.hpp"
#include "ui.hpp"

#include <cstring>
#include <string>

namespace adsb {

enum downlink_format {
	DF_ADSB = 17,
	DF_EHS_SQUAWK = 21
};

enum type_code {
	TC_IDENT = 4,
	TC_AIRBORNE_POS = 11,
	TC_AIRBORNE_VELO = 19
};

enum data_selector {
	BDS_ID = 0x20,
	BDS_ID_MARKS = 0x21,
	BDS_INTENT = 0x40,
	BDS_HEADING = 0x60
};

struct adsb_pos {
	bool valid;
	float latitude;
	float longitude;
	int32_t altitude;
};

struct adsb_vel {
	bool valid;
	int32_t speed;  //knot
	uint16_t heading; //degree
	int32_t v_rate; //ft/min
};

const float CPR_MAX_VALUE = 131072.0;

const float adsb_lat_lut[58] = {
	10.47047130,    14.82817437,    18.18626357,    21.02939493,
    23.54504487,    25.82924707,    27.93898710,    29.91135686,
    31.77209708,    33.53993436,    35.22899598,    36.85025108,
    38.41241892,    39.92256684,    41.38651832,    42.80914012,
    44.19454951,    45.54626723,    46.86733252,    48.16039128,
    49.42776439,    50.67150166,    51.89342469,    53.09516153,
    54.27817472,    55.44378444,    56.59318756,    57.72747354,
    58.84763776,    59.95459277,    61.04917774,    62.13216659,
    63.20427479,    64.26616523,    65.31845310,    66.36171008,
    67.39646774,    68.42322022,    69.44242631,    70.45451075,
    71.45986473,    72.45884545,    73.45177442,    74.43893416,
    75.42056257,    76.39684391,    77.36789461,    78.33374083,
    79.29428225,    80.24923213,    81.19801349,    82.13956981,
    83.07199445,    83.99173563,    84.89166191,    85.75541621,
    86.53536998,    87.00000000
};

void make_frame_adsb(ADSBFrame& frame, const uint32_t ICAO_address);

void encode_frame_id(ADSBFrame& frame, const uint32_t ICAO_address, const std::string& callsign);
std::string decode_frame_id(ADSBFrame& frame);

void encode_frame_pos(ADSBFrame& frame, const uint32_t ICAO_address, const int32_t altitude,
	const float latitude, const float longitude, const uint32_t time_parity);

adsb_pos decode_frame_pos(ADSBFrame& frame_even, ADSBFrame& frame_odd);

void encode_frame_velo(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t speed,
	const float angle, const int32_t v_rate);

adsb_vel decode_frame_velo(ADSBFrame& frame);

//void encode_frame_emergency(ADSBFrame& frame, const uint32_t ICAO_address, const uint8_t code);

void encode_frame_squawk(ADSBFrame& frame, const uint32_t squawk);

} /* namespace adsb */

#endif/*__ADSB_H__*/
