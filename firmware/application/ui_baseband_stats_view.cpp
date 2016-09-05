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

#include "ui_baseband_stats_view.hpp"

#include <string>
#include <algorithm>

#include "hackrf_hal.hpp"
using namespace hackrf::one;

#include "string_format.hpp"

namespace ui {

/* BasebandStatsView *****************************************************/

BasebandStatsView::BasebandStatsView() {
	add_children({
		&text_stats,
	});
}

static std::string ticks_to_percent_string(const uint32_t ticks) {
	constexpr size_t decimal_digits = 1;
	constexpr size_t decimal_factor = decimal_digits * 10;

 	const uint32_t percent_x10 = ticks / (base_m4_clk_f / (100 * decimal_factor));
 	const uint32_t percent_x10_clipped = std::min(percent_x10, static_cast<uint32_t>(100 * decimal_factor) - 1);
	return
		to_string_dec_uint(percent_x10_clipped / decimal_factor, 2) + "." +
		to_string_dec_uint(percent_x10_clipped % decimal_factor, decimal_digits, '0');
}

void BasebandStatsView::on_statistics_update(const BasebandStatistics& statistics) {
	std::string message = ticks_to_percent_string(statistics.idle_ticks)
		+ " " + ticks_to_percent_string(statistics.main_ticks)
		+ " " + ticks_to_percent_string(statistics.rssi_ticks)
		+ " " + ticks_to_percent_string(statistics.baseband_ticks);

	text_stats.set(message);
}

} /* namespace ui */
