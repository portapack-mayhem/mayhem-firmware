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

#ifndef __MATCHED_FILTER_H__
#define __MATCHED_FILTER_H__

#include "utility.hpp"

#include <cstddef>

#include <complex>
#include <array>
#include <memory>

#include <algorithm>
#include <numeric>

namespace dsp {
namespace matched_filter {

class MatchedFilter {
public:
	using sample_t = std::complex<float>;
	using tap_t = std::complex<float>;

	using taps_t = tap_t[];

	MatchedFilter(
		const tap_t* const taps,
		const size_t taps_count,
		size_t decimation_factor = 1
	) : samples_ { std::make_unique<samples_t>(taps_count) },
		taps_reversed_ { std::make_unique<taps_t>(taps_count) },
		taps_count_ { taps_count },
		decimation_factor { decimation_factor }
	{
		std::reverse_copy(&taps[0], &taps[taps_count], &taps_reversed_[0]);
	}

	template<typename T>
	MatchedFilter(
		const T& taps,
		size_t decimation_factor = 1
	) : MatchedFilter(taps.data(), taps.size(), decimation_factor)
	{
	}

	bool execute_once(const sample_t input);

	float get_output() const {
		return output;
	}

private:
	using samples_t = sample_t[];

	const std::unique_ptr<samples_t> samples_;
	const std::unique_ptr<taps_t> taps_reversed_;
	const size_t taps_count_;
	size_t decimation_factor { 1 };
	size_t decimation_phase { 0 };
	float output;

	void shift_by_decimation_factor();

	void advance_decimation_phase() {
		decimation_phase = (decimation_phase + 1) % decimation_factor;
	}

	bool is_new_decimation_cycle() {
		return (decimation_phase == 0);
	}
};

} /* namespace matched_filter */
} /* namespace dsp */

#endif/*__MATCHED_FILTER_H__*/
