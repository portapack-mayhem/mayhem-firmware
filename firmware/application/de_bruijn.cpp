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

#include "de_bruijn.hpp"

/*
How this works:
B(2,4):		2 states, 4 bits
Primitive poly with n=4: 1001

	0001
xor 1001 = 0 ^ 1 = 1

	00011
xor  1001 = 0 ^ 1 = 1

	000111
xor   1001 = 0 ^ 1 = 1

	0001111
xor    1001 = 1 ^ 1 = 0

	00011110
xor     1001 = 1 ^ 0 = 1

	000111101
xor      1001 = 1 ^ 1 = 0

	0001111010
xor       1001 = 0 ^ 1 = 1

	00011110101
xor        1001 = 0 ^ 1 = 1

	000111101011
xor         1001 = 1 ^ 1 = 0

	0001111010110
xor          1001 = 0 ^ 0 = 0

	00011110101100
xor           1001 = 1 ^ 0 = 1

	000111101011001
xor            1001 = 1 ^ 1 = 0

	0001111010110010
xor             1001 = 0 ^ 0 = 0
*/

void de_bruijn::init(const uint32_t n) {
	if ((n < 3) || (n > 16))
		length = 3;
	else
		length = n;
	
	// k is 2 (binary sequence)
	poly = de_bruijn_polys[length - 3];
	shift_register = 1;
}

uint32_t de_bruijn::compute(const uint32_t step) {
	uint32_t c, bits, masked;
	uint8_t new_bit;
	
	for (c = 0; c < step; c++) {
		masked = shift_register & poly;
		new_bit = 0;
		for (bits = 0; bits < length; bits++) {
			new_bit ^= (masked & 1);
			masked >>= 1;
		}
		shift_register <<= 1;
		shift_register |= new_bit;
	}
	
	return shift_register;
}
