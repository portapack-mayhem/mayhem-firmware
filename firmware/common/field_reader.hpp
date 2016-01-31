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

#ifndef __FIELD_READER_H__
#define __FIELD_READER_H__

#include <cstdint>
#include <cstddef>

struct BitRemapNone {
	constexpr size_t operator()(const size_t& bit_index) const {
		return bit_index;
	}
};

struct BitRemapByteReverse {
	constexpr size_t operator()(const size_t bit_index) const {
		return bit_index ^ 7;
	}
};

template<typename T, typename BitRemap>
class FieldReader {
public:
	constexpr FieldReader(
		const T& data
	) : data { data }
	{
	}

	/* The "start_bit" winds up being the MSB of the returned field value. */
	/* The BitRemap functor determines which bits are read from the source
	 * packet. */
	uint32_t read(const size_t start_bit, const size_t length) const {
		uint32_t value = 0;
		for(size_t i=start_bit; i<(start_bit + length); i++) {
			value = (value << 1) | data[bit_remap(i)];
		}
		return value;
	}

private:
	const T& data;
	const BitRemap bit_remap { };
};

#endif/*__FIELD_READER_H__*/
