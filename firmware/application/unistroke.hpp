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

#define HWDIR_TWO 0x40
#define HWDIR_ONLY 0x80

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
	Uw = 0x00,
	Dw = 0x10,
	Lw = 0x00,
	Rw = 0x01,
	U = 0x00 | HWDIR_ONLY,
	D = 0x10 | HWDIR_ONLY,
	L = 0x00 | HWDIR_ONLY,
	R = 0x01 | HWDIR_ONLY,
	UL = 0x00 | HWDIR_TWO,
	DL = 0x10 | HWDIR_TWO,
	UR = 0x01 | HWDIR_TWO,
	DR = 0x11 | HWDIR_TWO
};

struct HandWriting {
	struct HandWritingLetter {
		Condition cond;
		Direction dir;
	} letter[3];
	int8_t count;
};

const HandWriting handwriting_unistroke[32] = {
	{{{stroke_a, U},		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// A	0=U MI=1
	{{{stroke_a, DR},		{stroke_b, DL}, 			{cond_empty, dir_empty}}, 		0},	// B	0=DR 1=DL
	{{{stroke_a, UL},		{stroke_b, UR}, 			{cond_empty, dir_empty}}, 		0},	// C	0=UL 1=UR
	{{{stroke_a, DL},		{stroke_b, DR}, 			{cond_empty, dir_empty}}, 		0},	// D	0=DL 1=DR
	{{{stroke_a, L}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// E	0=L MI=1
	{{{stroke_a, U}, 		{stroke_b, R}, 				{cond_empty, dir_empty}}, 		0},	// F	0=U 1=R
	{{{stroke_a, R}, 		{stroke_b, U}, 				{cond_empty, dir_empty}}, 		0},	// G	0=R 1=U
	{{{stroke_a, R}, 		{stroke_b, D}, 				{cond_empty, dir_empty}}, 		0},	// H	0=R 1=D
	{{{stroke_a, D}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// I	0=D MI=1
	{{{stroke_a, D}, 		{stroke_b, L}, 				{cond_empty, dir_empty}}, 		0},	// J	0=D 1=L
	{{{stroke_a, UR}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// K	0=UR MI=1
	{{{stroke_a, D}, 		{stroke_b, R}, 				{cond_empty, dir_empty}}, 		0},	// L	0=D 1=R
	{{{stroke_a, UL}, 		{stroke_b, DL}, 			{cond_empty, dir_empty}},		0},	// M	0=UL 1=DL
	{{{stroke_a, UR}, 		{stroke_b, DR}, 			{cond_empty, dir_empty}},		0},	// N	0=UR 1=DR
	{{{stroke_a, Lw}, 		{last, Lw}, 				{cond_empty, dir_empty}},		2},	// O	0=Lw MI>2 -=Lw		!!!
	{{{stroke_a, Dw}, 		{last, Dw}, 				{cond_empty, dir_empty}},		2},	// P	0=Dw MI>2 -=Dw		!!!
	{{{stroke_a, Dw}, 		{stroke_b, Lw}, 			{cond_empty, dir_empty}}, 		2},	// Q	0=Dw MI>2 1=Lw
	{{{stroke_a, DR}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// R	0=DR MI=1
	{{{stroke_a, Lw}, 		{stroke_b, Rw}, 			{cond_empty, dir_empty}},		0},	// S	0=Lw 1=Rw
	{{{stroke_a, R}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// T	0=R MI=1
	{{{stroke_a, DL}, 		{stroke_b, UL}, 			{cond_empty, dir_empty}}, 		2},	// U	0=DL 1=UL MI=2
	{{{stroke_a, DR}, 		{stroke_b, UR}, 			{cond_empty, dir_empty}}, 		2},	// V	0=DR 1=UR MI=2
	{{{stroke_a, D}, 		{stroke_b, UR}, 			{stroke_c, D}},					0},	// W	0=D 1=UR 2=D
	{{{stroke_a, Rw}, 		{last, Rw}, 				{cond_empty, dir_empty}},		0},	// X	0=Rw MI>2 -=Rw
	{{{stroke_a, DL}, 		{cond_empty, dir_empty}, 	{cond_empty, dir_empty}}, 		1},	// Y	0=DL MI=1
	{{{stroke_a, Rw}, 		{stroke_b, DL}, 			{cond_empty, dir_empty}}, 		0},	// Z	0=Rw 1=DL
	
	// Erase 0=UR MI!1 1=L
};
