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

#ifndef __BASEBAND_STATS_COLLECTOR_H__
#define __BASEBAND_STATS_COLLECTOR_H__

#include "ch.h"

#include "dsp_types.hpp"
#include "message.hpp"
#include "utility.hpp"

#include <cstdint>
#include <cstddef>

class BasebandStatsCollector {
public:
	template<typename Callback>
	void process(buffer_c8_t buffer, Callback callback) {
		samples += buffer.count;

		const size_t report_samples = buffer.sampling_rate * report_interval;
		const auto report_delta = samples - samples_last_report;
		if( report_delta >= report_samples ) {
			const auto idle_ticks = chSysGetIdleThread()->total_ticks;
			statistics.idle_ticks = (idle_ticks - last_idle_ticks);
			last_idle_ticks = idle_ticks;

			const auto baseband_ticks = chThdSelf()->total_ticks;
			statistics.baseband_ticks = (baseband_ticks - last_baseband_ticks);
			last_baseband_ticks = baseband_ticks;

			statistics.saturation = m4_flag_saturation();
			clear_m4_flag_saturation();

			callback(statistics);

			samples_last_report = samples;
		}
	}

private:
	static constexpr float report_interval { 1.0f };
	BasebandStatistics statistics;
	size_t samples { 0 };
	size_t samples_last_report { 0 };
	uint32_t last_idle_ticks { 0 };
	uint32_t last_baseband_ticks { 0 };
};

#endif/*__BASEBAND_STATS_COLLECTOR_H__*/
