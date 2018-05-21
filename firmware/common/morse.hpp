/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __MORSE_H__
#define __MORSE_H__

#include "tonesets.hpp"
#include "portapack_shared_memory.hpp"

#define MORSE_DOT 1
#define MORSE_DASH 3
#define MORSE_SYMBOL_SPACE 1
#define MORSE_LETTER_SPACE 3
#define MORSE_WORD_SPACE 7

namespace morse {
	
const uint32_t morse_symbols[5] = {
	MORSE_DOT,
	MORSE_DASH,
	MORSE_SYMBOL_SPACE,
	MORSE_LETTER_SPACE,
	MORSE_WORD_SPACE
};

size_t morse_encode(std::string& message, const uint32_t time_unit_ms,
	const uint32_t tone, uint32_t * const time_units);

constexpr char foxhunt_codes[11][4] = {
	{ "MOE" },	// -----.
	{ "MOI" },	// -----..
	{ "MOS" },	// -----...
	{ "MOH" },	// -----....
	{ "MO5" },	// -----.....
	{ "MON" },	// ------.
	{ "MOD" },	// ------..
	{ "MOB" },	// ------...
	{ "MO6" },	// ------....
	{ "MO" },	// -----
	{ "S" }		// ...
};

// 0=dot 1=dash
constexpr uint16_t morse_ITU[63] = {
						//    Code    Size
	0b1010110000000110,	// !: 101011- 110
	0b0100100000000110,	// ": 010010- 110
	0,					// #
	0b0001001000000111,	// $: 0001001 111
	0,					// %
	0b0100000000000101,	// &: 01000-- 101
	0b0111100000000110,	// ': 011110- 110
	0b1011000000000101,	// (: 10110-- 101
	0b1011010000000110,	// ): 101101- 110
	0,					// *
	0b0101000000000101,	// +: 01010-- 101
	0b1100110000000110,	// ,: 110011- 110
	0b1000010000000110,	// -: 100001- 110
	0b0101010000000110,	// .: 010101- 110
	0b1001000000000101,	// /: 10010-- 101
	0b1111100000000101,	// 0: 11111-- 101
	0b0111100000000101,	// 1: 01111-- 101
	0b0011100000000101,	// 2: 00111-- 101
	0b0001100000000101,	// 3: 00011-- 101
	0b0000100000000101,	// 4: 00001-- 101
	0b0000000000000101,	// 5: 00000-- 101
	0b1000000000000101,	// 6: 10000-- 101
	0b1100000000000101,	// 7: 11000-- 101
	0b1110000000000101,	// 8: 11100-- 101
	0b1111000000000101,	// 9: 11110-- 101
	0b1110000000000110,	// :: 111000- 110
	0b1010100000000110,	// ;: 101010- 110
	0,					// <
	0b1000100000000101,	// =: 10001-- 101
	0,					// >
	0b0011000000000110,	// ?: 001100- 110
	0b0110100000000110,	// @: 011010- 110
	0b0100000000000010,	// A: 01----- 010
	0b1000000000000100,	// B: 1000--- 100
	0b1010000000000100,	// C: 1010--- 100
	0b1000000000000011,	// D: 100---- 011
	0b0000000000000001,	// E: 0------ 001
	0b0010000000000100,	// F: 0010--- 100
	0b1100000000000011,	// G: 110---- 011
	0b0000000000000100,	// H: 0000--- 100
	0b0000000000000010,	// I: 00----- 010
	0b0111000000000100,	// J: 0111--- 100
	0b1010000000000011,	// K: 101---- 011
	0b0100000000000100,	// L: 0100--- 100
	0b1100000000000010,	// M: 11----- 010
	0b1000000000000010,	// N: 10----- 010
	0b1110000000000011,	// O: 111---- 011
	0b0110000000000100,	// P: 0110--- 100 .#-###-###.# ##.#-### ##.#-###.# ##.#.# ##.#.#.# = 48 units
	0b1101000000000100,	// Q: 1101--- 100
	0b0100000000000011,	// R: 010---- 011
	0b0000000000000011,	// S: 000---- 011
	0b1000000000000001,	// T: 1------ 001
	0b0010000000000011,	// U: 001---- 011
	0b0001000000000100,	// V: 0001--- 100
	0b0110000000000011,	// W: 011---- 011
	0b1001000000000100,	// X: 1001--- 100
	0b1011000000000100,	// Y: 1011--- 100
	0b1100000000000100,	// Z: 1100--- 100
	0,					// [
	0,					// Back-slash
	0,					// ]
	0,					// ^
	0b0011010000000110	// _: 001101- 110
};

constexpr uint16_t prosigns[12] = {
						//    	  Code		Size
	0b0001110000001001,	// <SOS>: 000111000	1001
	0b0101000000000100,	// <AA>:  0101----- 0100
	0b0101000000000101,	// <AR>:  01010---- 0101
	0b0100000000000101,	// <AS>:  01000---- 0101
	0b1000100000000101,	// <BT>:  10001---- 0101
	0b1010100000000101,	// <CT>:  10101---- 0101
	0b0000000000001000,	// <HH>:  00000000- 1000
	0b1010000000000011,	// <K>:   101------ 0011
	0b1011000000000101,	// <KN>:  10110---- 0101
	0b1001110000000110,	// <NJ>:  100111--- 0110
	0b0001010000000110,	// <SK>:  000101--- 0110
	0b0001100000000101,	// <SN>:  00010---- 0101
};

} /* namespace morse */

#endif/*__MORSE_H__*/
