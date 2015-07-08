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

#ifndef __RF_PATH_H__
#define __RF_PATH_H__

#include <cstdint>

namespace rf {

using Frequency = int64_t;

struct FrequencyRange {
	Frequency min;
	Frequency max;
	/* TODO: static_assert low < high? */

	bool below_range(const Frequency f) const {
		return f < min;
	}

	bool contains(const Frequency f) const {
		return (f >= min) && (f < max);
	}

	bool out_of_range(const Frequency f) const {
		return !contains(f);
	}
};

enum class Direction {
	/* Zero-based, used as index into table */
	Receive = 0,
	Transmit = 1,
};

namespace path {

constexpr FrequencyRange band_low {
	.min = 0,
	.max = 2150000000,
};

constexpr FrequencyRange band_high {
	.min = 2750000000,
	.max = 7250000000,
};

constexpr FrequencyRange band_mid {
	.min = band_low.max,
	.max = band_high.min,
};

enum class Band {
	/* Zero-based, used as index into frequency_bands table */
	Low = 0,
	Mid = 1,
	High = 2,
};

class Path {
public:
	void init();

	void set_direction(const Direction direction);
	void set_band(const Band band);
	void set_rf_amp(const bool rf_amp);

private:
	Direction direction { Direction::Receive };
	Band band { Band::Mid };
	bool rf_amp { false };

	void update();
};

} /* path */

constexpr FrequencyRange tuning_range {
	.min = path::band_low.min,
	.max = path::band_high.max,
};

} /* rf */

#endif/*__RF_PATH_H__*/
