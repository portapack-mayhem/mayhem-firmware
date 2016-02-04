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

#include "baseband_stats_collector.hpp"

#include "lpc43xx_cpp.hpp"

bool BasebandStatsCollector::process(const buffer_c8_t& buffer) {
	samples += buffer.count;

	const size_t report_samples = buffer.sampling_rate * report_interval;
	const auto report_delta = samples - samples_last_report;
	return report_delta >= report_samples;
}

BasebandStatistics BasebandStatsCollector::capture_statistics() {
	BasebandStatistics statistics;

	const auto idle_ticks = thread_idle->total_ticks;
	statistics.idle_ticks = (idle_ticks - last_idle_ticks);
	last_idle_ticks = idle_ticks;

	const auto main_ticks = thread_main->total_ticks;
	statistics.main_ticks = (main_ticks - last_main_ticks);
	last_main_ticks = main_ticks;

	const auto rssi_ticks = thread_rssi->total_ticks;
	statistics.rssi_ticks = (rssi_ticks - last_rssi_ticks);
	last_rssi_ticks = rssi_ticks;

	const auto baseband_ticks = thread_baseband->total_ticks;
	statistics.baseband_ticks = (baseband_ticks - last_baseband_ticks);
	last_baseband_ticks = baseband_ticks;

	statistics.saturation = lpc43xx::m4::flag_saturation();
	lpc43xx::m4::clear_flag_saturation();

	samples_last_report = samples;

	return statistics;
}
