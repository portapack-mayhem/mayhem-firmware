/*
 * Copyright (C) 2015 Craig Shelley (craig@microtron.org.uk)
 * Copyright (C) 2016 Furrtek
 *
 * BCH Encoder/Decoder - Adapted from GNURadio
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

#ifndef __BCHCODE_H__
#define __BCHCODE_H__

#include <vector>

class BCHCode {
public:
	BCHCode(std::vector<int> p_init, int m, int n, int k, int t);
	~BCHCode();
	
	BCHCode(const BCHCode&) = delete;
	BCHCode(BCHCode&&) = delete;
	BCHCode& operator=(const BCHCode&) = delete;
	BCHCode& operator=(BCHCode&&) = delete;

	int * encode(int data[]);
	int decode(int recd[]);
	
private:
	void gen_poly();
	void generate_gf();
	
	bool valid { false };

	int d { };
	int * p { };			// coefficients of primitive polynomial used to generate GF(2**5)
	int m { };				// order of the field GF(2**5) = 5
	int n { };				// 2**5 - 1 = 31
	int k { };				// n - deg(g(x)) = 21 = dimension
	int t { };				// 2 = error correcting capability
	int * alpha_to { };		// log table of GF(2**5)
	int * index_of { };		// antilog table of GF(2**5)
	int * g { };			// coefficients of generator polynomial, g(x) [n - k + 1]=[11]
	int * bb { };			// coefficients of redundancy polynomial ( x**(10) i(x) ) modulo g(x)
};

#endif/*__BCHCODE_H__*/
