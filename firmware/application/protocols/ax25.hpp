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

#include "crc.hpp"

#include <cstring>
#include <string>

#ifndef __AX25_H__
#define __AX25_H__

#define AX25_FLAG 0x7E

namespace ax25 {

enum protocol_id_t {
	X25_PLP = 0x01,
	COMP_TCPIP = 0x06,
	UNCOMP_TCPIP = 0x07,
	SEG_FRAG = 0x08,
	FLEXNET = 0xCE,
	NO_LAYER3 = 0xF0
};

class AX25Frame {
public:
	void make_ui_frame(char * const address, const uint8_t control, const uint8_t protocol,
						const std::string& info);
	
private:
	void NRZI_add_bit(const uint32_t bit);
	void make_extended_field(char * const data, size_t length);
	void add_byte(uint8_t byte, bool is_flag, bool is_data);
	void add_data(uint8_t byte);
	void add_checksum();
	void add_flag();
	void flush();
	
	uint16_t * bb_data_ptr { nullptr };
	uint8_t current_bit { 0 };
	uint8_t current_byte { 0 };
	size_t bit_counter { 0 };
	uint8_t ones_counter { 0 };
	
	CRC<16, true, true> crc_ccitt { 0x1021, 0xFFFF, 0xFFFF };
};

} /* namespace ax25 */

#endif/*__AX25_H__*/
