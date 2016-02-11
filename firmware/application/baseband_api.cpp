/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

namespace baseband {

void AMConfig::apply() const {
	const AMConfigureMessage message {
		taps_6k0_decim_0,
		taps_6k0_decim_1,
		taps_6k0_decim_2,
		channel,
		modulation,
		audio_12k_hpf_300hz_config
	};
	shared_memory.baseband_queue.push(message);
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
	shared_memory.baseband_queue.push(message);
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
	shared_memory.baseband_queue.push(message);
	audio::set_rate(audio::Rate::Hz_48000);
}

void start(BasebandConfiguration configuration) {
	BasebandConfigurationMessage message { configuration };
	shared_memory.baseband_queue.push(message);
}

void stop() {
	shared_memory.baseband_queue.push_and_wait(
		BasebandConfigurationMessage {
			.configuration = { },
		}
	);
}

void shutdown() {
	ShutdownMessage shutdown_message;
	shared_memory.baseband_queue.push(shutdown_message);
}

void spectrum_streaming_start() {
	shared_memory.baseband_queue.push_and_wait(
		SpectrumStreamingConfigMessage {
			SpectrumStreamingConfigMessage::Mode::Running
		}
	);
}

void spectrum_streaming_stop() {
	shared_memory.baseband_queue.push_and_wait(
		SpectrumStreamingConfigMessage {
			SpectrumStreamingConfigMessage::Mode::Stopped
		}
	);
}

} /* namespace baseband */
