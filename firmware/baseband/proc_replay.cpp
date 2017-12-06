/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_replay.hpp"
#include "sine_table_int8.hpp"

#include "event_m4.hpp"

#include "utility.hpp"

ReplayProcessor::ReplayProcessor() {
	channel_filter_pass_f = taps_200k_decim_1.pass_frequency_normalized * 1000000;	// 162760.416666667
	channel_filter_stop_f = taps_200k_decim_1.stop_frequency_normalized * 1000000;	// 337239.583333333
	
	spectrum_interval_samples = (baseband_fs / 8) / spectrum_rate_hz;
	spectrum_samples = 0;

	channel_spectrum.set_decimation_factor(1);
}

void ReplayProcessor::execute(const buffer_c8_t& buffer) {
	/* 4MHz, 2048 samples */

	size_t pos = 0;
	
	// File data is in C16 format, we need C8
	// File samplerate is 500kHz, we're at 4MHz
	// iq_buffer can only be 512 C16 samples (RAM limitation)
	// To fill up the 2048-sample C8 buffer, we need:
	// 2048 samples * 2 bytes per sample = 4096 bytes
	// Since we're oversampling by 4M/500k = 8, we only need 2048/8 = 256 samples from the file and duplicate them 8 times each
	// So 256 * 4 bytes per sample (C16) = 1024 bytes from the file
	if( stream ) {
		const size_t bytes_to_read = sizeof(*buffer.p) * 2 * (buffer.count / 8);	// *2 (C16), /8 (oversampling) should be == 1024
		const auto result = stream->read(iq_buffer.p, bytes_to_read);
	}

	//feed_channel_stats(channel);
	
	// Zero-stuff
	for (size_t i = 0; i < buffer.count; i++) {
		
		// DEBUG: This works. Transmits a 1kHz tone
		/*sample = (sine_table_i8[(tone_phase & 0xFF000000) >> 24]);
		tone_phase += (1000 * ((1ULL << 32) / baseband_fs));
		// Do FM
		delta = sample * 30000 * (0xFFFFFFULL / baseband_fs);
		phase += delta;
		sphase = phase + (64 << 24);
		iq_buffer.p[i >> 3] = { (int16_t)(sine_table_i8[(sphase & 0xFF000000) >> 24]) << 8, (int16_t)(sine_table_i8[(phase & 0xFF000000) >> 24]) << 8 };
		*/
		
		/*if (i & 3)
			buffer.p[i] = buffer.p[i - 1];
		else {*/
			auto re_out = iq_buffer.p[i >> 3].real() >> 8;
			auto im_out = iq_buffer.p[i >> 3].imag() >> 8;
			buffer.p[i] = { re_out, im_out };
		//}
	}
	
	spectrum_samples += buffer.count;
	if( spectrum_samples >= spectrum_interval_samples ) {
		spectrum_samples -= spectrum_interval_samples;
		channel_spectrum.feed(iq_buffer, channel_filter_pass_f, channel_filter_stop_f);
	}
}

void ReplayProcessor::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;

	case Message::ID::ReplayConfig:
		replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
		break;

	default:
		break;
	}
}

void ReplayProcessor::replay_config(const ReplayConfigMessage& message) {
	if( message.config ) {
		stream = std::make_unique<StreamOutput>(message.config);
	} else {
		stream.reset();
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ReplayProcessor>() };
	event_dispatcher.run();
	return 0;
}
