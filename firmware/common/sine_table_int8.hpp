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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __SINE_TABLE_I8_H__
#define __SINE_TABLE_I8_H__

#include <cmath>

static const int8_t sine_table_i8[256] = {
	0, 2, 5, 8, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45,
	48, 51, 54, 57, 59, 62, 65, 67, 70, 73, 75, 78, 80, 83, 85, 87,
	90, 92, 94, 96, 98, 100, 102, 104, 105, 107, 109, 110, 112, 113, 115, 116,
	117, 118, 120, 121, 121, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127,
	127, 127, 127, 127, 126, 126, 126, 125, 125, 124, 123, 122, 121, 121, 120, 118,
	117, 116, 115, 113, 112, 110, 109, 107, 105, 104, 102, 100, 98, 96, 94, 92,
	90, 87, 85, 83, 80, 78, 75, 73, 70, 67, 65, 62, 59, 57, 54, 51,
	48, 45, 42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12, 8, 5, 2,
	0, -3, -6, -9, -13, -16, -19, -22, -25, -28, -31, -34, -37, -40, -43, -46,
	-49, -52, -55, -58, -60, -63, -66, -68, -71, -74, -76, -79, -81, -84, -86, -88,
	-91, -93, -95, -97, -99, -101, -103, -105, -106, -108, -110, -111, -113, -114, -116, -117,
	-118, -119, -121, -122, -122, -123, -124, -125, -126, -126, -127, -127, -127, -128, -128, -128,
	-128, -128, -128, -128, -127, -127, -127, -126, -126, -125, -124, -123, -122, -122, -121, -119,
	-118, -117, -116, -114, -113, -111, -110, -108, -106, -105, -103, -101, -99, -97, -95, -93,
	-91, -88, -86, -84, -81, -79, -76, -74, -71, -68, -66, -63, -60, -58, -55, -52,
	-49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -13, -9, -6, -3
};

#endif/*__SINE_TABLE_I8_H__*/
