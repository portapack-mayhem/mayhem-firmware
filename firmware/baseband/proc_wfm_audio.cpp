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

void WidebandFMAudio::execute(const buffer_c8_t& buffer) {
	auto decimator_out = decimator.execute(buffer);

	const buffer_s16_t work_audio_buffer {
		(int16_t*)decimator_out.p,
		sizeof(*decimator_out.p) * decimator_out.count
	};

	auto channel = decimator_out;

	// TODO: Feed channel_stats post-decimation data?
	feed_channel_stats(channel);
	//feed_channel_spectrum(channel);

	/* 768kHz complex<int16_t>[512]
	 * -> FM demodulation
	 * -> 768kHz int16_t[512] */
	/* TODO: To improve adjacent channel rejection, implement complex channel filter:
	 *		pass < +/- 100kHz, stop > +/- 200kHz
	 */

	auto audio_oversampled = demod.execute(decimator_out, work_audio_buffer);

	/* 768kHz int16_t[512]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 384kHz int16_t[256] */
	auto audio_8fs = audio_dec_1.execute(audio_oversampled, work_audio_buffer);

	/* 384kHz int16_t[256]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 192kHz int16_t[128] */
	auto audio_4fs = audio_dec_2.execute(audio_8fs, work_audio_buffer);

	/* 192kHz int16_t[128]
	 * -> 4th order CIC decimation by 2, gain of 1
	 * -> 96kHz int16_t[64] */
	auto audio_2fs = audio_dec_3.execute(audio_4fs, work_audio_buffer);

	/* 96kHz int16_t[64]
	 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop, gain of 1
	 * -> 48kHz int16_t[32] */
	auto audio = audio_filter.execute(audio_2fs, work_audio_buffer);

	/* -> 48kHz int16_t[32] */
	audio_hpf.execute_in_place(audio);
	fill_audio_buffer(audio);
}
