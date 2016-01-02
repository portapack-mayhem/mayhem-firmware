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

#include "proc_nfm_audio.hpp"

#include <cstdint>
#include <cstddef>

NarrowbandFMAudio::NarrowbandFMAudio() {
	set_mode(Mode::Deviation2K5Narrow);
}

void NarrowbandFMAudio::set_mode(const Mode mode) {
	constexpr size_t baseband_fs = 3072000;

	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_decimation_factor = 8;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0_decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_decimation_factor = 8;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1_decimation_factor;

	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	constexpr size_t channel_filter_decimation_factor = 1;
	constexpr size_t channel_filter_output_fs = channel_filter_input_fs / channel_filter_decimation_factor;

	constexpr size_t demod_input_fs = channel_filter_output_fs;

	switch(mode) {
	default:
	case Mode::Deviation5K:
		decim_0.configure(taps_16k0_decim_0.taps, 33554432);
		decim_1.configure(taps_16k0_decim_1.taps, 131072);
		channel_filter.configure(taps_16k0_channel.taps, channel_filter_decimation_factor);
		demod.configure(demod_input_fs, 5000);
		channel_filter_pass_f = taps_16k0_channel.pass_frequency_normalized * channel_filter_input_fs;
		channel_filter_stop_f = taps_16k0_channel.stop_frequency_normalized * channel_filter_input_fs;
		break;

	case Mode::Deviation2K5Wide:
		decim_0.configure(taps_11k0_decim_0.taps, 33554432);
		decim_1.configure(taps_11k0_decim_1.taps, 131072);
		channel_filter.configure(taps_11k0_channel.taps, channel_filter_decimation_factor);
		demod.configure(demod_input_fs, 2500);
		channel_filter_pass_f = taps_11k0_channel.pass_frequency_normalized * channel_filter_input_fs;
		channel_filter_stop_f = taps_11k0_channel.stop_frequency_normalized * channel_filter_input_fs;
		break;

	case Mode::Deviation2K5Narrow:
		decim_0.configure(taps_4k25_decim_0.taps, 33554432);
		decim_1.configure(taps_4k25_decim_1.taps, 131072);
		channel_filter.configure(taps_4k25_channel.taps, channel_filter_decimation_factor);
		demod.configure(demod_input_fs, 2500);
		channel_filter_pass_f = taps_4k25_channel.pass_frequency_normalized * channel_filter_input_fs;
		channel_filter_stop_f = taps_4k25_channel.stop_frequency_normalized * channel_filter_input_fs;
		break;
	}

	channel_spectrum.set_decimation_factor(std::floor((channel_filter_output_fs / 2) / ((channel_filter_pass_f + channel_filter_stop_f) / 2)));
}

void NarrowbandFMAudio::execute(const buffer_c8_t& buffer) {
	std::array<complex16_t, 512> dst;
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	feed_channel_stats(channel_out);
	channel_spectrum.feed(channel_out, channel_filter_pass_f, channel_filter_stop_f);

	const buffer_s16_t work_audio_buffer {
		(int16_t*)dst.data(),
		sizeof(*dst.data()) * dst.size()
	};

	auto audio = demod.execute(channel_out, work_audio_buffer);

	static uint64_t audio_present_history = 0;
	const auto audio_present_now = squelch.execute(audio);
	audio_present_history = (audio_present_history << 1) | (audio_present_now ? 1 : 0);
	const bool audio_present = (audio_present_history != 0);

	audio_hpf.execute_in_place(audio);
	audio_deemph.execute_in_place(audio);

	if( !audio_present ) {
		// Zero audio buffer.
		for(size_t i=0; i<audio.count; i++) {
			audio.p[i] = 0;
		}
	}

	fill_audio_buffer(audio);
}
