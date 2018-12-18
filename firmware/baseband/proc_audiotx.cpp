/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_audiotx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void AudioTXProcessor::execute(const buffer_c8_t& buffer){
	
	if (!configured) return;
	
	// Zero-order hold (poop)
	for (size_t i = 0; i < buffer.count; i++) {
		resample_acc += resample_inc;
		if (resample_acc >= 0x10000) {
			resample_acc -= 0x10000;
			if (stream) {
				stream->read(&audio_sample, 1);
				bytes_read++;
			}
		}
		
		sample = tone_gen.process(audio_sample - 0x80);
		
		// FM
		delta = sample * fm_delta;
		
		phase += delta;
		sphase = phase + (64 << 24);
		
		re = sine_table_i8[(sphase & 0xFF000000U) >> 24];
		im = sine_table_i8[(phase & 0xFF000000U) >> 24];
		
		buffer.p[i] = { (int8_t)re, (int8_t)im };
	}
	
	progress_samples += buffer.count;
	if (progress_samples >= progress_interval_samples) {
		progress_samples -= progress_interval_samples;
		
		txprogress_message.progress = bytes_read;	// Inform UI about progress
		txprogress_message.done = false;
		shared_memory.application_queue.push(txprogress_message);
	}
}

void AudioTXProcessor::on_message(const Message* const message) {
	switch(message->id) {
		case Message::ID::AudioTXConfig:
			audio_config(*reinterpret_cast<const AudioTXConfigMessage*>(message));
			break;

		case Message::ID::ReplayConfig:
			configured = false;
			bytes_read = 0;
			replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
			break;
		
		case Message::ID::SamplerateConfig:
			samplerate_config(*reinterpret_cast<const SamplerateConfigMessage*>(message));
			break;
		
		case Message::ID::FIFOData:
			configured = true;
			break;
		
		default:
			break;
	}
}

void AudioTXProcessor::audio_config(const AudioTXConfigMessage& message) {
	fm_delta = message.deviation_hz * (0xFFFFFFULL / baseband_fs);
	tone_gen.configure(message.tone_key_delta, message.tone_key_mix_weight);
	progress_interval_samples = message.divider;
	resample_acc = 0;
}

void AudioTXProcessor::replay_config(const ReplayConfigMessage& message) {
	if( message.config ) {
		
		stream = std::make_unique<StreamOutput>(message.config);
		
		// Tell application that the buffers and FIFO pointers are ready, prefill
		shared_memory.application_queue.push(sig_message);
	} else {
		stream.reset();
	}
}

void AudioTXProcessor::samplerate_config(const SamplerateConfigMessage& message) {
	resample_inc = (((uint64_t)message.sample_rate) << 16) / baseband_fs;	// 16.16 fixed point message.sample_rate
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<AudioTXProcessor>() };
	event_dispatcher.run();
	return 0;
}
