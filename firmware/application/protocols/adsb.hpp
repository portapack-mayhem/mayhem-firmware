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

#include <cstring>
#include <string>

#ifndef __ADSB_H__
#define __ADSB_H__

#define ADSB_PREAMBLE_LENGTH 16

namespace adsb {

struct adsb_frame {
public:
	uint8_t get_DF();
	uint8_t get_msg_type();
	uint32_t get_ICAO_address();
	std::string get_callsign();
	
	void push_byte(uint8_t byte);
	uint8_t get_byte(uint8_t index);	// DEBUG
	void make_CRC();
	void clear();

private:
	uint8_t index { 0 };
	uint8_t raw_data[14] { };	// 112 bits at most
};

const char icao_id_lut[65] = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ##### ###############0123456789######";
const uint8_t adsb_preamble[16] = { 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0 };

void make_frame_mode_s(adsb_frame& frame, const uint32_t ICAO_address);
void generate_frame_id(adsb_frame&, const uint32_t ICAO_address, std::string& callsign);
//void generate_frame_pos(uint8_t * const adsb_frame, const uint32_t ICAO_address, const uint32_t altitude,
//	const float latitude, const float longitude);
void generate_frame_emergency(adsb_frame&, const uint32_t ICAO_address, const uint8_t code);

} /* namespace adsb */

#endif/*__ADSB_H__*/
