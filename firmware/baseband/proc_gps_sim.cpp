/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
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

#include "proc_gps_sim.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include "utility.hpp"

ReplayProcessor::ReplayProcessor() {
	channel_filter_pass_f = taps_200k_decim_1.pass_frequency_normalized * 1000000;	// 162760.416666667
	channel_filter_stop_f = taps_200k_decim_1.stop_frequency_normalized * 1000000;	// 337239.583333333
	
	spectrum_samples = 0;

	channel_spectrum.set_decimation_factor(1);
	
	configured = false;
}

void ReplayProcessor::execute(const buffer_c8_t& buffer) {
	/* 4MHz, 2048 samples */
	
	if (!configured) return;
	
	// File data is in C16 format, we need C8
	// File samplerate is 500kHz, we're at 4MHz
	// iq_buffer can only be 512 C16 samples (RAM limitation)
	// To fill up the 2048-sample C8 buffer, we need:
	// 2048 samples * 2 bytes per sample = 4096 bytes
	// Since we're oversampling by 4M/500k = 8, we only need 2048/8 = 256 samples from the file and duplicate them 8 times each
	// So 256 * 4 bytes per sample (C16) = 1024 bytes from the file
	if( stream ) {                             //sizeof(*buffer.p) = sizeof(C8) = 2*int8 = 2 bytes //buffer.count = 2048
		const size_t bytes_to_read = sizeof(*buffer.p) * 1 * (buffer.count );	// *2 (C16), /8 (oversampling) should be == 1024
		bytes_read += stream->read(iq_buffer.p, bytes_to_read);
	}
	
	// Fill and "stretch"
	for (size_t i = 0; i < buffer.count; i++) {
		/*if (i & 3) {
			buffer.p[i] = buffer.p[i - 1];
		} else {
			auto re_out = iq_buffer.p[i >> 3].real() ;
			auto im_out = iq_buffer.p[i >> 3].imag() ;
			buffer.p[i] = { (int8_t)re_out, (int8_t)im_out };
		}*/
                /*
                if (i % 8 != 0) {
			buffer.p[i] = buffer.p[i - 1];
		} else {
			auto re_out = iq_buffer.p[i/8].real() ;
			auto im_out = iq_buffer.p[i/8].imag() ;
			buffer.p[i] = { (int8_t)re_out, (int8_t)im_out };
		}*/
                
                auto re_out = iq_buffer.p[i].real() ;
	        auto im_out = iq_buffer.p[i].imag() ;
		buffer.p[i] = { (int8_t)re_out, (int8_t)im_out };
	}
	
	spectrum_samples += buffer.count;
	if( spectrum_samples >= spectrum_interval_samples ) {
		spectrum_samples -= spectrum_interval_samples;
		//channel_spectrum.feed(iq_buffer, channel_filter_pass_f, channel_filter_stop_f);
		
		txprogress_message.progress = bytes_read;	// Inform UI about progress
		txprogress_message.done = false;
		shared_memory.application_queue.push(txprogress_message);
	}
}

void ReplayProcessor::on_message(const Message* const message) {
	switch(message->id) {
	case Message::ID::UpdateSpectrum:
	case Message::ID::SpectrumStreamingConfig:
		channel_spectrum.on_message(message);
		break;

	case Message::ID::SamplerateConfig:
		samplerate_config(*reinterpret_cast<const SamplerateConfigMessage*>(message));
		break;
	
	case Message::ID::ReplayConfig:
		configured = false;
		bytes_read = 0;
		replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
		break;
		
	// App has prefilled the buffers, we're ready to go now
	case Message::ID::FIFOData:
		configured = true;
		break;

	default:
		break;
	}
}

void ReplayProcessor::samplerate_config(const SamplerateConfigMessage& message) {
	baseband_fs = message.sample_rate;
	baseband_thread.set_sampling_rate(baseband_fs);
	spectrum_interval_samples = baseband_fs / spectrum_rate_hz;
}

void ReplayProcessor::replay_config(const ReplayConfigMessage& message) {
	if( message.config ) {
		
		stream = std::make_unique<StreamOutput>(message.config);
		
		// Tell application that the buffers and FIFO pointers are ready, prefill
		shared_memory.application_queue.push(sig_message);
	} else {
		stream.reset();
	}
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<ReplayProcessor>() };
	event_dispatcher.run();
	return 0;
}
