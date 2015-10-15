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

#include "matched_filter.hpp"

// TODO: Move the fast complex multiply code to another place.
#include "dsp_fft.hpp"

namespace dsp {
namespace matched_filter {

bool MatchedFilter::execute_once(
	const sample_t input
) {
	samples_[taps_count_ - decimation_factor + decimation_phase] = input;

	advance_decimation_phase();
	if( is_new_decimation_cycle() ) {
		float r_n = 0.0f;
		float i_n = 0.0f;
		float r_p = 0.0f;
		float i_p = 0.0f;
		for(size_t n=0; n<taps_count_; n++) {
			const auto sample = samples_[n];
			const auto tap = taps_reversed_[n];

			// N: complex multiple of samples and taps (conjugate, tap.i negated).
			// P: complex multiply of samples and taps.
			r_n += sample.real() * tap.real();
			i_n -= sample.real() * tap.imag();
			r_n += sample.imag() * tap.imag();
			i_n += sample.imag() * tap.real();

			r_p += sample.real() * tap.real();
			i_p += sample.real() * tap.imag();
			r_p -= sample.imag() * tap.imag();
			i_p += sample.imag() * tap.real();
		}

		const auto mag_n = std::sqrt(r_n * r_n + i_n * i_n);
		const auto mag_p = std::sqrt(r_p * r_p + i_p * i_p);
		const auto diff = mag_p - mag_n;
		output = diff;

		shift_by_decimation_factor();
		return true;
	} else {
		return false;
	}
}

void MatchedFilter::shift_by_decimation_factor() {
	std::move(&samples_[decimation_factor], &samples_[taps_count_], &samples_[0]);
}

} /* namespace matched_filter */
} /* namespace dsp */
