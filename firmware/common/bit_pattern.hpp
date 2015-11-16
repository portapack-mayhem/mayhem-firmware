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

class BitHistory {
public:
	void add(const uint_fast8_t bit) {
		history = (history << 1) | (bit & 1);
	}

	uint64_t value() const {
		return history;
	}

private:
	uint64_t history { 0 };
};

class BitPattern {
public:
	constexpr BitPattern(
	) : code_ { 0 },
		mask_ { 0 },
		maximum_hanning_distance_ { 0 }
	{
	}
	
	constexpr BitPattern(
		const uint64_t code,
		const size_t code_length,
		const size_t maximum_hanning_distance = 0
	) : code_ { code },
		mask_ { (1ULL << code_length) - 1ULL },
		maximum_hanning_distance_ { maximum_hanning_distance }
	{
	}

	bool operator()(const BitHistory& history, const size_t) const {
		const auto delta_bits = (history.value() ^ code_) & mask_;
		const size_t count = __builtin_popcountll(delta_bits);
		return (count <= maximum_hanning_distance_);
	}

private:
	uint64_t code_;
	uint64_t mask_;
	size_t maximum_hanning_distance_;
};

#endif/*__BIT_PATTERN_H__*/
