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

#include <string>

#ifndef __DE_BRUIJN_H__
#define __DE_BRUIJN_H__

// n from 3 to 16
const uint32_t de_bruijn_polys[14] {
	0b0000000000000101,
	0b0000000000001001,
	0b0000000000011011,
	0b0000000000110011,
	0b0000000001010011,
	0b0000000010001101,
	0b0000000100100001,
	0b0000001000100001,
	0b0000010001100001,
	0b0000110101000011,
	0b0001001001100101,
	0b0010101100000011,
	0b0101000000001001,
	0b1010000101000101
};

struct de_bruijn {
public:
	size_t init(const uint32_t n);
	uint32_t compute(const uint32_t steps);

private:
	uint32_t length { };
	uint32_t poly { };
	uint32_t shift_register { };
};

#endif/*__DE_BRUIJN_H__*/
