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

#ifndef __ACCESS_CODE_CORRELATOR_H__
#define __ACCESS_CODE_CORRELATOR_H__

#include <cstdint>
#include <cstddef>

class AccessCodeCorrelator {
public:
	void configure(
		const uint32_t new_code,
		const size_t new_code_length,
		const size_t new_maximum_hamming_distance
	);

	bool operator()(
		const uint_fast8_t in
	) {
		history = (history << 1) | (in & 1);
		const auto delta_bits = (history ^ code) & mask;
		//const size_t count = __builtin_popcountll(delta_bits);
		const size_t count = __builtin_popcountl(delta_bits);
		return (count <= maximum_hamming_distance);
	}

private:
	uint32_t code { 0 };
	uint32_t mask { 0 };
	uint32_t history { 0 };
	size_t maximum_hamming_distance { 0 };

	static constexpr uint32_t mask_value(const size_t n) {
		return static_cast<uint32_t>((1ULL << n) - 1ULL);
	}
};

#endif/*__ACCESS_CODE_CORRELATOR_H__*/
