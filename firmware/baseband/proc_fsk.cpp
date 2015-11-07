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

#include "proc_fsk.hpp"

#include "ais_baseband.hpp"

#include "portapack_shared_memory.hpp"

#include "i2s.hpp"
using namespace lpc43xx;

FSKProcessor::FSKProcessor(
	MessageHandlerMap& message_handlers
) : message_handlers(message_handlers)
{
	message_handlers.register_handler(Message::ID::FSKConfiguration,
		[this](const Message* const p) {
			auto m = reinterpret_cast<const FSKConfigurationMessage*>(p);
			this->configure(m->configuration);
		}
	);
}

FSKProcessor::~FSKProcessor() {
	message_handlers.unregister_handler(Message::ID::FSKConfiguration);
}

void FSKProcessor::configure(const FSKConfiguration new_configuration) {
	decimator.set_decimation_factor(ChannelDecimator::DecimationFactor::By32);
	mf.configure(baseband::ais::rrc_taps_76k8_4t_p, 4);
	clock_recovery.configure(new_configuration.symbol_rate * 2, new_configuration.symbol_rate, { 0.0555f });
	packet_builder.configure(
		{ new_configuration.access_code, new_configuration.access_code_length, new_configuration.access_code_tolerance },
		{ new_configuration.unstuffing_pattern, new_configuration.unstuffing_length }
	);
}

void FSKProcessor::execute(buffer_c8_t buffer) {
	/* 2.4576MHz, 2048 samples */

	auto decimator_out = decimator.execute(buffer);

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
		// TODO: No idea why implicit cast int16_t->float is not allowed.
		const std::complex<float> sample {
			static_cast<float>(decimator_out.p[i].real()),
			static_cast<float>(decimator_out.p[i].imag())
		};
		if( mf.execute_once(sample) ) {
			clock_recovery(mf.get_output());
		}
	}

	i2s::i2s0::tx_mute();
}

void FSKProcessor::consume_symbol(
	const float raw_symbol
) {
	const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
	const auto decoded_symbol = nrzi_decode(sliced_symbol);

	packet_builder.execute(decoded_symbol);
}

void FSKProcessor::payload_handler(
	const std::bitset<1024>& payload,
	const size_t bits_received
) {
	FSKPacketMessage message;
	message.packet.payload = payload;
	message.packet.bits_received = bits_received;
	shared_memory.application_queue.push(message);
}
