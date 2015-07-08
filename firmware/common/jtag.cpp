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

#include "jtag.hpp"

#include <cstdint>
#include <cstddef>

namespace jtag {

uint32_t JTAG::shift(const size_t count, uint32_t value) {
	for(size_t i=0; i<count; i++) {
		const auto tdo = target.clock(
			(i == (count - 1)) ? 1 : 0,
			value & 1
		);
		value >>= 1;
		value |= tdo << (count - 1);
	}
	return value;
}

} /* namespace jtag */
