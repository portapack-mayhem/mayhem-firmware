/*
 * Copyright (C) 2025 StarVore Labs
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

#include "ui_sstv_rx.hpp"

#include "portapack_persistent_memory.hpp"
#include "portapack.hpp"
#include "hackrf_hal.hpp"
#include <cstdint>
#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace modems;
using namespace ui;

// SSTV RX View Implementation
namespace ui::external_app::sstv_rx {

SstvRxView::SstvRxView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
    add_children({
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &field_frequency,
        &field_volume,
        &field_bw,
        &audio,
        &start_btn,
        &stop_btn,
        &options_mode,
        &labels
    });

    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t mode_options;
    uint32_t c;

    // Start button handlers
    start_btn.on_select = [this](Button&) {
        start_btn.focus();
        on_start();
    };

    // Stop button handlers
    stop_btn.on_select = [this](Button&) {
        stop_btn.focus();
        on_stop();
    };

    // Initialize frequency field
    if (field_frequency.value() == 0) {
        field_frequency.set_value(96100000);  // Default to 96.100 MHz
    }
    field_frequency.set_step(25000);

    // Populate mode list
    for (c = 0; c < SSTV_MODES_NB; c++)
        mode_options.emplace_back(sstv_modes[c].name, c);
    options_mode.set_options(mode_options);

    options_mode.on_change = [this](size_t i, int32_t) {
        this->on_mode_changed(i);
    };
    options_mode.set_selected_index(1);  // Scottie 2
    on_mode_changed(1);
}

// Destructor: Ensure reception is stopped
SstvRxView::~SstvRxView() {
    is_receiving = true;
    on_stop();
}

void SstvRxView::on_show() {
    return;
}

void SstvRxView::focus() {
    field_frequency.focus();
}

// Start WFM audio reception if not already started
void SstvRxView::on_start() {
    if (!is_receiving) {
        start_audio();
    }
}

// Stop WFM audio reception
void SstvRxView::on_stop() {
    if (is_receiving) {
        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
        is_receiving = false;
    }
}

// Start WFM audio reception
void SstvRxView::start_audio() {
    is_receiving = true;
    field_bw.on_change = [this](size_t n, OptionsField::value_t) { (void)n; };
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();

    std::fill(audio_spectrum, audio_spectrum + 128, 0);
    audio_sampling_rate = audio::Rate::Hz_48000;
    freqman_set_bandwidth_option(2, field_bw);  // WFM_MODULATION
    baseband::run_image(portapack::spi_flash::image_tag_wfm_audio);
    receiver_mode = ReceiverModel::Mode::WidebandFMAudio;
    field_bw.set_by_value(0);  // 200k default
    receiver_model.set_wfm_configuration(field_bw.selected_index_value());
    field_bw.on_change = [this](size_t index, OptionsField::value_t n) {
        radio_bw = index;
        receiver_model.set_wfm_configuration(n);
    };

    receiver_model.set_modulation(receiver_mode);

    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);
    audio::set_rate(audio_sampling_rate);
    audio::output::start();
    receiver_model.set_headphone_volume(receiver_model.headphone_volume());  // WM8731 hack
    receiver_model.enable();
}

void SstvRxView::on_mode_changed(const size_t index) {
    sstv_color_seq rx_color_sequence;
    rx_sstv_mode = &sstv_modes[index];
    rx_color_sequence = sstv_modes[index].color_sequence;
}

}  // namespace ui::external_app::sstv_rx