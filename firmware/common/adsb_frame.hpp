/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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
 
#ifndef __ADSB_FRAME_H__
#define __ADSB_FRAME_H__

#include <cstring>
#include <string>

namespace adsb {

alignas(4) const uint8_t adsb_preamble[16] = { 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0 };
alignas(4) const char icao_id_lut[65] = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ##### ###############0123456789######";

class ADSBFrame {
public:
	uint8_t get_DF() {
		return (raw_data[0] >> 3);
	}

	uint8_t get_msg_type() {
		return (raw_data[4] >> 3);
	}

	uint32_t get_ICAO_address() {
		return (raw_data[1] << 16) + (raw_data[2] << 8) + raw_data[3];
	}

	void clear() {
		index = 0;
		memset(raw_data, 0, 14);
	}

	void push_byte(uint8_t byte) {
		if (index >= 14)
			return;
		
		raw_data[index++] = byte;
	}

	uint8_t * get_raw_data() {
		return raw_data;
	}

	std::string get_callsign() {
		uint64_t callsign_coded = 0;
		uint32_t c;
		std::string callsign = "";
		
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
	
	void make_CRC() {
		uint8_t adsb_crc[14];	// Temp buffer
		uint8_t b, c, s, bitn;
		const uint32_t crc_poly = 0x1205FFF;
		
		// Clear CRC
		raw_data[11] = 0x00;
		raw_data[12] = 0x00;
		raw_data[13] = 0x00;
		
		// Compute CRC
		memcpy(adsb_crc, raw_data, 14);
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
		memcpy(&raw_data[11], &adsb_crc[11], 3);
	}

private:
	static const uint8_t adsb_preamble[16];
	static const char icao_id_lut[65];
	alignas(4) uint8_t index { 0 };
	alignas(4) uint8_t raw_data[14] { };	// 112 bits at most
};

} /* namespace adsb */

#endif/*__ADSB_FRAME_H__*/
