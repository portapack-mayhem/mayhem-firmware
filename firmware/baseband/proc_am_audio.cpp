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

#include "proc_am_audio.hpp"

#include "audio_output.hpp"

#include <array>

void NarrowbandAMAudio::execute(const buffer_c8_t& buffer) {
	if( !configured ) {
		return;
	}

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	// TODO: Feed channel_stats post-decimation data?
	feed_channel_stats(channel_out);
	channel_spectrum.feed(channel_out, channel_filter_pass_f, channel_filter_stop_f);

	auto audio = demod.execute(channel_out, audio_buffer);
	audio_output.write(audio);
}

void NarrowbandAMAudio::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;

	case Message::ID::AMConfigure:
		configure(*reinterpret_cast<const AMConfigureMessage*>(message));
		break;

	default:
		break;
	}
}

void NarrowbandAMAudio::configure(const AMConfigureMessage& message) {
	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	const size_t channel_filter_output_fs = channel_filter_input_fs / channel_filter_decimation_factor;

	decim_0.configure(message.decim_0_filter.taps, 33554432);
	decim_1.configure(message.decim_1_filter.taps, 131072);
	channel_filter.configure(message.channel_filter.taps, channel_filter_decimation_factor);
	channel_filter_pass_f = message.channel_filter.pass_frequency_normalized * channel_filter_input_fs;
	channel_filter_stop_f = message.channel_filter.stop_frequency_normalized * channel_filter_input_fs;
	channel_spectrum.set_decimation_factor(std::floor((channel_filter_output_fs / 2) / ((channel_filter_pass_f + channel_filter_stop_f) / 2)));
	audio_output.configure(message.audio_hpf_config);

	configured = true;
}
