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
		configure_nbfm();
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

void AnalogAudioModel::configure_nbfm() {
	const NBFMConfigureMessage message {
		taps_4k25_decim_0,
		taps_4k25_decim_1,
		taps_4k25_channel,
		2500,
	};
	shared_memory.baseband_queue.push(message);
	clock_manager.set_base_audio_clock_divider(3);
}

void AnalogAudioModel::configure_wfm() {
	const WFMConfigureMessage message {
		taps_200k_wfm_decim_0,
		taps_200k_wfm_decim_1,
		taps_64_lp_156_198,
		75000,
	};
	shared_memory.baseband_queue.push(message);
	clock_manager.set_base_audio_clock_divider(1);
}

void AnalogAudioModel::configure_am() {
	const AMConfigureMessage message {
		taps_6k0_decim_0,
		taps_6k0_decim_1,
		taps_6k0_channel,
	};
	shared_memory.baseband_queue.push(message);
	clock_manager.set_base_audio_clock_divider(6);
}
