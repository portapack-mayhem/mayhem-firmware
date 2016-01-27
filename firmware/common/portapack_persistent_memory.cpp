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

#include "portapack_persistent_memory.hpp"

#include "hal.h"

#include "utility.hpp"

#include <algorithm>
#include <utility>

namespace portapack {
namespace persistent_memory {

using tuned_frequency_range_t = range_t<rf::Frequency>;
constexpr tuned_frequency_range_t tuned_frequency_range { rf::tuning_range.min, rf::tuning_range.max };
constexpr rf::Frequency tuned_frequency_reset_value { 858750000 };

using ppb_range_t = range_t<ppb_t>;
constexpr ppb_range_t ppb_range { -99000, 99000 };
constexpr ppb_t ppb_reset_value { 0 };

/* struct must pack the same way on M4 and M0 cores. */
struct data_t {
	int64_t tuned_frequency;
	int32_t correction_ppb;
};

static_assert(sizeof(data_t) <= 0x100, "Persistent memory structure too large for VBAT-maintained region");

static data_t* const data = reinterpret_cast<data_t*>(LPC_BACKUP_REG_BASE);

rf::Frequency tuned_frequency() {
	tuned_frequency_range.reset_if_outside(data->tuned_frequency, tuned_frequency_reset_value);
	return data->tuned_frequency;
}

void set_tuned_frequency(const rf::Frequency new_value) {
	data->tuned_frequency = tuned_frequency_range.clip(new_value);
}

ppb_t correction_ppb() {
	ppb_range.reset_if_outside(data->correction_ppb, ppb_reset_value);
	return data->correction_ppb;
}

void set_correction_ppb(const ppb_t new_value) {
	data->correction_ppb = ppb_range.clip(new_value);
}

} /* namespace persistent_memory */
} /* namespace portapack */
