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

#include <cstdint>
#include <array>

#include "wm8731.hpp"

namespace wolfson {
namespace wm8731 {

void WM8731::write(const address_t reg_address, const reg_t value) {
	const uint16_t word = (reg_address << 9) | value;
	const std::array<uint8_t, 2> values {
		static_cast<uint8_t>(word >> 8),
		static_cast<uint8_t>(word & 0xff),
	};
	bus.transmit(bus_address, values.data(), values.size());
}

} /* namespace wm8731 */
} /* namespace wolfson */
