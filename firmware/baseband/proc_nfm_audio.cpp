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
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include <cstdint>
#include <cstddef>

void NarrowbandFMAudio::execute(const buffer_c8_t& buffer) {
	//bool new_state;
	
	if( !configured ) {
		return;
	}
	
	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

	channel_spectrum.feed(decim_1_out, channel_filter_pass_f, channel_filter_stop_f);

	const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

	feed_channel_stats(channel_out);

	if (!pitch_rssi_enabled) {
		// Normal mode, output demodulated audio
		auto audio = demod.execute(channel_out, audio_buffer);
		audio_output.write(audio);
		
		if (ctcss_detect_enabled) {
			/* 24kHz int16_t[16]
			 * -> FIR filter, <300Hz pass, >300Hz stop, gain of 1
			 * -> 12kHz int16_t[8] */
			auto audio_ctcss = ctcss_filter.execute(audio, work_audio_buffer);
			
			// s16 to f32 for hpf
			std::array<float, 8> audio_f;
			for (size_t i = 0; i < audio_ctcss.count; i++) {
				audio_f[i] = audio_ctcss.p[i] * ki;
			}
			
			hpf.execute_in_place(buffer_f32_t {
				audio_f.data(),
				audio_ctcss.count,
				audio_ctcss.sampling_rate
				});
			
			// Zero-crossing detection
			for (size_t c = 0; c < audio_ctcss.count; c++) {
				cur_sample = audio_f[c];
				if (cur_sample * prev_sample < 0.0) {
					z_acc += z_timer;
					z_timer = 0;
					z_count++;
				} else
					z_timer++;
				prev_sample = cur_sample;
			}
			
			if (z_count >= 30) {
				ctcss_message.value = (100 * 12000 / 2 * z_count) / z_acc;
				shared_memory.application_queue.push(ctcss_message);
				z_count = 0;
				z_acc = 0;
			}
		}
	} else {
		// Direction-finding mode; output tone with pitch related to RSSI
		for (size_t c = 0; c < 16; c++) {
			tone_buffer.p[c] = (sine_table_i8[(tone_phase & 0xFF000000U) >> 24]) * 128;
			tone_phase += tone_delta;
		}
		
		audio_output.write(tone_buffer);
		
		/*new_state = audio_output.is_squelched();
		
		if (new_state && !old_state)
			shared_memory.application_queue.push(sig_message);
		
		old_state = new_state;*/	
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
	
	case Message::ID::PitchRSSIConfigure:
		pitch_rssi_config(*reinterpret_cast<const PitchRSSIConfigureMessage*>(message));
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
	channel_spectrum.set_decimation_factor(1.0f);
	audio_output.configure(message.audio_hpf_config, message.audio_deemph_config, (float)message.squelch_level / 100.0);
	
	hpf.configure(audio_24k_hpf_30hz_config);
	ctcss_filter.configure(taps_64_lp_025_025.taps);

	configured = true;
}

void NarrowbandFMAudio::pitch_rssi_config(const PitchRSSIConfigureMessage& message) {
	pitch_rssi_enabled = message.enabled;
	tone_delta = (message.rssi + 1000) * ((1ULL << 32) / 24000);
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
