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

#include "encoder.hpp"

#include "utility.hpp"

static const int8_t transition_map[] = {
	 0,	// 0000: noop
	 0,	// 0001: start
	 0,	// 0010: start
	 0,	// 0011: rate
	 1,	// 0100: end
	 0,	// 0101: noop
	 0,	// 0110: rate
	-1,	// 0111: end
	-1,	// 1000: end
	 0,	// 1001: rate
	 0,	// 1010: noop
	 1,	// 1011: end
	 0,	// 1100: rate
	 0,	// 1101: start
	 0,	// 1110: start
	 0,	// 1111: noop
};

int_fast8_t Encoder::update(
	const uint_fast8_t phase_0,
	const uint_fast8_t phase_1
) {
	state <<= 1;
	state |= phase_0;
	state <<= 1;
	state |= phase_1;

	return transition_map[state & 0xf];
}
