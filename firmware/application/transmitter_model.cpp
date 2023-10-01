/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

#include "audio.hpp"
#include "baseband_api.hpp"
#include "event_m0.hpp"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "radio.hpp"

using namespace hackrf::one;
using namespace portapack;

rf::Frequency TransmitterModel::target_frequency() const {
    return persistent_memory::target_frequency();
}

void TransmitterModel::set_target_frequency(rf::Frequency f) {
    persistent_memory::set_target_frequency(f);
    update_tuning_frequency();
}

uint32_t TransmitterModel::baseband_bandwidth() const {
    return settings_.baseband_bandwidth;
}

void TransmitterModel::set_baseband_bandwidth(uint32_t v) {
    settings_.baseband_bandwidth = v;
    update_baseband_bandwidth();
}

uint32_t TransmitterModel::sampling_rate() const {
    return settings_.sampling_rate;
}

void TransmitterModel::set_sampling_rate(uint32_t v) {
    settings_.sampling_rate = v;
    update_sampling_rate();
}

uint32_t TransmitterModel::channel_bandwidth() const {
    return settings_.channel_bandwidth;
}

void TransmitterModel::set_channel_bandwidth(uint32_t v) {
    settings_.channel_bandwidth = v;
}

uint8_t TransmitterModel::tx_gain() const {
    return settings_.tx_gain_db;
}

void TransmitterModel::set_tx_gain(uint8_t v_db) {
    settings_.tx_gain_db = v_db;
    update_tx_gain();
}

bool TransmitterModel::rf_amp() const {
    return settings_.rf_amp;
}

void TransmitterModel::set_rf_amp(bool enabled) {
    settings_.rf_amp = enabled;
    update_rf_amp();
}

void TransmitterModel::set_antenna_bias() {
    update_antenna_bias();
}

void TransmitterModel::enable() {
    enabled_ = true;
    radio::set_direction(rf::Direction::Transmit);
    update_tuning_frequency();
    update_antenna_bias();
    update_rf_amp();
    update_baseband_bandwidth();
    update_sampling_rate();
    update_tx_gain();

    led_tx.on();
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };

    if (portapack::persistent_memory::stealth_mode()) {
        DisplaySleepMessage message;
        EventDispatcher::send_message(message);
    }
}

void TransmitterModel::disable() {
    enabled_ = false;

    radio::disable();

    rtc_time::signal_tick_second -= signal_token_tick_second;
    led_tx.off();
}

void TransmitterModel::initialize() {
    settings_ = settings_t{};
}

void TransmitterModel::configure_from_app_settings(
    const app_settings::AppSettings& settings) {
    settings_.baseband_bandwidth = settings.baseband_bandwidth;
    settings_.sampling_rate = settings.sampling_rate;
    settings_.channel_bandwidth = settings.channel_bandwidth;
    settings_.tx_gain_db = settings.tx_gain;
    settings_.rf_amp = settings.tx_amp;
}

void TransmitterModel::update_tuning_frequency() {
    radio::set_tuning_frequency(target_frequency());
}

void TransmitterModel::update_baseband_bandwidth() {
    radio::set_baseband_filter_bandwidth_tx(baseband_bandwidth());
}

void TransmitterModel::update_sampling_rate() {
    // TODO: Move more low-level radio control stuff to M4. It'll enable tighter
    // synchronization for things like wideband (sweeping) spectrum analysis, and
    // protocols that need quick RX/TX turn-around.

    // Disabling baseband while changing sampling rates seems like a good idea...

    radio::set_baseband_rate(sampling_rate());
    update_tuning_frequency();
}

void TransmitterModel::update_tx_gain() {
    radio::set_tx_gain(tx_gain());
}

void TransmitterModel::update_rf_amp() {
    radio::set_rf_amp(rf_amp());
}

void TransmitterModel::update_antenna_bias() {
    if (enabled_)
        radio::set_antenna_bias(portapack::get_antenna_bias());
}

void TransmitterModel::on_tick_second() {
    if (portapack::persistent_memory::stealth_mode())
        led_tx.toggle();
}
