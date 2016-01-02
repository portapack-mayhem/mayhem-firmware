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

#include "proc_wfm_audio.hpp"

#include <cstdint>

WidebandFMAudio::WidebandFMAudio() {
	constexpr size_t baseband_fs = 3072000;

	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_decimation_factor = 4;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0_decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_decimation_factor = 2;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1_decimation_factor;

	constexpr size_t demod_input_fs = decim_1_output_fs;

	decim_0.configure(taps_200k_wfm_decim_0.taps, 33554432);
	decim_1.configure(taps_200k_wfm_decim_1.taps, 131072);
	demod.configure(demod_input_fs, 75000);
	audio_filter.configure(taps_64_lp_156_198.taps);
}

void WidebandFMAudio::execute(const buffer_c8_t& buffer) {
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto decimator_out = decim_1_out;

	const buffer_s16_t work_audio_buffer {
		(int16_t*)decimator_out.p,
		sizeof(*decimator_out.p) * decimator_out.count
	};

	auto channel = decimator_out;

	// TODO: Feed channel_stats post-decimation data?
	feed_channel_stats(channel);
	//feed_channel_spectrum(channel);

	/* 384kHz complex<int16_t>[256]
	 * -> FM demodulation
	 * -> 384kHz int16_t[256] */
	/* TODO: To improve adjacent channel rejection, implement complex channel filter:
	 *		pass < +/- 100kHz, stop > +/- 200kHz
	 */

	auto audio_oversampled = demod.execute(decimator_out, work_audio_buffer);

	/* 384kHz int16_t[256]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 192kHz int16_t[128] */
	auto audio_4fs = audio_dec_1.execute(audio_oversampled, work_audio_buffer);

	/* 192kHz int16_t[128]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 96kHz int16_t[64] */
	auto audio_2fs = audio_dec_2.execute(audio_4fs, work_audio_buffer);

	/* 96kHz int16_t[64]
	 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop, gain of 1
	 * -> 48kHz int16_t[32] */
	auto audio = audio_filter.execute(audio_2fs, work_audio_buffer);

	/* -> 48kHz int16_t[32] */
	audio_hpf.execute_in_place(audio);
	audio_deemph.execute_in_place(audio);

	fill_audio_buffer(audio);
}
