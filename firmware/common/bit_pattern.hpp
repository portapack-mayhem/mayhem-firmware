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

#ifndef __BIT_PATTERN_H__
#define __BIT_PATTERN_H__

#include <cstdint>
#include <cstddef>

class BitPattern {
public:
	constexpr BitPattern(
		const uint32_t code,
		const size_t code_length,
		const size_t maximum_hanning_distance = 0
	) : code_ { code },
		mask_ { (1UL << code_length) - 1UL },
		maximum_hanning_distance_ { maximum_hanning_distance }
	{
	}

	uint32_t code() const {
		return code_;
	}

	uint32_t mask() const {
		return mask_;
	}

	size_t maximum_hanning_distance() const {
		return maximum_hanning_distance_;
	}

private:
	uint32_t code_;
	uint32_t mask_;
	size_t maximum_hanning_distance_;
};

class BitHistory {
public:
	void add(const uint_fast8_t bit) {
		history = (history << 1) | (bit & 1);
	}

	bool matches(const BitPattern& pattern) const {
		const auto delta_bits = (history ^ pattern.code()) & pattern.mask();
		const size_t count = __builtin_popcountl(delta_bits);
		return (count <= pattern.maximum_hanning_distance());
	}

private:
	uint32_t history { 0 };
};

#endif/*__BIT_PATTERN_H__*/
