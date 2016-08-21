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

#ifndef __TUNING_H__
#define __TUNING_H__

#include "rf_path.hpp"

namespace tuning {
namespace config {

struct Config {
	/* Empty config to denote an error, in lieu of throwing an exception. */
	constexpr Config(
	) : first_lo_frequency(0),
		second_lo_frequency(0),
		rf_path_band(rf::path::Band::Mid),
		baseband_invert(false)
	{
	}

	constexpr Config(
		rf::Frequency first_lo_frequency,
		rf::Frequency second_lo_frequency,
		rf::path::Band rf_path_band,
		bool baseband_invert
	) : first_lo_frequency(first_lo_frequency),
		second_lo_frequency(second_lo_frequency),
		rf_path_band(rf_path_band),
		baseband_invert(baseband_invert)
	{
	}

	bool is_valid() const {
		return (second_lo_frequency != 0);
	}

	const rf::Frequency first_lo_frequency;
	const rf::Frequency second_lo_frequency;
	const rf::path::Band rf_path_band;
	const bool baseband_invert;
};

Config create(const rf::Frequency target_frequency);

} /* namespace config */
} /* namespace tuning */

#endif/*__TUNING_H__*/
