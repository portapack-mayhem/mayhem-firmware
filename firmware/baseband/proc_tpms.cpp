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

#include "proc_tpms.hpp"

#include "portapack_shared_memory.hpp"

#include "i2s.hpp"
using namespace lpc43xx;

#include "dsp_fir_taps.hpp"

// IFIR image-reject filter: fs=2457600, pass=100000, stop=407200, decim=4, fout=614400
static constexpr fir_taps_real<24> taps_200k_decim_0 = {
	.pass_frequency_normalized = 100000.0f / 2457600.0f,
	.stop_frequency_normalized = 407200.0f / 2457600.0f,
	.taps = { {
	    90,     94,      4,   -240,   -570,   -776,   -563,    309,
	  1861,   3808,   5618,   6710,   6710,   5618,   3808,   1861,
	   309,   -563,   -776,   -570,   -240,      4,     94,     90,
	} },
};

// IFIR prototype filter: fs=614400, pass=100000, stop=207200, decim=2, fout=307200
static constexpr fir_taps_real<16> taps_200k_decim_1 = {
	.pass_frequency_normalized = 100000.0f / 614400.0f,
	.stop_frequency_normalized = 207200.0f / 614400.0f,
	.taps = { {
		  -132,   -256,    545,    834,  -1507,  -2401,   4666,  14583,
		 14583,   4666,  -2401,  -1507,    834,    545,   -256,   -132,
	} },
};

TPMSProcessor::TPMSProcessor() {
	decim_0.configure(taps_200k_decim_0.taps, 33554432);
	decim_1.configure(taps_200k_decim_1.taps, 131072);
}

void TPMSProcessor::execute(const buffer_c8_t& buffer) {
	/* 2.4576MHz, 2048 samples */

	std::array<complex16_t, 512> dst;
	const buffer_c16_t dst_buffer {
		dst.data(),
		dst.size()
	};

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto decimator_out = decim_1_out;

	/* 76.8kHz, 64 samples */
	feed_channel_stats(decimator_out);
	/* No spectrum display while FSK decoding.
	feed_channel_spectrum(
		channel,
		decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized,
		decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized
	);
	*/

	for(size_t i=0; i<decimator_out.count; i++) {
		if( mf.execute_once(decimator_out.p[i]) ) {
			clock_recovery(mf.get_output());
		}
	}

	i2s::i2s0::tx_mute();
}

void TPMSProcessor::consume_symbol(
	const float raw_symbol
) {
	const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
	packet_builder.execute(sliced_symbol);
}

void TPMSProcessor::payload_handler(
	const baseband::Packet& packet
) {
	const TPMSPacketMessage message { packet };
	shared_memory.application_queue.push(message);
}
