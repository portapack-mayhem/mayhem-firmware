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

#include "baseband_api.hpp"

#include "portapack_persistent_memory.hpp"
using namespace portapack;

#include "radio.hpp"
#include "audio.hpp"

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

void TransmitterModel::enable() {
	enabled_ = true;
	radio::set_direction(rf::Direction::Transmit);
	update_tuning_frequency();
	update_rf_amp();
	update_lna();
	update_vga();
	update_baseband_bandwidth();
	update_baseband_configuration();
}

void TransmitterModel::disable() {
	enabled_ = false;
	baseband::stop();

	// TODO: Responsibility for enabling/disabling the radio is muddy.
	// Some happens in ReceiverModel, some inside radio namespace.
	radio::disable();
}

uint32_t TransmitterModel::baseband_oversampling() const {
	// TODO: Rename decimation_factor.
	return baseband_configuration.decimation_factor;
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

uint32_t TransmitterModel::modulation() const {
	return baseband_configuration.mode;
}

void TransmitterModel::set_baseband_configuration(const BasebandConfiguration config) {
	baseband_configuration = config;
	update_baseband_configuration();
}

void TransmitterModel::update_baseband_configuration() {
	// TODO: Move more low-level radio control stuff to M4. It'll enable tighter
	// synchronization for things like wideband (sweeping) spectrum analysis, and
	// protocols that need quick RX/TX turn-around.

	// Disabling baseband while changing sampling rates seems like a good idea...
	baseband::stop();

	radio::set_baseband_rate(sampling_rate() * baseband_oversampling());
	update_tuning_frequency();
	radio::set_baseband_decimation_by(baseband_oversampling());

	baseband::start(baseband_configuration);
}
