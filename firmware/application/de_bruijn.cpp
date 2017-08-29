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

/* How this works:
 * B(2,4) means:
 * 	Alphabet size = 2 (binary)
 * 	Code length = 4
 * The length of the bitstream is (2 ^ 4) + (4 - 1) = 19 bits
 * The primitive polynomials come from the de_bruijn_polys[] array (one for each code length)
 * The shift register is init with 1 and shifted left each step
 * The polynomial is kept on the right, and used as a AND mask applied on the corresponding shift register bits
 * The resulting bits are XORed together to produce the new bit pushed in the shift register
 * 
 * 		0001 (init)
 * AND	1001 (polynomial)
 *      0001 XOR'd -> 1
 * 
 * 		00011 (shift left)
 * AND	 1001
 *       0001 XOR'd -> 1
 * 
 * 		000111 (shift left)
 * AND	  1001
 *        0001 XOR'd -> 1
 * 
 * 		0001111 (shift left)
 * AND	   1001
 *         1001 XOR'd -> 0
 * 
 * 		00011110 (shift left)
 * AND	    1001
 *          1000 XOR'd -> 1
 * ...
 * After 16 steps: (0+) 000111101011001000
 * Each of the 16 possible values appears in the sequence:
 * -0000-111101011001000 0
 * 0-0001-11101011001000 1
 * 0000111101011-0010-00 2
 * 00-0011-1101011001000 3
 * 00001111010110-0100-0 4
 * 00001111-0101-1001000 5
 * 0000111101-0110-01000 6
 * 000-0111-101011001000 7
 * 000011110101100-1000- 8
 * 000011110101-1001-000 9
 * 0000111-1010-11001000 10
 * 000011110-1011-001000 11
 * 00001111010-1100-1000 12
 * 000011-1101-011001000 13
 * 00001-1110-1011001000 14
 * 0000-1111-01011001000 15
 */

size_t de_bruijn::init(const uint32_t n) {
	// Cap
	if ((n < 3) || (n > 16))
		length = 3;
	else
		length = n;
	
	poly = de_bruijn_polys[length - 3];
	shift_register = 1;
	
	return (1U << length) + (length - 1);
}

uint32_t de_bruijn::compute(const uint32_t steps) {
	uint32_t step, bits, masked;
	uint8_t new_bit;
	
	for (step = 0; step < steps; step++) {
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
