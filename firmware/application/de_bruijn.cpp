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
 * The length of the bitstream is (2 ^ 4) - 1 = 15
 * The primitive polynomials come from the de_bruijn_polys[] array (one for each code length)
 * The shift register is init with 1 and shifted left each step
 * The polynomial is kept on the right, and ANDed with the corresponding shift register bits
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
 */

void de_bruijn::init(const uint32_t n) {
	if ((n < 3) || (n > 16))
		length = 3;
	else
		length = n;
	
	poly = de_bruijn_polys[length - 3];
	shift_register = 1;
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
