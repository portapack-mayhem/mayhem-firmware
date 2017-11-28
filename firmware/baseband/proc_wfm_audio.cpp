/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "audio_output.hpp"

#include "event_m4.hpp"

#include <cstdint>

void WidebandFMAudio::execute(const buffer_c8_t& buffer) {
	if( !configured ) {
		return;
	}
	
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto channel = decim_1.execute(decim_0_out, dst_buffer);

	// TODO: Feed channel_stats post-decimation data?
	feed_channel_stats(channel);

	spectrum_samples += channel.count;
	if( spectrum_samples >= spectrum_interval_samples ) {
		spectrum_samples -= spectrum_interval_samples;
		channel_spectrum.feed(channel, channel_filter_pass_f, channel_filter_stop_f);
	}

	/* 384kHz complex<int16_t>[256]
	 * -> FM demodulation
	 * -> 384kHz int16_t[256] */
	/* TODO: To improve adjacent channel rejection, implement complex channel filter:
	 *		pass < +/- 100kHz, stop > +/- 200kHz
	 */

	auto audio_oversampled = demod.execute(channel, work_audio_buffer);

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
	audio_output.write(audio);
	
}

void WidebandFMAudio::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;

	case Message::ID::WFMConfigure:
		configure(*reinterpret_cast<const WFMConfigureMessage*>(message));
		break;

	case Message::ID::CaptureConfig:
		capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
		break;
		
	default:
		break;
	}
}

void WidebandFMAudio::configure(const WFMConfigureMessage& message) {
	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

	constexpr size_t demod_input_fs = decim_1_output_fs;

	spectrum_interval_samples = decim_1_output_fs / spectrum_rate_hz;
	spectrum_samples = 0;

	decim_0.configure(message.decim_0_filter.taps, 33554432);
	decim_1.configure(message.decim_1_filter.taps, 131072);
	channel_filter_pass_f = message.decim_1_filter.pass_frequency_normalized * decim_1_input_fs;
	channel_filter_stop_f = message.decim_1_filter.stop_frequency_normalized * decim_1_input_fs;
	demod.configure(demod_input_fs, message.deviation);
	audio_filter.configure(message.audio_filter.taps);
	audio_output.configure(message.audio_hpf_config, message.audio_deemph_config);

	channel_spectrum.set_decimation_factor(1);

	configured = true;
}

void WidebandFMAudio::capture_config(const CaptureConfigMessage& message) {
	if( message.config ) {
		audio_output.set_stream(std::make_unique<StreamInput>(message.config));
	} else {
		audio_output.set_stream(nullptr);
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<WidebandFMAudio>() };
	event_dispatcher.run();
	return 0;
}
