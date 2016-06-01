/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __PHASE_DETECTOR_HPP__
#define __PHASE_DETECTOR_HPP__

#include <cstdint>
#include <cstddef>
#include <cmath>

class PhaseDetectorEarlyLateGate {
public:
	using history_t = uint32_t;

	using symbol_t = bool;
	using error_t = int;

	struct result_t {
		symbol_t symbol;
		error_t error;
	};

	constexpr PhaseDetectorEarlyLateGate(
		const size_t samples_per_symbol
	) : sample_threshold { samples_per_symbol / 2 },
		late_mask { (1UL << sample_threshold) - 1UL },
		early_mask { late_mask << sample_threshold }
	{
	}

	result_t operator()(const history_t symbol_history) const {
		static_assert(sizeof(history_t) == sizeof(unsigned long), "popcountl size mismatch");

		// history = ...0111, early
		// history = ...1110, late

		const size_t late_side = __builtin_popcountl(symbol_history & late_mask);
		const size_t early_side = __builtin_popcountl(symbol_history & early_mask);
		const size_t total_count = late_side + early_side;
		const auto lateness = static_cast<int>(late_side) - static_cast<int>(early_side);
		const symbol_t symbol = (total_count >= sample_threshold);
		const error_t error = symbol ? -lateness : lateness;
		return { symbol, error };
	}

private:
	const size_t sample_threshold;
	const history_t late_mask;
	const history_t early_mask;
};

#endif/*__PHASE_DETECTOR_HPP__*/
