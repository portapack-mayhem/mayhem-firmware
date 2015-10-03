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

namespace dsp {
namespace matched_filter {

bool MatchedFilter::execute_once(
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

void MatchedFilter::shift_by_decimation_factor() {
	std::rotate(samples.begin(), samples.begin() + decimation_factor, samples.end());
}

} /* namespace matched_filter */
} /* namespace dsp */
