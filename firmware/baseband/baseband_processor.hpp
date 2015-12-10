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

#ifndef __BASEBAND_PROCESSOR_H__
#define __BASEBAND_PROCESSOR_H__

#include "dsp_types.hpp"
#include "complex.hpp"

#include "block_decimator.hpp"
#include "channel_stats_collector.hpp"
#include "audio_stats_collector.hpp"

#include <array>
#include <cstdint>
#include <complex>

class BasebandProcessor {
public:
	virtual ~BasebandProcessor() = default;

	virtual void execute(buffer_c8_t& buffer) = 0;

	void update_spectrum();

protected:
	void feed_channel_stats(const buffer_c16_t& channel);

	void feed_channel_spectrum(
		const buffer_c16_t& channel,
		const uint32_t filter_pass_frequency,
		const uint32_t filter_stop_frequency
	);

	void fill_audio_buffer(const buffer_s16_t& audio);

	volatile bool channel_spectrum_request_update { false };
	std::array<std::complex<float>, 256> channel_spectrum;
	uint32_t channel_spectrum_sampling_rate { 0 };
	uint32_t channel_filter_pass_frequency { 0 };
	uint32_t channel_filter_stop_frequency { 0 };

private:
	BlockDecimator<256> channel_spectrum_decimator { 4 };

	ChannelStatsCollector channel_stats;
	AudioStatsCollector audio_stats;

	void post_channel_stats_message(const ChannelStatistics& statistics);
	void post_channel_spectrum_message(const buffer_c16_t& data);
	void feed_audio_stats(const buffer_s16_t& audio);
	void post_audio_stats_message(const AudioStatistics& statistics);
};

#endif/*__BASEBAND_PROCESSOR_H__*/
