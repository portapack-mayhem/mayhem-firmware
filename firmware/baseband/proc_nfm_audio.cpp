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

#include "proc_nfm_audio.hpp"

#include "event_m4.hpp"

#include <cstdint>
#include <cstddef>

void NarrowbandFMAudio::execute(const buffer_c8_t& buffer) {
	if( !configured ) {
		return;
	}
	
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	feed_channel_stats(channel_out);
	channel_spectrum.feed(channel_out, channel_filter_pass_f, channel_filter_stop_f);

	if ( !pwmrssi_enabled ) {
		auto audio = demod.execute(channel_out, audio_buffer);
		audio_output.write(audio);
	} else {
		for (c = 0; c < 32; c++) {
			if (synth_acc < pwmrssi_avg)
				pwmrssi_audio_buffer.p[c] = 32767;
			else
				pwmrssi_audio_buffer.p[c] = -32768;
			
			if (synth_acc < synth_div)		// 24kHz / 30 = 800Hz
				synth_acc++;
			else
				synth_acc = 0;
		}

		audio_output.write(pwmrssi_audio_buffer);
	}
}

void NarrowbandFMAudio::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;

	case Message::ID::NBFMConfigure:
		configure(*reinterpret_cast<const NBFMConfigureMessage*>(message));
		break;

	case Message::ID::CaptureConfig:
		capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
		break;
	
	case Message::ID::PWMRSSIConfigure:
		pwmrssi_config(*reinterpret_cast<const PWMRSSIConfigureMessage*>(message));
		break;
		
	default:
		break;
	}
}

void NarrowbandFMAudio::configure(const NBFMConfigureMessage& message) {
	constexpr size_t decim_0_input_fs = baseband_fs;
	constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

	constexpr size_t decim_1_input_fs = decim_0_output_fs;
	constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

	constexpr size_t channel_filter_input_fs = decim_1_output_fs;
	const size_t channel_filter_output_fs = channel_filter_input_fs / message.channel_decimation;

	const size_t demod_input_fs = channel_filter_output_fs;

	decim_0.configure(message.decim_0_filter.taps, 33554432);
	decim_1.configure(message.decim_1_filter.taps, 131072);
	channel_filter.configure(message.channel_filter.taps, message.channel_decimation);
	demod.configure(demod_input_fs, message.deviation);
	channel_filter_pass_f = message.channel_filter.pass_frequency_normalized * channel_filter_input_fs;
	channel_filter_stop_f = message.channel_filter.stop_frequency_normalized * channel_filter_input_fs;
	channel_spectrum.set_decimation_factor(std::floor(channel_filter_output_fs / (channel_filter_pass_f + channel_filter_stop_f)));
	audio_output.configure(message.audio_hpf_config, message.audio_deemph_config);	// , 0.8f

	synth_acc = 0;
	
	configured = true;
}

void NarrowbandFMAudio::pwmrssi_config(const PWMRSSIConfigureMessage& message) {
	pwmrssi_enabled = message.enabled;
	pwmrssi_avg = message.avg / 3;
	synth_div = message.synth_div;
	synth_acc = 0;
}

void NarrowbandFMAudio::capture_config(const CaptureConfigMessage& message) {
	if( message.config ) {
		audio_output.set_stream(std::make_unique<StreamInput>(message.config));
	} else {
		audio_output.set_stream(nullptr);
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<NarrowbandFMAudio>() };
	event_dispatcher.run();
	return 0;
}
