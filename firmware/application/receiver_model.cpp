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

#include "receiver_model.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "audio.hpp"

#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"

namespace {

struct AMConfig {
	const fir_taps_complex<64> channel;
	const AMConfigureMessage::Modulation modulation;

	void apply() const;
};

struct NBFMConfig {
	const fir_taps_real<24> decim_0;
	const fir_taps_real<32> decim_1;
	const fir_taps_real<32> channel;
	const size_t deviation;

	void apply() const;
};

struct WFMConfig {
	void apply() const;
};

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

static constexpr std::array<AMConfig, 3> am_configs { {
	{ taps_6k0_dsb_channel, AMConfigureMessage::Modulation::DSB },
	{ taps_2k8_usb_channel, AMConfigureMessage::Modulation::SSB },
	{ taps_2k8_lsb_channel, AMConfigureMessage::Modulation::SSB },	
} };

static constexpr std::array<NBFMConfig, 3> nbfm_configs { {
	{ taps_4k25_decim_0, taps_4k25_decim_1, taps_4k25_channel, 2500 },
	{ taps_11k0_decim_0, taps_11k0_decim_1, taps_11k0_channel, 2500 },
	{ taps_16k0_decim_0, taps_16k0_decim_1, taps_16k0_channel, 5000 },
} };

static constexpr std::array<WFMConfig, 1> wfm_configs { {
	{ },
} };

} /* namespace */

rf::Frequency ReceiverModel::tuning_frequency() const {
	return persistent_memory::tuned_frequency();
}

void ReceiverModel::set_tuning_frequency(rf::Frequency f) {
	persistent_memory::set_tuned_frequency(f);
	update_tuning_frequency();
}

rf::Frequency ReceiverModel::frequency_step() const {
	return frequency_step_;
}

void ReceiverModel::set_frequency_step(rf::Frequency f) {
	frequency_step_ = f;
}

int32_t ReceiverModel::reference_ppm_correction() const {
	return persistent_memory::correction_ppb() / 1000;
}

void ReceiverModel::set_reference_ppm_correction(int32_t v) {
	persistent_memory::set_correction_ppb(v * 1000);
	clock_manager.set_reference_ppb(v * 1000);
}

bool ReceiverModel::antenna_bias() const {
	return antenna_bias_;
}

void ReceiverModel::set_antenna_bias(bool enabled) {
	antenna_bias_ = enabled;
	update_antenna_bias();
}

bool ReceiverModel::rf_amp() const {
	return rf_amp_;
}

void ReceiverModel::set_rf_amp(bool enabled) {
	rf_amp_ = enabled;
	update_rf_amp();
}

int32_t ReceiverModel::lna() const {
	return lna_gain_db_;
}

void ReceiverModel::set_lna(int32_t v_db) {
	lna_gain_db_ = v_db;
	update_lna();
}

uint32_t ReceiverModel::baseband_bandwidth() const {
	return baseband_bandwidth_;
}

void ReceiverModel::set_baseband_bandwidth(uint32_t v) {
	baseband_bandwidth_ = v;
	update_baseband_bandwidth();
}

int32_t ReceiverModel::vga() const {
	return vga_gain_db_;
}

void ReceiverModel::set_vga(int32_t v_db) {
	vga_gain_db_ = v_db;
	update_vga();
}

uint32_t ReceiverModel::sampling_rate() const {
	return baseband_configuration.sampling_rate;
}

uint32_t ReceiverModel::modulation() const {
	return baseband_configuration.mode;
}

volume_t ReceiverModel::headphone_volume() const {
	return headphone_volume_;
}

void ReceiverModel::set_headphone_volume(volume_t v) {
	headphone_volume_ = v;
	update_headphone_volume();
}

uint32_t ReceiverModel::baseband_oversampling() const {
	// TODO: Rename decimation_factor.
	return baseband_configuration.decimation_factor;
}

void ReceiverModel::enable() {
	enabled_ = true;
	radio::set_direction(rf::Direction::Receive);
	update_tuning_frequency();
	update_antenna_bias();
	update_rf_amp();
	update_lna();
	update_vga();
	update_baseband_bandwidth();
	update_baseband_configuration();
	update_modulation_configuration();
	update_headphone_volume();
}

void ReceiverModel::baseband_disable() {
	shared_memory.baseband_queue.push_and_wait(
		BasebandConfigurationMessage {
			.configuration = { },
		}
	);
}

void ReceiverModel::disable() {
	enabled_ = false;
	update_antenna_bias();
	baseband_disable();

	// TODO: Responsibility for enabling/disabling the radio is muddy.
	// Some happens in ReceiverModel, some inside radio namespace.
	radio::disable();
}

int32_t ReceiverModel::tuning_offset() {
	if( (baseband_configuration.mode == 4) ||
		(baseband_configuration.mode == 6) ) {
		return 0;
	} else {
		return -(sampling_rate() / 4);
	}
}

void ReceiverModel::update_tuning_frequency() {
	radio::set_tuning_frequency(persistent_memory::tuned_frequency() + tuning_offset());
}

void ReceiverModel::update_antenna_bias() {
	radio::set_antenna_bias(antenna_bias_ && enabled_);
}

void ReceiverModel::update_rf_amp() {
	radio::set_rf_amp(rf_amp_);
}

void ReceiverModel::update_lna() {
	radio::set_lna_gain(lna_gain_db_);
}

void ReceiverModel::update_baseband_bandwidth() {
	radio::set_baseband_filter_bandwidth(baseband_bandwidth_);
}

void ReceiverModel::update_vga() {
	radio::set_vga_gain(vga_gain_db_);
}

void ReceiverModel::set_baseband_configuration(const BasebandConfiguration config) {
	baseband_configuration = config;
	update_baseband_configuration();
}

void ReceiverModel::set_am_configuration(const size_t n) {
	if( n < am_configs.size() ) {
		am_config_index = n;
		update_modulation_configuration();
	}
}

void ReceiverModel::set_nbfm_configuration(const size_t n) {
	if( n < nbfm_configs.size() ) {
		nbfm_config_index = n;
		update_modulation_configuration();
	}
}

void ReceiverModel::set_wfm_configuration(const size_t n) {
	if( n < wfm_configs.size() ) {
		wfm_config_index = n;
		update_modulation_configuration();
	}
}

void ReceiverModel::update_baseband_configuration() {
	// TODO: Move more low-level radio control stuff to M4. It'll enable tighter
	// synchronization for things like wideband (sweeping) spectrum analysis, and
	// protocols that need quick RX/TX turn-around.

	// Disabling baseband while changing sampling rates seems like a good idea...
	baseband_disable();

	clock_manager.set_sampling_frequency(sampling_rate() * baseband_oversampling());
	update_tuning_frequency();
	radio::set_baseband_decimation_by(baseband_oversampling());

	BasebandConfigurationMessage message { baseband_configuration };
	shared_memory.baseband_queue.push(message);
}

void ReceiverModel::update_headphone_volume() {
	// TODO: Manipulating audio codec here, and in ui_receiver.cpp. Good to do
	// both?

	audio::headphone::set_volume(headphone_volume_);
}

void ReceiverModel::update_modulation_configuration() {
	switch(static_cast<Mode>(modulation())) {
	default:
	case Mode::AMAudio:
		update_am_configuration();
		break;

	case Mode::NarrowbandFMAudio:
		update_nbfm_configuration();
		break;

	case Mode::WidebandFMAudio:
		update_wfm_configuration();
		break;

	case Mode::SpectrumAnalysis:
		break;
	}
}

size_t ReceiverModel::am_configuration() const {
	return am_config_index;
}

void ReceiverModel::update_am_configuration() {
	am_configs[am_config_index].apply();
}

size_t ReceiverModel::nbfm_configuration() const {
	return nbfm_config_index;
}

void ReceiverModel::update_nbfm_configuration() {
	nbfm_configs[nbfm_config_index].apply();
}

size_t ReceiverModel::wfm_configuration() const {
	return wfm_config_index;
}

void ReceiverModel::update_wfm_configuration() {
	wfm_configs[wfm_config_index].apply();
}
