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

#define CORDIC_TAB 16

static int32_t cordic_tab [CORDIC_TAB] = {
	12867, 7596, 4013, 2037, 1022, 511, 255, 
	127, 63, 31, 15, 7, 3, 1, 0, 0, 
};

#define CORDIC_SCALE	16384.0
