/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "receiver_model.hpp"

#include "baseband_api.hpp"

#include "portapack_persistent_memory.hpp"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "audio.hpp"
#include "dsp_fir_taps.hpp"
#include "dsp_iir.hpp"
#include "dsp_iir_config.hpp"
#include "utility.hpp"

using namespace hackrf::one;
using namespace portapack;

namespace {

static constexpr std::array<baseband::AMConfig, 5> am_configs{{
    // we config here all the non COMMON parameters to each AM modulation type in RX.
    {taps_9k0_decim_2, taps_9k0_dsb_channel, AMConfigureMessage::Modulation::DSB},  // AM DSB-C  BW 9khz  (+-4k5)  commercial EU bandwidth .
    {taps_6k0_decim_2, taps_6k0_dsb_channel, AMConfigureMessage::Modulation::DSB},  // AM DSB-C  BW 6khz  (+-3k0)  narrow AM , ham equipments.
    {taps_6k0_decim_2, taps_2k8_usb_channel, AMConfigureMessage::Modulation::SSB},  // SSB USB   BW 2K8   (+ 2K8)
    {taps_6k0_decim_2, taps_2k8_lsb_channel, AMConfigureMessage::Modulation::SSB},  // SSB LSB   BW 2K8   (- 2K8)
    {taps_6k0_decim_2, taps_0k7_usb_channel, AMConfigureMessage::Modulation::SSB},  // SSB USB   BW 0K7   (+ 0K7)  used to get audio tone from CW Morse, assuming tx shifted +700hz aprox
}};

static constexpr std::array<baseband::NBFMConfig, 3> nbfm_configs{{
    {taps_4k25_decim_0, taps_4k25_decim_1, taps_4k25_channel, 2500},
    {taps_11k0_decim_0, taps_11k0_decim_1, taps_11k0_channel, 2500},
    {taps_16k0_decim_0, taps_16k0_decim_1, taps_16k0_channel, 5000},
}};

static constexpr std::array<baseband::WFMConfig, 3> wfm_configs{{
    {taps_200k_wfm_decim_0, taps_200k_wfm_decim_1},
    {taps_180k_wfm_decim_0, taps_180k_wfm_decim_1},
    {taps_40k_wfm_decim_0, taps_40k_wfm_decim_1},
}};

} /* namespace */

rf::Frequency ReceiverModel::target_frequency() const {
    return persistent_memory::target_frequency();
}

void ReceiverModel::set_target_frequency(rf::Frequency f) {
    persistent_memory::set_target_frequency(f);
    settings_.frequency_app_override = f;
    update_tuning_frequency();
}

uint32_t ReceiverModel::baseband_bandwidth() const {
    return settings_.baseband_bandwidth;
}

void ReceiverModel::set_baseband_bandwidth(uint32_t v) {
    settings_.baseband_bandwidth = v;
    update_baseband_bandwidth();
}

uint32_t ReceiverModel::sampling_rate() const {
    return settings_.sampling_rate;
}

void ReceiverModel::set_sampling_rate(uint32_t v) {
    settings_.sampling_rate = v;
    update_sampling_rate();
}

rf::Frequency ReceiverModel::frequency_step() const {
    return settings_.frequency_step;
}

void ReceiverModel::set_frequency_step(rf::Frequency f) {
    settings_.frequency_step = f;
}

uint8_t ReceiverModel::lna() const {
    return settings_.lna_gain_db;
}

void ReceiverModel::set_lna(uint8_t v_db) {
    settings_.lna_gain_db = v_db;
    update_lna();
}

uint8_t ReceiverModel::vga() const {
    return settings_.vga_gain_db;
}

void ReceiverModel::set_vga(uint8_t v_db) {
    settings_.vga_gain_db = v_db;
    update_vga();
}

bool ReceiverModel::rf_amp() const {
    return settings_.rf_amp;
}

void ReceiverModel::set_rf_amp(bool enabled) {
    settings_.rf_amp = enabled;
    update_rf_amp();
}

ReceiverModel::Mode ReceiverModel::modulation() const {
    return settings_.mode;
}

void ReceiverModel::set_modulation(Mode v) {
    settings_.mode = v;
    update_modulation();
}

uint8_t ReceiverModel::am_configuration() const {
    return settings_.am_config_index;
}

void ReceiverModel::set_am_configuration(uint8_t n) {
    if (n < am_configs.size()) {
        settings_.am_config_index = n;
        update_modulation();
    }
}

uint8_t ReceiverModel::nbfm_configuration() const {
    return settings_.nbfm_config_index;
}

void ReceiverModel::set_nbfm_configuration(uint8_t n) {
    if (n < nbfm_configs.size()) {
        settings_.nbfm_config_index = n;
        update_modulation();
    }
}

uint8_t ReceiverModel::wfm_configuration() const {
    return settings_.wfm_config_index;
}

void ReceiverModel::set_wfm_configuration(uint8_t n) {
    if (n < wfm_configs.size()) {
        settings_.wfm_config_index = n;
        update_modulation();
    }
}

uint8_t ReceiverModel::squelch_level() const {
    return settings_.squelch_level;
}

void ReceiverModel::set_squelch_level(uint8_t v) {
    settings_.squelch_level = v;
    update_modulation();
}

void ReceiverModel::set_antenna_bias() {
    update_antenna_bias();
}

volume_t ReceiverModel::headphone_volume() const {
    return persistent_memory::headphone_volume();
}

void ReceiverModel::set_headphone_volume(volume_t v) {
    persistent_memory::set_headphone_volume(v);
    update_headphone_volume();
}

uint8_t ReceiverModel::normalized_headphone_volume() const {
    auto db = (headphone_volume() - audio::headphone::volume_range().max).decibel();
    return clip<uint8_t>(db + 99, 0, 99);
}

void ReceiverModel::set_normalized_headphone_volume(uint8_t v) {
    // TODO: Linear map instead to ensure 0 is minimal value or fix volume_range_t::normalize.
    v = clip<uint8_t>(v, 0, 99);
    auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
    set_headphone_volume(new_volume);
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
    update_sampling_rate();
    update_modulation();

    // TODO: maybe not the perfect place for this, but it's reasonable.
    update_headphone_volume();

    led_rx.on();
}

void ReceiverModel::disable() {
    enabled_ = false;

    // TODO: Responsibility for enabling/disabling the radio is muddy.
    // Some happens in ReceiverModel, some inside radio namespace.
    radio::disable();
    led_rx.off();
}

void ReceiverModel::initialize() {
    settings_ = settings_t{};
}

void ReceiverModel::set_configuration_without_update(
    Mode new_mode,
    rf::Frequency new_frequency_step,
    size_t new_am_config_index,
    size_t new_nbfm_config_index,
    size_t new_wfm_config_index,
    uint8_t new_squelch_level) {
    settings_.mode = new_mode;
    settings_.frequency_step = new_frequency_step;
    settings_.am_config_index = new_am_config_index;
    settings_.nbfm_config_index = new_nbfm_config_index;
    settings_.wfm_config_index = new_wfm_config_index;
    settings_.squelch_level = new_squelch_level;
}

void ReceiverModel::configure_from_app_settings(
    const app_settings::AppSettings& settings) {
    settings_.baseband_bandwidth = settings.baseband_bandwidth;
    settings_.sampling_rate = settings.sampling_rate;
    settings_.lna_gain_db = settings.lna;
    settings_.vga_gain_db = settings.vga;
    settings_.rf_amp = settings.rx_amp;
    settings_.squelch_level = settings.squelch;
}

int32_t ReceiverModel::tuning_offset() {
    if ((modulation() == Mode::SpectrumAnalysis)) {
        return 0;
    } else {
        return -(sampling_rate() / 4);
    }
}

void ReceiverModel::update_tuning_frequency() {
    // TODO: use positive offset if freq < offset.
    radio::set_tuning_frequency(target_frequency() + tuning_offset());
}

void ReceiverModel::update_baseband_bandwidth() {
    radio::set_baseband_filter_bandwidth_rx(baseband_bandwidth());
}

void ReceiverModel::update_sampling_rate() {
    // TODO: Move more low-level radio control stuff to M4. It'll enable tighter
    // synchronization for things like wideband (sweeping) spectrum analysis, and
    // protocols that need quick RX/TX turn-around.

    // Disabling baseband while changing sampling rates seems like a good idea...
    radio::set_baseband_rate(sampling_rate());
    update_tuning_frequency();
}

void ReceiverModel::update_lna() {
    radio::set_lna_gain(lna());
}

void ReceiverModel::update_vga() {
    radio::set_vga_gain(vga());
}

void ReceiverModel::update_rf_amp() {
    radio::set_rf_amp(rf_amp());
}

void ReceiverModel::update_modulation() {
    switch (modulation()) {
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
        case Mode::Capture:
            break;
    }
}

void ReceiverModel::update_am_configuration() {
    am_configs[am_configuration()].apply();
}

void ReceiverModel::update_nbfm_configuration() {
    nbfm_configs[nbfm_configuration()].apply(squelch_level());
}

void ReceiverModel::update_wfm_configuration() {
    wfm_configs[wfm_configuration()].apply();
}

void ReceiverModel::update_antenna_bias() {
    if (enabled_)
        radio::set_antenna_bias(portapack::get_antenna_bias());
}

void ReceiverModel::update_headphone_volume() {
    audio::headphone::set_volume(headphone_volume());
}
