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

#include <cstddef>
#include <complex>
#include <memory>

namespace dsp {
namespace matched_filter {

// This filter contains "magic" (optimizations) that expect the taps to
// combine a low-pass filter with a complex sinusoid that performs shifting of
// the input signal to 0Hz/DC. This also means that the taps length must be
// a multiple of the complex sinusoid period.

class MatchedFilter {
public:
	using sample_t = std::complex<float>;
	using tap_t = std::complex<float>;

	using taps_t = tap_t[];

	template<class T>
	MatchedFilter(
		const T& taps,
		size_t decimation_factor = 1
	) {
		configure(taps, decimation_factor);
	}

	template<class T>
	void configure(
		const T& taps,
		size_t decimation_factor
	) {
		configure(taps.data(), taps.size(), decimation_factor);
 	}

	bool execute_once(const sample_t input);

	float get_output() const {
		return output;
	}

private:
	using samples_t = sample_t[];

	std::unique_ptr<samples_t> samples_ { };
	std::unique_ptr<taps_t> taps_reversed_ { };
	size_t taps_count_ { 0 };
	size_t decimation_factor_ { 1 };
	size_t decimation_phase { 0 };
	float output { 0 };

	void shift_by_decimation_factor();

	void advance_decimation_phase() {
		decimation_phase = (decimation_phase + 1) % decimation_factor_;
	}

	bool is_new_decimation_cycle() const {
		return (decimation_phase == 0);
	}

	void configure(
		const tap_t* const taps,
		const size_t taps_count,
		const size_t decimation_factor
	);
};

} /* namespace matched_filter */
} /* namespace dsp */

#endif/*__MATCHED_FILTER_H__*/
