/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "spectrum_color_lut.hpp"

const std::array<ui::Color, 256> spectrum_rgb2_lut { {
	{   0,   0, 128 },
	{   0,   0, 132 },
	{   0,   0, 136 },
	{   0,   0, 140 },
	{   0,   0, 144 },
	{   0,   0, 148 },
	{   0,   0, 152 },
	{   0,   0, 156 },
	{   0,   0, 160 },
	{   0,   0, 164 },
	{   0,   0, 168 },
	{   0,   0, 172 },
	{   0,   0, 176 },
	{   0,   0, 180 },
	{   0,   0, 184 },
	{   0,   0, 188 },
	{   0,   0, 192 },
	{   0,   0, 195 },
	{   0,   0, 199 },
	{   0,   0, 203 },
	{   0,   0, 207 },
	{   0,   0, 211 },
	{   0,   0, 215 },
	{   0,   0, 219 },
	{   0,   0, 223 },
	{   0,   0, 227 },
	{   0,   0, 231 },
	{   0,   0, 235 },
	{   0,   0, 239 },
	{   0,   0, 243 },
	{   0,   0, 247 },
	{   0,   0, 251 },
	{   0,   0, 255 },
	{   0,   4, 255 },
	{   0,   8, 255 },
	{   0,  12, 255 },
	{   0,  16, 255 },
	{   0,  20, 255 },
	{   0,  24, 255 },
	{   0,  28, 255 },
	{   0,  32, 255 },
	{   0,  36, 255 },
	{   0,  40, 255 },
	{   0,  44, 255 },
	{   0,  48, 255 },
	{   0,  52, 255 },
	{   0,  56, 255 },
	{   0,  60, 255 },
	{   0,  64, 255 },
	{   0,  68, 255 },
	{   0,  72, 255 },
	{   0,  76, 255 },
	{   0,  80, 255 },
	{   0,  84, 255 },
	{   0,  88, 255 },
	{   0,  92, 255 },
	{   0,  96, 255 },
	{   0, 100, 255 },
	{   0, 104, 255 },
	{   0, 108, 255 },
	{   0, 112, 255 },
	{   0, 116, 255 },
	{   0, 120, 255 },
	{   0, 124, 255 },
	{   0, 128, 255 },
	{   0, 131, 255 },
	{   0, 135, 255 },
	{   0, 139, 255 },
	{   0, 143, 255 },
	{   0, 147, 255 },
	{   0, 151, 255 },
	{   0, 155, 255 },
	{   0, 159, 255 },
	{   0, 163, 255 },
	{   0, 167, 255 },
	{   0, 171, 255 },
	{   0, 175, 255 },
	{   0, 179, 255 },
	{   0, 183, 255 },
	{   0, 187, 255 },
	{   0, 191, 255 },
	{   0, 195, 255 },
	{   0, 199, 255 },
	{   0, 203, 255 },
	{   0, 207, 255 },
	{   0, 211, 255 },
	{   0, 215, 255 },
	{   0, 219, 255 },
	{   0, 223, 255 },
	{   0, 227, 255 },
	{   0, 231, 255 },
	{   0, 235, 255 },
	{   0, 239, 255 },
	{   0, 243, 255 },
	{   0, 247, 255 },
	{   0, 251, 255 },
	{   0, 255, 255 },
	{   4, 255, 251 },
	{   8, 255, 247 },
	{  12, 255, 243 },
	{  16, 255, 239 },
	{  20, 255, 235 },
	{  24, 255, 231 },
	{  28, 255, 227 },
	{  32, 255, 223 },
	{  36, 255, 219 },
	{  40, 255, 215 },
	{  44, 255, 211 },
	{  48, 255, 207 },
	{  52, 255, 203 },
	{  56, 255, 199 },
	{  60, 255, 195 },
	{  64, 255, 191 },
	{  68, 255, 187 },
	{  72, 255, 183 },
	{  76, 255, 179 },
	{  80, 255, 175 },
	{  84, 255, 171 },
	{  88, 255, 167 },
	{  92, 255, 163 },
	{  96, 255, 159 },
	{ 100, 255, 155 },
	{ 104, 255, 151 },
	{ 108, 255, 147 },
	{ 112, 255, 143 },
	{ 116, 255, 139 },
	{ 120, 255, 135 },
	{ 124, 255, 131 },
	{ 128, 255, 128 },
	{ 131, 255, 124 },
	{ 135, 255, 120 },
	{ 139, 255, 116 },
	{ 143, 255, 112 },
	{ 147, 255, 108 },
	{ 151, 255, 104 },
	{ 155, 255, 100 },
	{ 159, 255,  96 },
	{ 163, 255,  92 },
	{ 167, 255,  88 },
	{ 171, 255,  84 },
	{ 175, 255,  80 },
	{ 179, 255,  76 },
	{ 183, 255,  72 },
	{ 187, 255,  68 },
	{ 191, 255,  64 },
	{ 195, 255,  60 },
	{ 199, 255,  56 },
	{ 203, 255,  52 },
	{ 207, 255,  48 },
	{ 211, 255,  44 },
	{ 215, 255,  40 },
	{ 219, 255,  36 },
	{ 223, 255,  32 },
	{ 227, 255,  28 },
	{ 231, 255,  24 },
	{ 235, 255,  20 },
	{ 239, 255,  16 },
	{ 243, 255,  12 },
	{ 247, 255,   8 },
	{ 251, 255,   4 },
	{ 255, 255,   0 },
	{ 255, 251,   0 },
	{ 255, 247,   0 },
	{ 255, 243,   0 },
	{ 255, 239,   0 },
	{ 255, 235,   0 },
	{ 255, 231,   0 },
	{ 255, 227,   0 },
	{ 255, 223,   0 },
	{ 255, 219,   0 },
	{ 255, 215,   0 },
	{ 255, 211,   0 },
	{ 255, 207,   0 },
	{ 255, 203,   0 },
	{ 255, 199,   0 },
	{ 255, 195,   0 },
	{ 255, 191,   0 },
	{ 255, 187,   0 },
	{ 255, 183,   0 },
	{ 255, 179,   0 },
	{ 255, 175,   0 },
	{ 255, 171,   0 },
	{ 255, 167,   0 },
	{ 255, 163,   0 },
	{ 255, 159,   0 },
	{ 255, 155,   0 },
	{ 255, 151,   0 },
	{ 255, 147,   0 },
	{ 255, 143,   0 },
	{ 255, 139,   0 },
	{ 255, 135,   0 },
	{ 255, 131,   0 },
	{ 255, 128,   0 },
	{ 255, 124,   0 },
	{ 255, 120,   0 },
	{ 255, 116,   0 },
	{ 255, 112,   0 },
	{ 255, 108,   0 },
	{ 255, 104,   0 },
	{ 255, 100,   0 },
	{ 255,  96,   0 },
	{ 255,  92,   0 },
	{ 255,  88,   0 },
	{ 255,  84,   0 },
	{ 255,  80,   0 },
	{ 255,  76,   0 },
	{ 255,  72,   0 },
	{ 255,  68,   0 },
	{ 255,  64,   0 },
	{ 255,  60,   0 },
	{ 255,  56,   0 },
	{ 255,  52,   0 },
	{ 255,  48,   0 },
	{ 255,  44,   0 },
	{ 255,  40,   0 },
	{ 255,  36,   0 },
	{ 255,  32,   0 },
	{ 255,  28,   0 },
	{ 255,  24,   0 },
	{ 255,  20,   0 },
	{ 255,  16,   0 },
	{ 255,  12,   0 },
	{ 255,   8,   0 },
	{ 255,   4,   0 },
	{ 255,   0,   0 },
	{ 251,   0,   0 },
	{ 247,   0,   0 },
	{ 243,   0,   0 },
	{ 239,   0,   0 },
	{ 235,   0,   0 },
	{ 231,   0,   0 },
	{ 227,   0,   0 },
	{ 223,   0,   0 },
	{ 219,   0,   0 },
	{ 215,   0,   0 },
	{ 211,   0,   0 },
	{ 207,   0,   0 },
	{ 203,   0,   0 },
	{ 199,   0,   0 },
	{ 195,   0,   0 },
	{ 192,   0,   0 },
	{ 188,   0,   0 },
	{ 184,   0,   0 },
	{ 180,   0,   0 },
	{ 176,   0,   0 },
	{ 172,   0,   0 },
	{ 168,   0,   0 },
	{ 164,   0,   0 },
	{ 160,   0,   0 },
	{ 156,   0,   0 },
	{ 152,   0,   0 },
	{ 148,   0,   0 },
	{ 144,   0,   0 },
	{ 140,   0,   0 },
	{ 136,   0,   0 },
	{ 132,   0,   0 },
} };

const std::array<ui::Color, 256> spectrum_rgb3_lut { {
	{ 0x00, 0x00, 0x02 },
	{ 0x00, 0x00, 0x04 },
	{ 0x00, 0x00, 0x05 },
	{ 0x00, 0x00, 0x07 },
	{ 0x00, 0x00, 0x0a },
	{ 0x00, 0x00, 0x0c },
	{ 0x00, 0x00, 0x0e },
	{ 0x00, 0x00, 0x10 },
	{ 0x00, 0x00, 0x13 },
	{ 0x00, 0x00, 0x15 },
	{ 0x00, 0x00, 0x17 },
	{ 0x00, 0x00, 0x19 },
	{ 0x00, 0x00, 0x1b },
	{ 0x00, 0x00, 0x1d },
	{ 0x00, 0x00, 0x1f },
	{ 0x00, 0x00, 0x21 },
	{ 0x00, 0x00, 0x24 },
	{ 0x00, 0x00, 0x25 },
	{ 0x00, 0x00, 0x28 },
	{ 0x00, 0x00, 0x29 },
	{ 0x00, 0x00, 0x2c },
	{ 0x00, 0x00, 0x2e },
	{ 0x00, 0x00, 0x30 },
	{ 0x00, 0x00, 0x33 },
	{ 0x00, 0x00, 0x35 },
	{ 0x00, 0x00, 0x37 },
	{ 0x00, 0x00, 0x39 },
	{ 0x00, 0x00, 0x3b },
	{ 0x00, 0x00, 0x3d },
	{ 0x00, 0x00, 0x3f },
	{ 0x00, 0x00, 0x41 },
	{ 0x00, 0x00, 0x43 },
	{ 0x00, 0x00, 0x46 },
	{ 0x00, 0x00, 0x48 },
	{ 0x00, 0x00, 0x4a },
	{ 0x00, 0x00, 0x4c },
	{ 0x00, 0x00, 0x4e },
	{ 0x00, 0x00, 0x50 },
	{ 0x00, 0x00, 0x53 },
	{ 0x00, 0x00, 0x55 },
	{ 0x00, 0x00, 0x56 },
	{ 0x00, 0x00, 0x59 },
	{ 0x00, 0x00, 0x5b },
	{ 0x00, 0x00, 0x5d },
	{ 0x00, 0x00, 0x5f },
	{ 0x00, 0x00, 0x62 },
	{ 0x00, 0x00, 0x63 },
	{ 0x00, 0x00, 0x66 },
	{ 0x00, 0x00, 0x68 },
	{ 0x00, 0x00, 0x6a },
	{ 0x00, 0x00, 0x6c },
	{ 0x00, 0x00, 0x6e },
	{ 0x00, 0x00, 0x70 },
	{ 0x00, 0x00, 0x73 },
	{ 0x00, 0x00, 0x75 },
	{ 0x00, 0x00, 0x77 },
	{ 0x00, 0x00, 0x79 },
	{ 0x00, 0x00, 0x7b },
	{ 0x00, 0x00, 0x7d },
	{ 0x00, 0x00, 0x7f },
	{ 0x00, 0x00, 0x82 },
	{ 0x00, 0x00, 0x84 },
	{ 0x00, 0x00, 0x86 },
	{ 0x00, 0x00, 0x88 },
	{ 0x00, 0x00, 0x8a },
	{ 0x00, 0x00, 0x8c },
	{ 0x00, 0x00, 0x8e },
	{ 0x00, 0x00, 0x91 },
	{ 0x00, 0x00, 0x93 },
	{ 0x00, 0x00, 0x95 },
	{ 0x00, 0x00, 0x97 },
	{ 0x00, 0x00, 0x99 },
	{ 0x00, 0x00, 0x9b },
	{ 0x00, 0x00, 0x9d },
	{ 0x00, 0x00, 0x9f },
	{ 0x00, 0x00, 0xa2 },
	{ 0x00, 0x00, 0xa4 },
	{ 0x00, 0x00, 0xa6 },
	{ 0x00, 0x00, 0xa8 },
	{ 0x00, 0x00, 0xaa },
	{ 0x00, 0x00, 0xac },
	{ 0x00, 0x00, 0xae },
	{ 0x00, 0x00, 0xb1 },
	{ 0x00, 0x00, 0xb3 },
	{ 0x00, 0x00, 0xb5 },
	{ 0x00, 0x00, 0xb7 },
	{ 0x00, 0x00, 0xb9 },
	{ 0x00, 0x00, 0xbb },
	{ 0x00, 0x00, 0xbd },
	{ 0x00, 0x00, 0xbf },
	{ 0x00, 0x00, 0xc2 },
	{ 0x00, 0x00, 0xc4 },
	{ 0x00, 0x00, 0xc6 },
	{ 0x00, 0x00, 0xc8 },
	{ 0x00, 0x00, 0xca },
	{ 0x00, 0x00, 0xcc },
	{ 0x00, 0x00, 0xce },
	{ 0x00, 0x00, 0xd1 },
	{ 0x00, 0x00, 0xd3 },
	{ 0x00, 0x00, 0xd5 },
	{ 0x00, 0x00, 0xd7 },
	{ 0x00, 0x00, 0xd9 },
	{ 0x00, 0x00, 0xdb },
	{ 0x00, 0x00, 0xdd },
	{ 0x00, 0x00, 0xdf },
	{ 0x00, 0x00, 0xe2 },
	{ 0x00, 0x00, 0xe4 },
	{ 0x00, 0x00, 0xe6 },
	{ 0x00, 0x00, 0xe8 },
	{ 0x00, 0x00, 0xea },
	{ 0x00, 0x00, 0xec },
	{ 0x00, 0x00, 0xee },
	{ 0x00, 0x00, 0xf1 },
	{ 0x00, 0x00, 0xf3 },
	{ 0x00, 0x00, 0xf5 },
	{ 0x00, 0x00, 0xf7 },
	{ 0x00, 0x00, 0xf9 },
	{ 0x00, 0x00, 0xfb },
	{ 0x00, 0x00, 0xfd },
	{ 0x01, 0x00, 0xfe },
	{ 0x05, 0x00, 0xfa },
	{ 0x09, 0x00, 0xf6 },
	{ 0x0e, 0x00, 0xf2 },
	{ 0x12, 0x00, 0xed },
	{ 0x16, 0x00, 0xe9 },
	{ 0x1b, 0x00, 0xe4 },
	{ 0x1f, 0x00, 0xe0 },
	{ 0x24, 0x00, 0xdc },
	{ 0x27, 0x00, 0xd8 },
	{ 0x2c, 0x00, 0xd3 },
	{ 0x30, 0x00, 0xcf },
	{ 0x35, 0x00, 0xcb },
	{ 0x39, 0x00, 0xc6 },
	{ 0x3d, 0x00, 0xc2 },
	{ 0x42, 0x00, 0xbd },
	{ 0x46, 0x00, 0xb9 },
	{ 0x4b, 0x00, 0xb5 },
	{ 0x4f, 0x00, 0xb0 },
	{ 0x53, 0x00, 0xac },
	{ 0x58, 0x00, 0xa7 },
	{ 0x5c, 0x00, 0xa3 },
	{ 0x61, 0x00, 0x9f },
	{ 0x65, 0x00, 0x9a },
	{ 0x69, 0x00, 0x96 },
	{ 0x6e, 0x00, 0x91 },
	{ 0x72, 0x00, 0x8d },
	{ 0x77, 0x00, 0x89 },
	{ 0x7b, 0x00, 0x84 },
	{ 0x7f, 0x00, 0x80 },
	{ 0x84, 0x00, 0x7b },
	{ 0x88, 0x00, 0x77 },
	{ 0x8d, 0x00, 0x73 },
	{ 0x91, 0x00, 0x6e },
	{ 0x95, 0x00, 0x6a },
	{ 0x9a, 0x00, 0x65 },
	{ 0x9e, 0x00, 0x61 },
	{ 0xa3, 0x00, 0x5d },
	{ 0xa7, 0x00, 0x58 },
	{ 0xab, 0x00, 0x54 },
	{ 0xb0, 0x00, 0x4f },
	{ 0xb4, 0x00, 0x4b },
	{ 0xb9, 0x00, 0x47 },
	{ 0xbd, 0x00, 0x42 },
	{ 0xc1, 0x00, 0x3e },
	{ 0xc6, 0x00, 0x39 },
	{ 0xca, 0x00, 0x35 },
	{ 0xcf, 0x00, 0x31 },
	{ 0xd3, 0x00, 0x2c },
	{ 0xd7, 0x00, 0x28 },
	{ 0xdc, 0x00, 0x23 },
	{ 0xe0, 0x00, 0x1f },
	{ 0xe5, 0x00, 0x1b },
	{ 0xe9, 0x00, 0x16 },
	{ 0xed, 0x00, 0x12 },
	{ 0xf2, 0x00, 0x0d },
	{ 0xf6, 0x00, 0x09 },
	{ 0xfb, 0x00, 0x05 },
	{ 0xfe, 0x01, 0x01 },
	{ 0xff, 0x06, 0x00 },
	{ 0xff, 0x0d, 0x00 },
	{ 0xff, 0x13, 0x00 },
	{ 0xff, 0x1a, 0x00 },
	{ 0xff, 0x20, 0x00 },
	{ 0xff, 0x26, 0x00 },
	{ 0xff, 0x2d, 0x00 },
	{ 0xff, 0x33, 0x00 },
	{ 0xff, 0x3a, 0x00 },
	{ 0xff, 0x40, 0x00 },
	{ 0xff, 0x46, 0x00 },
	{ 0xff, 0x4d, 0x00 },
	{ 0xff, 0x53, 0x00 },
	{ 0xff, 0x59, 0x00 },
	{ 0xff, 0x60, 0x00 },
	{ 0xff, 0x66, 0x00 },
	{ 0xff, 0x6c, 0x00 },
	{ 0xff, 0x73, 0x00 },
	{ 0xff, 0x79, 0x00 },
	{ 0xff, 0x80, 0x00 },
	{ 0xff, 0x86, 0x00 },
	{ 0xff, 0x8c, 0x00 },
	{ 0xff, 0x93, 0x00 },
	{ 0xff, 0x99, 0x00 },
	{ 0xff, 0x9f, 0x00 },
	{ 0xff, 0xa6, 0x00 },
	{ 0xff, 0xac, 0x00 },
	{ 0xff, 0xb3, 0x00 },
	{ 0xff, 0xb9, 0x00 },
	{ 0xff, 0xbf, 0x00 },
	{ 0xff, 0xc6, 0x00 },
	{ 0xff, 0xcc, 0x00 },
	{ 0xff, 0xd2, 0x00 },
	{ 0xff, 0xd9, 0x00 },
	{ 0xff, 0xdf, 0x00 },
	{ 0xff, 0xe6, 0x00 },
	{ 0xff, 0xec, 0x00 },
	{ 0xff, 0xf2, 0x00 },
	{ 0xff, 0xf9, 0x00 },
	{ 0xff, 0xfe, 0x01 },
	{ 0xff, 0xff, 0x06 },
	{ 0xff, 0xff, 0x0d },
	{ 0xff, 0xff, 0x14 },
	{ 0xff, 0xff, 0x1b },
	{ 0xff, 0xff, 0x21 },
	{ 0xff, 0xff, 0x28 },
	{ 0xff, 0xff, 0x2f },
	{ 0xff, 0xff, 0x35 },
	{ 0xff, 0xff, 0x3c },
	{ 0xff, 0xff, 0x42 },
	{ 0xff, 0xff, 0x49 },
	{ 0xff, 0xff, 0x4f },
	{ 0xff, 0xff, 0x56 },
	{ 0xff, 0xff, 0x5d },
	{ 0xff, 0xff, 0x63 },
	{ 0xff, 0xff, 0x6a },
	{ 0xff, 0xff, 0x71 },
	{ 0xff, 0xff, 0x77 },
	{ 0xff, 0xff, 0x7e },
	{ 0xff, 0xff, 0x84 },
	{ 0xff, 0xff, 0x8b },
	{ 0xff, 0xff, 0x92 },
	{ 0xff, 0xff, 0x98 },
	{ 0xff, 0xff, 0x9f },
	{ 0xff, 0xff, 0xa6 },
	{ 0xff, 0xff, 0xac },
	{ 0xff, 0xff, 0xb3 },
	{ 0xff, 0xff, 0xb9 },
	{ 0xff, 0xff, 0xc0 },
	{ 0xff, 0xff, 0xc7 },
	{ 0xff, 0xff, 0xcd },
	{ 0xff, 0xff, 0xd4 },
	{ 0xff, 0xff, 0xdb },
	{ 0xff, 0xff, 0xe1 },
	{ 0xff, 0xff, 0xe8 },
	{ 0xff, 0xff, 0xee },
	{ 0xff, 0xff, 0xf5 },
	{ 0xff, 0xff, 0xfb }
} };

const std::array<ui::Color, 256> spectrum_rgb4_lut { {
	{   0,   0,   0 },
	{   1,   1,   1 },
	{   2,   2,   2 },
	{   3,   3,   3 },
	{   4,   4,   4 },
	{   5,   5,   5 },
	{   6,   6,   6 },
	{   7,   7,   7 },
	{   8,   8,   8 },
	{   9,   9,   9 },
	{   10,  10,  10 },
	{   11,  11,  11 },
	{   12,  12,  12 },
	{   13,  13,  13 },
	{   14,  14,  14 },
	{   15,  15,  15 },
	{   16,  16,  16 },
	{   17,  17,  17 },
	{   18,  18,  18 },
	{   19,  19,  19 },
	{   20,  20,  20 },
	{   21,  21,  21 },
	{   22,  22,  22 },
	{   23,  23,  23 },
	{   24,  24,  24 },
	{   25,  25,  25 },
	{   26,  26,  26 },
	{   27,  27,  27 },
	{   28,  28,  28 },
	{   29,  29,  29 },
	{   30,  30,  30 },
	{   31,  31,  31 },
	{   32,  32,  32 },
	{   33,  33,  33 },
	{   34,  34,  34 },
	{   35,  35,  35 },
	{   36,  36,  36 },
	{   37,  37,  37 },
	{   38,  38,  38 },
	{   39,  39,  39 },
	{   40,  40,  40 },
	{   41,  41,  41 },
	{   42,  42,  42 },
	{   43,  43,  43 },
	{   44,  44,  44 },
	{   45,  45,  45 },
	{   46,  46,  46 },
	{   47,  47,  47 },
	{   48,  48,  48 },
	{   49,  49,  49 },
	{   50,  50,  50 },
	{   51,  51,  51 },
	{   52,  52,  52 },
	{   53,  53,  53 },
	{   54,  54,  54 },
	{   55,  55,  55 },
	{   56,  56,  56 },
	{   57,  57,  57 },
	{   58,  58,  58 },
	{   59,  59,  59 },
	{   60,  60,  60 },
	{   61,  61,  61 },
	{   62,  62,  62 },
	{   63,  63,  63 },
	{   64,  64,  64 },
	{   65,  65,  65 },
	{   66,  66,  66 },
	{   67,  67,  67 },
	{   68,  68,  68 },
	{   69,  69,  69 },
	{   70,  70,  70 },
	{   71,  71,  71 },
	{   72,  72,  72 },
	{   73,  73,  73 },
	{   74,  74,  74 },
	{   75,  75,  75 },
	{   76,  76,  76 },
	{   77,  77,  77 },
	{   78,  78,  78 },
	{   79,  79,  79 },
	{   80,  80,  80 },
	{   81,  81,  81 },
	{   82,  82,  82 },
	{   83,  83,  83 },
	{   84,  84,  84 },
	{   85,  85,  85 },
	{   86,  86,  86 },
	{   87,  87,  87 },
	{   88,  88,  88 },
	{   89,  89,  89 },
	{   90,  90,  90 },
	{   91,  91,  91 },
	{   92,  92,  92 },
	{   93,  93,  93 },
	{   94,  94,  94 },
	{   95,  95,  95 },
	{   96,  96,  96 },
	{   97,  97,  97 },
	{   98,  98,  98 },
	{   99,  99,  99 },
	{   100, 100, 100 },
	{   101, 101, 101 },
	{   102, 102, 102 },
	{   103, 103, 103 },
	{   104, 104, 104 },
	{   105, 105, 105 },
	{   106, 106, 106 },
	{   107, 107, 107 },
	{   108, 108, 108 },
	{   109, 109, 109 },
	{   110, 110, 110 },
	{   111, 111, 111 },
	{   112, 112, 112 },
	{   113, 113, 113 },
	{   114, 114, 114 },
	{   115, 115, 115 },
	{   116, 116, 116 },
	{   117, 117, 117 },
	{   118, 118, 118 },
	{   119, 119, 119 },
	{   120, 120, 120 },
	{   121, 121, 121 },
	{   122, 122, 122 },
	{   123, 123, 123 },
	{   124, 124, 124 },
	{   125, 125, 125 },
	{   126, 126, 126 },
	{   127, 127, 127 },
	{   128, 128, 128 },
	{   129, 129, 129 },
	{   130, 130, 130 },
	{   131, 131, 131 },
	{   132, 132, 132 },
	{   133, 133, 133 },
	{   134, 134, 134 },
	{   135, 135, 135 },
	{   136, 136, 136 },
	{   137, 137, 137 },
	{   138, 138, 138 },
	{   139, 139, 139 },
	{   140, 140, 140 },
	{   141, 141, 141 },
	{   142, 142, 142 },
	{   143, 143, 143 },
	{   144, 144, 144 },
	{   145, 145, 145 },
	{   146, 146, 146 },
	{   147, 147, 147 },
	{   148, 148, 148 },
	{   149, 149, 149 },
	{   150, 150, 150 },
	{   151, 151, 151 },
	{   152, 152, 152 },
	{   153, 153, 153 },
	{   154, 154, 154 },
	{   155, 155, 155 },
	{   156, 156, 156 },
	{   157, 157, 157 },
	{   158, 158, 158 },
	{   159, 159, 159 },
	{   160, 160, 160 },
	{   161, 161, 161 },
	{   162, 162, 162 },
	{   163, 163, 163 },
	{   164, 164, 164 },
	{   165, 165, 165 },
	{   166, 166, 166 },
	{   167, 167, 167 },
	{   168, 168, 168 },
	{   169, 169, 169 },
	{   170, 170, 170 },
	{   171, 171, 171 },
	{   172, 172, 172 },
	{   173, 173, 173 },
	{   174, 174, 174 },
	{   175, 175, 175 },
	{   176, 176, 176 },
	{   177, 177, 177 },
	{   178, 178, 178 },
	{   179, 179, 179 },
	{   180, 180, 180 },
	{   181, 181, 181 },
	{   182, 182, 182 },
	{   183, 183, 183 },
	{   184, 184, 184 },
	{   185, 185, 185 },
	{   186, 186, 186 },
	{   187, 187, 187 },
	{   188, 188, 188 },
	{   189, 189, 189 },
	{   190, 190, 190 },
	{   191, 191, 191 },
	{   192, 192, 192 },
	{   193, 193, 193 },
	{   194, 194, 194 },
	{   195, 195, 195 },
	{   196, 196, 196 },
	{   197, 197, 197 },
	{   198, 198, 198 },
	{   199, 199, 199 },
	{   200, 200, 200 },
	{   201, 201, 201 },
	{   202, 202, 202 },
	{   203, 203, 203 },
	{   204, 204, 204 },
	{   205, 205, 205 },
	{   206, 206, 206 },
	{   207, 207, 207 },
	{   208, 208, 208 },
	{   209, 209, 209 },
	{   210, 210, 210 },
	{   211, 211, 211 },
	{   212, 212, 212 },
	{   213, 213, 213 },
	{   214, 214, 214 },
	{   215, 215, 215 },
	{   216, 216, 216 },
	{   217, 217, 217 },
	{   218, 218, 218 },
	{   219, 219, 219 },
	{   220, 220, 220 },
	{   221, 221, 221 },
	{   222, 222, 222 },
	{   223, 223, 223 },
	{   224, 224, 224 },
	{   225, 225, 225 },
	{   226, 226, 226 },
	{   227, 227, 227 },
	{   228, 228, 228 },
	{   229, 229, 229 },
	{   230, 230, 230 },
	{   231, 231, 231 },
	{   232, 232, 232 },
	{   233, 233, 233 },
	{   234, 234, 234 },
	{   235, 235, 235 },
	{   236, 236, 236 },
	{   237, 237, 237 },
	{   238, 238, 238 },
	{   239, 239, 239 },
	{   240, 240, 240 },
	{   241, 241, 241 },
	{   242, 242, 242 },
	{   243, 243, 243 },
	{   244, 244, 244 },
	{   245, 245, 245 },
	{   246, 246, 246 },
	{   247, 247, 247 },
	{   248, 248, 248 },
	{   249, 249, 249 },
	{   250, 250, 250 },
	{   251, 251, 251 },
	{   252, 252, 252 },
	{   253, 253, 253 },
	{   254, 254, 254 },
	{   255, 255, 255 },
} };
