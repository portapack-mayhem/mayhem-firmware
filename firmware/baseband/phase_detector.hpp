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
		const float samples_per_symbol
	) : late_mask { (1U << static_cast<size_t>(std::ceil(samples_per_symbol / 2))) - 1 },
		early_mask { late_mask << static_cast<size_t>(std::floor(samples_per_symbol / 2)) },
		sample_bit { static_cast<size_t>(std::floor(samples_per_symbol / 2)) }
	{
	}

	result_t operator()(const history_t symbol_history) const {
		// history = ...0111, early
		// history = ...1110, late

		const symbol_t symbol = (symbol_history >> sample_bit) & 1;
		const int late_side = __builtin_popcount(symbol_history & late_mask);
		const int early_side = __builtin_popcount(symbol_history & early_mask);
		const int lateness = late_side - early_side;
		const int direction = lateness; //std::min(std::max(lateness, -1), 1);
		const error_t error = direction;
		return { symbol, error };
	}

private:
	const history_t late_mask;
	const history_t early_mask;
	const size_t sample_bit;
};

#endif/*__PHASE_DETECTOR_HPP__*/
