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

#ifndef __CHANNEL_STATS_COLLECTOR_H__
#define __CHANNEL_STATS_COLLECTOR_H__

#include "dsp_types.hpp"
#include "message.hpp"
#include "utility.hpp"

#include <cstdint>
#include <cstddef>

#include <hal.h>

class ChannelStatsCollector {
public:
	template<typename Callback>
	void feed(const buffer_c16_t& src, Callback callback) {
		void *src_p = src.p;
		while(src_p < &src.p[src.count]) {
			const uint32_t sample = *__SIMD32(src_p)++;
			const uint32_t mag_sq = __SMUAD(sample, sample);
			if( mag_sq > max_squared ) {
				max_squared = mag_sq;
			}
		}
		count += src.count;

		const size_t samples_per_update = src.sampling_rate * update_interval;

		if( count >= samples_per_update ) {
			const float max_squared_f = max_squared;
			const int32_t max_db = mag2_to_dbv_norm(max_squared_f * (1.0f / (32768.0f * 32768.0f)));
			callback({ max_db, count });

			max_squared = 0;
			count = 0;
		}
	}

private:
	static constexpr float update_interval { 0.1f };
	uint32_t max_squared { 0 };
	size_t count { 0 };
};

#endif/*__CHANNEL_STATS_COLLECTOR_H__*/
