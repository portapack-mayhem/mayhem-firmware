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

#include <cstring>
#include <string>

namespace adsb {

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

void make_frame_mode_s(ADSBFrame& frame, const uint32_t ICAO_address);
void generate_frame_id(ADSBFrame& frame, const uint32_t ICAO_address, const std::string& callsign);
void generate_frame_pos(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t altitude,
	const float latitude, const float longitude, const uint32_t time_parity);
void generate_frame_emergency(ADSBFrame& frame, const uint32_t ICAO_address, const uint8_t code);
void generate_frame_identity(ADSBFrame& frame, const uint32_t ICAO_address, const uint32_t code);

} /* namespace adsb */

#endif/*__ADSB_H__*/
