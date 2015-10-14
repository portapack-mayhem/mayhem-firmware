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

#include "baseband_ais.hpp"

#include <cstddef>

#include <complex>
#include <array>
#include <memory>

#include <algorithm>
#include <numeric>

namespace dsp {
namespace matched_filter {

template<size_t N>
class MatchedFilter {
public:
	using sample_t = std::complex<float>;
	using tap_t = std::complex<float>;

	using taps_t = std::array<tap_t, N>;

	MatchedFilter(
		const taps_t& taps,
		size_t decimation_factor = 1
	) : taps(taps),
		decimation_factor { decimation_factor }
	{
	}

	bool execute_once(
		const sample_t input
	) {
		samples[samples.size() - decimation_factor + decimation_phase] = input;

		advance_decimation_phase();
		if( is_new_decimation_cycle() ) {
			output = std::inner_product(samples.cbegin(), samples.cend(), taps.cbegin(), sample_t { 0.0f, 0.0f });
			shift_by_decimation_factor();
			return true;
		} else {
			return false;
		}
	}

	sample_t get_output() const {
		return output;
	}

private:
	using samples_t = std::array<sample_t, N>;

	samples_t samples;
	const taps_t taps;
	size_t decimation_factor { 1 };
	size_t decimation_phase { 0 };
	sample_t output;

	void shift_by_decimation_factor() {
		std::rotate(samples.begin(), samples.begin() + decimation_factor, samples.end());
	}

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
