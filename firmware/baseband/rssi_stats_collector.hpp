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

#ifndef __RSSI_STATS_COLLECTOR_H__
#define __RSSI_STATS_COLLECTOR_H__

#include "rssi.hpp"
#include "message.hpp"

#include <cstdint>
#include <cstddef>

class RSSIStatisticsCollector {
public:
	template<typename Callback>
	void process(const rf::rssi::buffer_t& buffer, Callback callback) {
		auto p = buffer.p;
		if( p == nullptr ) {
			return;
		}

		if( statistics.count == 0 ) {
			const auto value_0 = *p;
			statistics.min = value_0;
			statistics.max = value_0;
		}
		
		const auto end = &p[buffer.count];
		while(p < end) {
			const uint32_t value = *(p++);

			if( statistics.min > value ) {
				statistics.min = value;
			}
			if( statistics.max < value ) {
				statistics.max = value;
			}

			statistics.accumulator += value;
		}
		statistics.count += buffer.count;

		const size_t samples_per_update = buffer.sampling_rate * update_interval;

		if( statistics.count >= samples_per_update ) {
			callback(statistics);
			statistics.accumulator = 0;
			statistics.count = 0;
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	RSSIStatistics statistics { };
};

#endif/*__RSSI_STATS_COLLECTOR_H__*/
