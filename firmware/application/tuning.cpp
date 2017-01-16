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

#include "tuning.hpp"

#include "utility.hpp"

namespace tuning {
namespace config {

namespace {

constexpr rf::Frequency low_band_second_lo_frequency(const rf::Frequency target_frequency) {
	return 2650000000 - (target_frequency / 7);
}

constexpr rf::Frequency high_band_second_lo_regions_2_and_3(const rf::Frequency target_frequency) {
	return (target_frequency < 5100000000)
		? (2350000000 + ((target_frequency - 3600000000) / 5))
		: (2500000000 + ((target_frequency - 5100000000) / 9))
		;
}

constexpr rf::Frequency high_band_second_lo_frequency(const rf::Frequency target_frequency) {
	return (target_frequency < 3600000000)
		? (2150000000 + (((target_frequency - 2750000000) * 60) / 85))
		: high_band_second_lo_regions_2_and_3(target_frequency)
		;
}

Config low_band(const rf::Frequency target_frequency) {
	const rf::Frequency first_lo_frequency = target_frequency + low_band_second_lo_frequency(target_frequency);
	const rf::Frequency second_lo_frequency = first_lo_frequency - target_frequency;
	const bool baseband_invert = true;
	return { first_lo_frequency, second_lo_frequency, rf::path::Band::Low, baseband_invert };
}

Config mid_band(const rf::Frequency target_frequency) {
	return { 0, target_frequency, rf::path::Band::Mid, false };
}

Config high_band(const rf::Frequency target_frequency) {
	const rf::Frequency first_lo_frequency = target_frequency - high_band_second_lo_frequency(target_frequency);
	const rf::Frequency second_lo_frequency = target_frequency - first_lo_frequency;
	const bool baseband_invert = false;
	return { first_lo_frequency, second_lo_frequency, rf::path::Band::High, baseband_invert };
}

} /* namespace */

Config create(const rf::Frequency target_frequency) {
	/* TODO: This is some lame code. */
	if( rf::path::band_low.contains(target_frequency) ) {
		return low_band(target_frequency);
	} else if( rf::path::band_mid.contains(target_frequency) ) {
		return mid_band(target_frequency);
	} else if( rf::path::band_high.contains(target_frequency) ) {
		return high_band(target_frequency);
	} else {
		return { };
	}
}

} /* namespace config */
} /* namespace tuning */
