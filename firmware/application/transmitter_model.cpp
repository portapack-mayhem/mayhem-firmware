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

#include "transmitter_model.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"
#include "portapack.hpp"
using namespace portapack;

rf::Frequency TransmitterModel::tuning_frequency() const {
	return persistent_memory::tuned_frequency();
}

void TransmitterModel::set_tuning_frequency(rf::Frequency f) {
	persistent_memory::set_tuned_frequency(f);
	update_tuning_frequency();
}

bool TransmitterModel::rf_amp() const {
	return rf_amp_;
}

void TransmitterModel::set_rf_amp(bool enabled) {
	rf_amp_ = enabled;
	update_rf_amp();
}

int32_t TransmitterModel::lna() const {
	return lna_gain_db_;
}

void TransmitterModel::set_lna(int32_t v_db) {
	lna_gain_db_ = v_db;
	update_lna();
}

uint32_t TransmitterModel::baseband_bandwidth() const {
	return baseband_bandwidth_;
}

void TransmitterModel::set_baseband_bandwidth(uint32_t v) {
	baseband_bandwidth_ = v;
	update_baseband_bandwidth();
}

int32_t TransmitterModel::vga() const {
	return vga_gain_db_;
}

void TransmitterModel::set_vga(int32_t v_db) {
	vga_gain_db_ = v_db;
	update_vga();
}

uint32_t TransmitterModel::sampling_rate() const {
	return baseband_configuration.sampling_rate;
}

void TransmitterModel::set_sampling_rate(uint32_t hz) {
	baseband_configuration.sampling_rate = hz;
	update_baseband_configuration();
}

mode_type TransmitterModel::modulation() const {
	return baseband_configuration.mode;
}

void TransmitterModel::set_modulation(mode_type v) {
	baseband_configuration.mode = v;
	update_modulation();
}

uint32_t TransmitterModel::baseband_oversampling() const {
	// TODO: Rename decimation_factor.
	return baseband_configuration.decimation_factor;
}

void TransmitterModel::set_baseband_oversampling(uint32_t v) {
	baseband_configuration.decimation_factor = v;
	update_baseband_configuration();
}

void TransmitterModel::enable() {
	radio::set_direction(rf::Direction::Transmit);
	update_tuning_frequency();
	update_rf_amp();
	update_lna();
	update_vga();
	update_baseband_bandwidth();
	update_baseband_configuration();
	radio::streaming_enable();
}

void TransmitterModel::disable() {
	/* TODO: This is a dumb hack to stop baseband from working so hard. */
	BasebandConfigurationMessage message {
		.configuration = {
			.mode = NONE,
			.sampling_rate = 0,
			.decimation_factor = 1,
		}
	};
	shared_memory.baseband_queue.push(message);

	radio::disable();
}

int32_t TransmitterModel::tuning_offset() {
	return -(sampling_rate() / 4);
}

void TransmitterModel::update_tuning_frequency() {
	radio::set_tuning_frequency(tuning_frequency());
}

void TransmitterModel::update_rf_amp() {
	radio::set_rf_amp(rf_amp_);
}

void TransmitterModel::update_lna() {
	radio::set_lna_gain(lna_gain_db_);
}

void TransmitterModel::update_baseband_bandwidth() {
	radio::set_baseband_filter_bandwidth(baseband_bandwidth_);
}

void TransmitterModel::update_vga() {
	radio::set_vga_gain(vga_gain_db_);
}

void TransmitterModel::update_modulation() {
	update_baseband_configuration();
}

void TransmitterModel::set_baseband_configuration(const BasebandConfiguration config) {
	baseband_configuration = config;
	update_baseband_configuration();
}

void TransmitterModel::update_baseband_configuration() {
	radio::streaming_disable();

	clock_manager.set_sampling_frequency(sampling_rate() * baseband_oversampling());
	update_tuning_frequency();
	radio::set_baseband_decimation_by(baseband_oversampling());

	BasebandConfigurationMessage message { baseband_configuration };
	shared_memory.baseband_queue.push(message);

	radio::streaming_enable();
}

