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

#include "analog_audio_app.hpp"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
using namespace portapack;

#include "dsp_fir_taps.hpp"
#include "dsp_iir_config.hpp"

#include "utility.hpp"

AnalogAudioModel::AnalogAudioModel(ReceiverModel::Mode mode) {
	receiver_model.set_baseband_configuration({
		.mode = toUType(mode),
		.sampling_rate = 3072000,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(1750000);

	switch(mode) {
	case ReceiverModel::Mode::NarrowbandFMAudio:
		configure_nbfm(2);
		break;

	case ReceiverModel::Mode::WidebandFMAudio:
		configure_wfm();
		break;

	case ReceiverModel::Mode::AMAudio:
		configure_am();
		break;
		
	default:
		break;
	}

}

struct NBFMMode {
	const fir_taps_real<24> decim_0;
	const fir_taps_real<32> decim_1;
	const fir_taps_real<32> channel;
	const size_t deviation;
};

static constexpr std::array<NBFMMode, 3> nbfm_mode_configs { {
	{ taps_4k25_decim_0, taps_4k25_decim_1, taps_4k25_channel, 2500 },
	{ taps_11k0_decim_0, taps_11k0_decim_1, taps_11k0_channel, 2500 },
	{ taps_16k0_decim_0, taps_16k0_decim_1, taps_16k0_channel, 5000 },
} };

void AnalogAudioModel::configure_nbfm(const size_t index) {
	const auto config = nbfm_mode_configs[index];
	const NBFMConfigureMessage message {
		config.decim_0,
		config.decim_1,
		config.channel,
		2,
		config.deviation,
		audio_24k_hpf_300hz_config,
		audio_24k_deemph_300_6_config
	};
	shared_memory.baseband_queue.push(message);
	clock_manager.set_base_audio_clock_divider(2);
}

void AnalogAudioModel::configure_wfm() {
	const WFMConfigureMessage message {
		taps_200k_wfm_decim_0,
		taps_200k_wfm_decim_1,
		taps_64_lp_156_198,
		75000,
		audio_48k_hpf_30hz_config,
		audio_48k_deemph_2122_6_config
	};
	shared_memory.baseband_queue.push(message);
	clock_manager.set_base_audio_clock_divider(1);
}

void AnalogAudioModel::configure_am() {
	const AMConfigureMessage message {
		taps_6k0_decim_0,
		taps_6k0_decim_1,
		taps_6k0_decim_2,
		taps_6k0_channel,
		audio_12k_hpf_300hz_config
	};
	shared_memory.baseband_queue.push(message);
	clock_manager.set_base_audio_clock_divider(4);
}
