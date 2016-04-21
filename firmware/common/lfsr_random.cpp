/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "lfsr_random.hpp"

static void lfsr_iterate_internal(lfsr_word_t& v)
{
	/*
	    Generated with lfsr-generator:
	    http://lfsr-generator.sourceforge.net
	    =============================================
		config          : fibonacci
		length          : 31
		taps            : (31, 18)
		shift-amounts   : (12, 12, 8)
		shift-direction : left
	*/
	enum {
		length         = 31,
		tap_0          = 31,
		tap_1          = 18,
		shift_amount_0 = 12,
		shift_amount_1 = 12,
		shift_amount_2 =  8
	};

	const lfsr_word_t zero = 0;
	v = (
		(
			v << shift_amount_0
		) | (
			(
				(v >> (tap_0 - shift_amount_0)) ^
				(v >> (tap_1 - shift_amount_0))
			) & (
				~(~zero << shift_amount_0)
			)
		)
	);
	v = (
		(
			v << shift_amount_1
		) | (
			(
				(v >> (tap_0 - shift_amount_1)) ^
				(v >> (tap_1 - shift_amount_1))
			) & (
				~(~zero << shift_amount_1)
			)
		)
	);
	v = (
		(
			v << shift_amount_2
		) | (
			(
				(v >> (tap_0 - shift_amount_2)) ^
				(v >> (tap_1 - shift_amount_2))
			) & (
				~(~zero << shift_amount_2)
			)
		)
	);
}

lfsr_word_t lfsr_iterate(lfsr_word_t v) {
	lfsr_iterate_internal(v);
	return v;
}

void lfsr_fill(lfsr_word_t& v, lfsr_word_t* buffer, size_t word_count) {
	while( word_count != 0 ) {
		lfsr_iterate_internal(v);
		*(buffer++) = v;
		word_count--;
	}
}

bool lfsr_compare(lfsr_word_t& v, const lfsr_word_t* buffer, size_t word_count) {
	while( word_count != 0 ) {
		lfsr_iterate_internal(v);
		if( *(buffer++) != v ) {
			return false;
		}
		word_count--;
	}
	return true;
}
