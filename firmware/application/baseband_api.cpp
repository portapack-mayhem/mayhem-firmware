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

#include "baseband_api.hpp"

#include "audio.hpp"
#include "dsp_iir_config.hpp"

#include "portapack_shared_memory.hpp"

#include "core_control.hpp"

namespace baseband {

static void send_message(const Message* const message) {
	// If message is only sent by this function via one thread, no need to check if
	// another message is present before setting new message.
	shared_memory.baseband_message = message;
	creg::m0apptxevent::assert();
	while(shared_memory.baseband_message);
}

void AMConfig::apply() const {
	const AMConfigureMessage message {
		taps_6k0_decim_0,
		taps_6k0_decim_1,
		taps_6k0_decim_2,
		channel,
		modulation,
		audio_12k_hpf_300hz_config
	};
	send_message(&message);
	audio::set_rate(audio::Rate::Hz_12000);
}

void NBFMConfig::apply() const {
	const NBFMConfigureMessage message {
		decim_0,
		decim_1,
		channel,
		2,
		deviation,
		audio_24k_hpf_300hz_config,
		audio_24k_deemph_300_6_config
	};
	send_message(&message);
	audio::set_rate(audio::Rate::Hz_24000);
}

void WFMConfig::apply() const {
	const WFMConfigureMessage message {
		taps_200k_wfm_decim_0,
		taps_200k_wfm_decim_1,
		taps_64_lp_156_198,
		75000,
		audio_48k_hpf_30hz_config,
		audio_48k_deemph_2122_6_config
	};
	send_message(&message);
	audio::set_rate(audio::Rate::Hz_48000);
}

void set_ccir_data(	const uint32_t samples_per_tone, const uint16_t tone_count) {
	const CCIRConfigureMessage message {
		samples_per_tone,
		tone_count
	};
	send_message(&message);
}

void set_afsk_data(const uint32_t afsk_samples_per_bit, const uint32_t afsk_phase_inc_mark, const uint32_t afsk_phase_inc_space,
					const uint8_t afsk_repeat, const uint32_t afsk_bw, const bool afsk_alt_format) {
	const AFSKConfigureMessage message {
		afsk_samples_per_bit,
		afsk_phase_inc_mark,
		afsk_phase_inc_space,
		afsk_repeat,
		afsk_bw,
		afsk_alt_format
	};
	send_message(&message);
}

void set_audiotx_data(const uint32_t divider, const uint32_t bw, const bool ctcss_enabled, const uint32_t ctcss_phase_inc) {
	const AudioTXConfigMessage message {
		divider,
		bw,
		ctcss_phase_inc,
		ctcss_enabled
	};
	send_message(&message);
}

void set_fifo_data(const int8_t * data) {
	const FIFODataMessage message {
		data
	};
	send_message(&message);
}

void set_pwmrssi(int32_t avg, bool enabled) {
	const PWMRSSIConfigureMessage message {
		enabled,
		1000,	// 1kHz
		avg
	};
	send_message(&message);	
}

void set_ook_data(const uint32_t stream_length, const uint32_t samples_per_bit, const uint8_t repeat,
					const uint32_t pause_symbols) {
	const OOKConfigureMessage message {
		stream_length,
		samples_per_bit,
		repeat,
		pause_symbols
	};
	send_message(&message);
}

void set_pocsag() {
	const POCSAGConfigureMessage message {
		1200
	};
	send_message(&message);
}

void set_adsb() {
	const ADSBConfigureMessage message {
		1
	};
	send_message(&message);
}

void set_rds_data(const uint16_t message_length) {
	const RDSConfigureMessage message {
		message_length
	};
	send_message(&message);
}

void set_dtmf_data(const uint32_t bw, const uint32_t tone_length, const uint32_t pause_length) {
	const DTMFTXConfigMessage message {
		bw,
		tone_length,
		pause_length
	};
	send_message(&message);
}

static bool baseband_image_running = false;

void run_image(const portapack::spi_flash::image_tag_t image_tag) {
	if( baseband_image_running ) {
		chDbgPanic("BBRunning");
	}

	creg::m4txevent::clear();

	m4_init(image_tag, portapack::memory::map::m4_code);
	baseband_image_running = true;

	creg::m4txevent::enable();
}

void shutdown() {
	if( !baseband_image_running ) {
		return;
	}

	creg::m4txevent::disable();

	ShutdownMessage message;
	send_message(&message);

	shared_memory.application_queue.reset();
	
	baseband_image_running = false;
}

void spectrum_streaming_start() {
	SpectrumStreamingConfigMessage message {
		SpectrumStreamingConfigMessage::Mode::Running
	};
	send_message(&message);
}

void spectrum_streaming_stop() {
	SpectrumStreamingConfigMessage message {
		SpectrumStreamingConfigMessage::Mode::Stopped
	};
	send_message(&message);
}

void capture_start(CaptureConfig* const config) {
	CaptureConfigMessage message { config };
	send_message(&message);
}

void capture_stop() {
	CaptureConfigMessage message { nullptr };
	send_message(&message);
}

} /* namespace baseband */
