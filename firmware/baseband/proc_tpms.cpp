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

#include "dsp_fir_taps.hpp"

TPMSProcessor::TPMSProcessor() {
	decim_0.configure(taps_200k_decim_0.taps, 33554432);
	decim_1.configure(taps_200k_decim_1.taps, 131072);
}

void TPMSProcessor::execute(const buffer_c8_t& buffer) {
	/* 2.4576MHz, 2048 samples */

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto decimator_out = decim_1_out;

	/* 307.2kHz, 256 samples */
	feed_channel_stats(decimator_out);

	for(size_t i=0; i<decimator_out.count; i++) {
		if( mf.execute_once(decimator_out.p[i]) ) {
			clock_recovery_fsk_19k2(mf.get_output());
		}
	}

	for(size_t i=0; i<decim_1_out.count; i+=channel_decimation) {
		const auto sliced = ook_slicer_5sps(decim_1_out.p[i]);
		slicer_history = (slicer_history << 1) | sliced;

		clock_recovery_ook_8k192(slicer_history, [this](const bool symbol) {
			this->packet_builder_schrader.execute(symbol);
		});
		clock_recovery_ook_8k4(slicer_history, [this](const bool symbol) {
			this->packet_builder_ook_gmc.execute(symbol);
		});
	}
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
	const TPMSPacketMessage message { tpms::SignalType::FLM, packet };
	shared_memory.application_queue.push(message);
}
