/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui.hpp"

#include "portapack_persistent_memory.hpp"

#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

namespace serializer {

enum parity_enum : uint8_t {
	NONE = 0,
	EVEN = 1,
	ODD = 2
};

enum order_enum : uint8_t {
	MSB_FIRST = 0,
	LSB_FIRST = 1
};

struct serial_format_t {
	uint8_t data_bits;
	parity_enum parity;
	uint8_t stop_bits;
	order_enum bit_order;

	constexpr serial_format_t() :
		data_bits(7),
		parity(parity_enum::EVEN),
		stop_bits(1),
		bit_order(order_enum::LSB_FIRST)
	{
	}
};

size_t symbol_count(const serial_format_t& serial_format);

	/*{ "7-Even-1 R", "7E1", 7, EVEN,	1, false, false },
	{ "7E1 LUT   ", "7Ea", 7, EVEN,	1, true, true },
	{ "7-Odd-1   ", "7o1", 7, ODD,	1, true, false },
	{ "8-Even-0  ", "8E0", 8, EVEN,	1, true, false }*/

} /* namespace serializer */

#endif/*__SERIALIZER_H__*/
