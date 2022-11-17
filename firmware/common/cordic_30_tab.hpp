/*
 * Copyright (C) 2022 Belousov Oleg
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

#define CORDIC_TAB 30

static int32_t cordic_tab [CORDIC_TAB] = {
	210828714, 124459457, 65760959, 33381289, 
	16755421, 8385878, 4193962, 2097109, 
	1048570, 524287, 262143, 131071, 
	65535, 32767, 16383, 8191, 
	4095, 2047, 1023, 511, 255, 
	127, 63, 31, 15, 7, 3, 2, 1, 0, 
};

#define CORDIC_SCALE	268435456.0
