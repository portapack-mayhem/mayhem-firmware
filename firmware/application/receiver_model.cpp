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
#include "radio.hpp"
#include "wm8731.hpp"

// TODO: Nasty. Put this in an #include somewhere, or a shared system state
// object?

extern wolfson::wm8731::WM8731 audio_codec;

rf::Frequency ReceiverModel::tuning_frequency() const {
	return tuning_frequency_;
}

void ReceiverModel::set_tuning_frequency(rf::Frequency f) {
	tuning_frequency_ = f;
	update_tuning_frequency();
}

rf::Frequency ReceiverModel::frequency_step() const {
	return frequency_step_;
}

void ReceiverModel::set_frequency_step(rf::Frequency f) {
	frequency_step_ = f;
}

int32_t ReceiverModel::reference_ppm_correction() const {
	return shared_memory.correction_ppm;
}

void ReceiverModel::set_reference_ppm_correction(int32_t v) {
	shared_memory.correction_ppm = v;
	update_tuning_frequency();
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

void ReceiverModel::set_sampling_rate(uint32_t hz) {
	baseband_configuration.sampling_rate = hz;
	update_baseband_configuration();
}

uint32_t ReceiverModel::modulation() const {
	return baseband_configuration.mode;
}

void ReceiverModel::set_modulation(uint32_t v) {
	baseband_configuration.mode = v;
	update_modulation();
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

void ReceiverModel::set_baseband_oversampling(uint32_t v) {
	baseband_configuration.decimation_factor = v;
	update_baseband_configuration();
}

void ReceiverModel::enable() {
	radio::set_direction(rf::Direction::Receive);
	update_tuning_frequency();
	update_rf_amp();
	update_lna();
	update_vga();
	update_baseband_bandwidth();
	update_baseband_configuration();
	radio::streaming_enable();

	update_headphone_volume();
}

void ReceiverModel::disable() {
	/* TODO: This is a dumb hack to stop baseband from working so hard. */
	BasebandConfigurationMessage message {
		.configuration = {
			.mode = -1,
			.sampling_rate = 0,
			.decimation_factor = 1,
		}
	};
	shared_memory.baseband_queue.push(&message);
	while( !message.is_free() );

	radio::disable();
}

int32_t ReceiverModel::tuning_offset() {
	return -(sampling_rate() / 4);
}

void ReceiverModel::update_tuning_frequency() {
	radio::set_tuning_frequency(tuning_frequency_ + tuning_offset());
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

void ReceiverModel::update_modulation() {
	update_baseband_configuration();
}

void ReceiverModel::update_baseband_configuration() {
	clock_manager.set_sampling_frequency(sampling_rate() * baseband_oversampling());
	update_tuning_frequency();
	radio::set_baseband_decimation_by(baseband_oversampling());

	BasebandConfigurationMessage message { baseband_configuration };
	shared_memory.baseband_queue.push(&message);

	// Block until message is consumed, since we allocated it on the stack.
	while( !message.is_free() );

	if( baseband_configuration.mode == 3 ) {
		update_fsk_configuration();
	}
}

void ReceiverModel::update_headphone_volume() {
	// TODO: Manipulating audio codec here, and in ui_receiver.cpp. Good to do
	// both?

	audio_codec.set_headphone_volume(headphone_volume_);
}

static constexpr FSKConfiguration fsk_configuration_ais = {
	.symbol_rate = 9600,
	.access_code = 0b01010101010101010101111110,
	.access_code_length = 26,
	.access_code_tolerance = 1,
	.packet_length = 256,
};

static constexpr FSKConfiguration fsk_configuration_tpms_a = {
	.symbol_rate = 19200,
	.access_code = 0b0101010101010101010101010110,
	.access_code_length = 28,
	.access_code_tolerance = 1,
	.packet_length = 160,
};

void ReceiverModel::update_fsk_configuration() {
	FSKConfigurationMessage message { fsk_configuration_ais };
	shared_memory.baseband_queue.push(&message);

	// Block until message is consumed, since we allocated it on the stack.
	while( !message.is_free() );
}
