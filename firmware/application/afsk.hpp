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

#include "ui.hpp"
#include <cstring>
#include <string>

#ifndef __AFSK_H__
#define __AFSK_H__

namespace afsk {
	
#define AFSK_MODES_COUNT 4

enum parity_enum {
	NONE = 0,
	EVEN,
	ODD
};

struct afsk_formats_t {
	std::string fullname;
	std::string shortname;
	uint8_t data_bits;
	parity_enum parity;
	uint8_t stop_bits;
	bool MSB_first;
	bool use_LUT;
};

const afsk_formats_t afsk_formats[4] = {
	{ "7-Even-1 R", "7E1", 7, EVEN,	1, false, false },
	{ "7E1 LUT   ", "7Ea", 7, EVEN,	1, true, true },
	{ "7-Odd-1   ", "7o1", 7, ODD,	1, true, false },
	{ "8-Even-0  ", "8E0", 8, EVEN,	1, true, false }
};

// TODO: Complete table
const char alt_lookup[128] = {
	0, 0, 0, 0x5F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0F,		// 0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				// 10
	0xF8, 0, 0x99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 20  !"#$%&'()*+,-./
	0xF5, 0, 0x94, 0x55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1C, 0, 0,	// 30 0123456789:;<=>?
	0, 0x3C, 0x9C, 0x5D, 0, 0, 0, 0, 0, 0x44, 0x85, 0, 0xD5, 0x14, 0, 0,		// 40 @ABCDEFGHIJKLMNO
	0xF0, 0, 0, 0x50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 50 PQRSTUVWXYZ[\]^_
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				// 60 `abcdefghijklmno
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7F			// 70 pqrstuvwxyz{|}~
};

void generate_data(const char * in_message, char * out_data);

} /* namespace afsk */

#endif/*__AFSK_H__*/
