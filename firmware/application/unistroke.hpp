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

enum Condition {
	cond_empty = -1,
	stroke_a = 0,
	stroke_b = 1,
	stroke_c = 2,
	stroke_d = 3,
	last = 8
};

enum Direction {
	dir_empty = -1,
	Uw = 0x0F,		// Wildcards
	Dw = 0x1F,
	Lw = 0xF0,
	Rw = 0xF1,
	U = 0x02,		// Orthos
	D = 0x12,
	L = 0x20,
	R = 0x21,
	UL = 0x00,		// Diagonals
	DL = 0x10,
	UR = 0x01,
	DR = 0x11
};

struct HandWriting {
	uint8_t letter_count;
	struct HandWritingLetter {
		struct HandWritingMatch {
			Condition cond;
			Direction dir;
		} match[3];
		uint8_t count;
	} letter[32];
};

const HandWriting handwriting_unistroke = {
	27,
	{
	{{{stroke_a, UL},		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// BS<	0=UL MI=1
	{{{stroke_a, U},		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// A	0=U MI=1
	{{{stroke_a, DR},		{stroke_b, DL}, 			{cond_empty, dir_empty}}, 		2},	// B	0=DR 1=DL MI=2
	{{{stroke_a, UL},		{stroke_b, UR}, 			{cond_empty, dir_empty}}, 		0},	// C	0=UL 1=UR
	{{{stroke_a, DL},		{stroke_b, DR}, 			{cond_empty, dir_empty}}, 		0},	// D	0=DL 1=DR
	{{{stroke_a, L}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// E	0=L MI=1
	{{{stroke_a, U}, 		{stroke_b, R}, 				{cond_empty, dir_empty}}, 		0},	// F	0=U 1=R
	{{{stroke_a, R}, 		{stroke_b, U}, 				{cond_empty, dir_empty}}, 		0},	// G	0=R 1=U
	{{{stroke_a, R}, 		{stroke_b, D}, 				{cond_empty, dir_empty}}, 		2},	// H	0=R 1=D MI=2
	{{{stroke_a, D}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// I	0=D MI=1
	{{{stroke_a, D}, 		{stroke_b, L}, 				{cond_empty, dir_empty}}, 		2},	// J	0=D 1=L MI=2
	{{{stroke_a, UR}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// K	0=UR MI=1
	{{{stroke_a, D}, 		{stroke_b, R}, 				{cond_empty, dir_empty}}, 		2},	// L	0=D 1=R MI=2
	{{{stroke_a, UL}, 		{stroke_b, DL}, 			{cond_empty, dir_empty}},		2},	// M	0=UL 1=DL MI=2
	{{{stroke_a, UR}, 		{stroke_b, DR}, 			{cond_empty, dir_empty}},		2},	// N	0=UR 1=DR MI=2
	{{{stroke_a, DL}, 		{last, Lw}, 				{cond_empty, dir_empty}},		3},	// O	0=DL MI>2 -=Uw
	{{{stroke_a, DR}, 		{last, Dw}, 				{cond_empty, dir_empty}},		3},	// P	0=DR MI>2 -=Dw
	{{{stroke_a, DL}, 		{last, Dw},		 			{cond_empty, dir_empty}}, 		3},	// Q	0=DL MI>2 -=Dw
	{{{stroke_a, DR}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// R	0=DR MI=1
	{{{stroke_a, Lw}, 		{stroke_b, DR}, 			{cond_empty, dir_empty}},		0},	// S	0=Lw 1=DR
	{{{stroke_a, R}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// T	0=R MI=1
	{{{stroke_a, DL}, 		{stroke_b, UL}, 			{cond_empty, dir_empty}}, 		2},	// U	0=DL 1=UL
	{{{stroke_a, DR}, 		{stroke_b, UR}, 			{cond_empty, dir_empty}}, 		2},	// V	0=DR 1=UR
	{{{stroke_a, D}, 		{stroke_b, UR}, 			{stroke_c, D}},					0},	// W	0=D 1=UR 2=D
	{{{stroke_a, DR}, 		{last, Uw}, 				{cond_empty, dir_empty}},		3},	// X	0=DR MI>2 -=Uw
	{{{stroke_a, DL}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// Y	0=DL MI=1
	{{{stroke_a, Rw}, 		{stroke_b, DL}, 			{cond_empty, dir_empty}}, 		0},	// Z	0=Rw 1=DL
	}
};
