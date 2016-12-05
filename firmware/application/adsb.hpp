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

namespace adsb {

	const char icao_id_lut[65] = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ##### ###############0123456789######";
	
	void make_frame_mode_s(uint8_t * adsb_frame, uint32_t ICAO_address);
	
	void generate_frame_id(uint8_t * adsb_frame, uint32_t ICAO_address, char * callsign);
	void generate_frame_pos(uint8_t * adsb_frame, uint32_t ICAO_address, uint32_t altitude, float latitude, float longitude);
	
	void ADSB_generate_CRC(uint8_t * in_message);

} /* namespace adsb */

#endif/*__ADSB_H__*/
