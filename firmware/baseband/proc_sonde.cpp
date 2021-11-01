/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "proc_sonde.hpp"

#include "dsp_fir_taps.hpp"

#include "event_m4.hpp"

#include "audio_output.hpp"

SondeProcessor::SondeProcessor() {

	decim_0.configure(taps_11k0_decim_0.taps, 33554432);
	decim_1.configure(taps_11k0_decim_1.taps, 131072);

	audio_output.configure(false);

	tone_gen.configure(BEEP_BASE_FREQ, 1.0, ToneGen::tone_type::sine, AUDIO_SAMPLE_RATE);
}

void SondeProcessor::execute(const buffer_c8_t& buffer) {
	/* 2.4576MHz, 2048 samples */

	const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
	const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
	const auto decimator_out = decim_1_out;

	/* 38.4kHz, 32 samples */
	feed_channel_stats(decimator_out);

	for (size_t i=0; i<decimator_out.count; i++) {
		if( mf.execute_once(decimator_out.p[i]) ) {
			clock_recovery_fsk_9600(mf.get_output());
			clock_recovery_fsk_4800(mf.get_output());
		}
	}

	if(pitch_rssi_enabled) {
		if(beep_play) {
			// if we let the buffer underrun, for some reason
			// once it starts looping it ignores zero (silence)
			// samples, so we need to keep feeding the buffer
			// and not be able to take advantage of the circular
			// buffer loop:
			//beep_play = false;
			generate_beep();
		}
		
		if(silence_play) {
			//silence_play = false;
			generate_silence();
		}
	}
}

void SondeProcessor::on_message(const Message* const msg) {
	switch(msg->id) {		
		case Message::ID::RequestSignal:
			if ((*reinterpret_cast<const RequestSignalMessage*>(msg)).signal == RequestSignalMessage::Signal::BeepRequest) {
				float rssi_ratio = (float) last_rssi / (float) RSSI_CEILING;
				int beep_duration = 0;

				if(rssi_ratio <= PROPORTIONAL_BEEP_THRES) {
					beep_duration = BEEP_MIN_DURATION;
				}
				else if(rssi_ratio < 1) {
					beep_duration =  (int) rssi_ratio * BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
				}
				else {
					beep_duration =  BEEP_DURATION_RANGE + BEEP_MIN_DURATION;
				}
				
				play_beep();
				chThdSleepMilliseconds(beep_duration);
				stop_beep();
			}		
			break;

		case Message::ID::PitchRSSIConfigure:
			pitch_rssi_config(*reinterpret_cast<const PitchRSSIConfigureMessage*>(msg));
			break;

		default:
			break;
	}
}

void SondeProcessor::play_beep() {
	beep_play = true;
	silence_play = false;
}

void SondeProcessor::stop_beep() {
	beep_play = false;
	silence_play = true;
}

void SondeProcessor::generate_beep() {
	// here we let the samples be created using the ToneGen class:

	for(uint8_t i = 0; i < sizeof(audio_buffer.p); i++) {
		audio_buffer.p[i] = (int16_t) ((tone_gen.process(0) >> 16) & 0x0000FFFF);
	}

	audio_output.write(audio_buffer);
}

void SondeProcessor::generate_silence() {
	for(uint8_t i = 0; i < sizeof(audio_buffer.p); i++) {
		audio_buffer.p[i] = 0;
	}

	audio_output.write(audio_buffer);
}

void SondeProcessor::pitch_rssi_config(const PitchRSSIConfigureMessage& message) {
	pitch_rssi_enabled = message.enabled;

	uint32_t freq = (int) ((float) message.rssi * (float) RSSI_PITCH_WEIGHT + (float) BEEP_BASE_FREQ);

	last_rssi = message.rssi;
	tone_gen.configure(freq, 1.0, ToneGen::tone_type::sine, AUDIO_SAMPLE_RATE);
}

int main() {
	EventDispatcher event_dispatcher { std::make_unique<SondeProcessor>() };
	event_dispatcher.run();

	return 0;
}
