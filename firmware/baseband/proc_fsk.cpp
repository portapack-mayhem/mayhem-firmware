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
	// TODO: Matched filter characteristics are hard-coded for the moment. YUCK!
	clock_recovery.configure(sampling_rate / 4, new_configuration.symbol_rate);
	packet_builder.configure(
		{ new_configuration.access_code, new_configuration.access_code_length, new_configuration.access_code_tolerance },
		{ new_configuration.unstuffing_pattern, new_configuration.unstuffing_length },
		new_configuration.packet_length
	);
}

void FSKProcessor::execute(buffer_c8_t buffer) {
	/* 2.4576MHz, 2048 samples */

	auto decimator_out = decimator.execute(buffer);

	/* 153.6kHz, 128 samples */

	const buffer_c16_t work_baseband_buffer {
		(complex16_t*)decimator_out.p,
		decimator_out.count
	};

	/* 153.6kHz complex<int16_t>[128]
	 * -> FIR filter, <?kHz (?fs) pass, gain 1.0
	 * -> 76.8kHz int16_t[64] */
	auto channel = channel_filter.execute(decimator_out, work_baseband_buffer);

	/* 76.8kHz, 64 samples */
	feed_channel_stats(channel);
	feed_channel_spectrum(
		channel,
		decimator_out.sampling_rate * channel_filter_taps.pass_frequency_normalized,
		decimator_out.sampling_rate * channel_filter_taps.stop_frequency_normalized
	);

	// 76.8k

	// TODO: Factor out this hidden decimation magic.
	for(size_t i=0; i<channel.count; i+=4) {
		// TODO: No idea why implicit cast int16_t->float is not allowed.
		const std::complex<float> sample {
			static_cast<float>(channel.p[i].real()),
			static_cast<float>(channel.p[i].imag())
		};
		mf_0.execute_once(sample);
		if( mf_1.execute_once(sample) ) {
			const auto value_0 = mf_0.get_output();
			const float mag_0 = std::sqrt(value_0.real() * value_0.real() + value_0.imag() * value_0.imag());
			const auto value_1 = mf_1.get_output();
			const float mag_1 = std::sqrt(value_1.real() * value_1.real() + value_1.imag() * value_1.imag());
			const float diff = mag_1 - mag_0;
			clock_recovery(diff);
		}
	}

	i2s::i2s0::tx_mute();
}

void FSKProcessor::consume_symbol(
	const float raw_symbol
) {
	const auto payload_handler_fn = [this](
		const std::bitset<256>& payload,
		const size_t bits_received
	) {
		this->payload_handler(payload, bits_received);
	};

	const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;
	const auto decoded_symbol = nrzi_decode(sliced_symbol);

	packet_builder.execute(
		decoded_symbol,
		payload_handler_fn
	);
}

void FSKProcessor::payload_handler(
	const std::bitset<256>& payload,
	const size_t bits_received
) {
	FSKPacketMessage message;
	message.packet.payload = payload;
	message.packet.bits_received = bits_received;
	shared_memory.application_queue.push(message);
}
