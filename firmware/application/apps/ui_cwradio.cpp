/*
 * Copyright (C) 2026
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

#include "ui_cwradio.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "audio.hpp"
#include "irq_controls.hpp"

using namespace portapack;

namespace ui {

CWRadioView::CWRadioView(NavigationView& nav)
    : nav_{nav} {

    baseband::run_image(portapack::spi_flash::image_tag_morse_tx);

    add_children({&labels,
                  &field_frequency,
                  &tx_view,
                  &options_mode,
                  &field_tone,
                  &field_fm_delta,
                  &text_fm_delta_label,
                  &button_key,
                  &text_status,
                  &text_instructions});

    // Load settings
    field_frequency.set_value(settings_.target_frequency);
    field_frequency.set_step(50);  // 50 Hz steps for precise CW tuning
    field_frequency.on_change = [this](rf::Frequency f) {
        settings_.target_frequency = f;
        transmitter_model.set_target_frequency(f);
    };
    field_frequency.on_edit = [this, &nav]() {
        auto freq_view = nav.push<FrequencyKeypadView>(settings_.target_frequency);
        freq_view->on_changed = [this](rf::Frequency f) {
            field_frequency.set_value(f);
            settings_.target_frequency = f;
            transmitter_model.set_target_frequency(f);
        };
    };

    // Set default values if settings are empty
    if (settings_.target_frequency == 0) {
        settings_.target_frequency = default_frequency;
        field_frequency.set_value(default_frequency);
    }

    // Configure tone field
    field_tone.set_value(default_tone);
    field_tone.on_change = [this](int32_t v) {
        (void)v;
        if (transmitting_) {
            update_tx();
        }
    };

    // Configure FM deviation field
    field_fm_delta.set_value(default_fm_delta);
    field_fm_delta.on_change = [this](int32_t v) {
        (void)v;
        if (transmitting_) {
            update_tx();
        }
    };

    // Update FM delta visibility based on mode
    text_fm_delta_label.hidden(true);
    field_fm_delta.hidden(true);

    // Configure mode selection
    options_mode.set_selected_index(0);  // Default to AM
    options_mode.on_change = [this](size_t index, int32_t value) {
        (void)index;
        selected_modulation_ = value;

        // Show/hide FM delta controls based on mode
        bool is_fm = (selected_modulation_ == 1);
        text_fm_delta_label.hidden(!is_fm);
        field_fm_delta.hidden(!is_fm);

        if (transmitting_) {
            update_tx();
        }
    };

    // Configure transmitter view
    tx_view.on_start = [this]() {
        start_tx();
    };
    tx_view.on_stop = [this]() {
        stop_tx();
    };

    // Configure key button
    button_key.on_select = [this](Button&) {
        // Button press handled by on_touch
    };

    button_key.on_touch_press = [this](TouchEvent) {
        button_held_ = true;
        on_button_change(true);
        return true;
    };

    button_key.on_touch_release = [this](TouchEvent) {
        button_held_ = false;
        on_button_change(false);
        return true;
    };

    // Set up encoder wheel handling for keying
    receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
    audio::output::start();
}

CWRadioView::~CWRadioView() {
    stop_tx();
    baseband::shutdown();
    audio::output::stop();
}

void CWRadioView::focus() {
    field_frequency.focus();
}

void CWRadioView::on_show() {
    // Update button state when showing
    update_button_state();
}

void CWRadioView::start_tx() {
    if (transmitting_)
        return;

    transmitting_ = true;

    // Configure transmitter
    transmitter_model.set_target_frequency(settings_.target_frequency);
    transmitter_model.set_baseband_bandwidth(150'000);  // 150 kHz for CW
    transmitter_model.set_sampling_rate(1'536'000);

    // Set appropriate power level (low power for practice)
    transmitter_model.set_tx_gain(settings_.tx_gain);
    transmitter_model.set_rf_amp(settings_.tx_amp);

    // Configure the morse TX baseband
    update_tx();

    // Enable transmitter
    transmitter_model.enable();

    text_status.set("TX ENABLED - Key to transmit");

    // Start with key up
    on_tx_key_change(false);
}

void CWRadioView::stop_tx() {
    if (!transmitting_)
        return;

    // Make sure key is released
    if (key_is_down_) {
        on_tx_key_change(false);
    }

    transmitting_ = false;
    transmitter_model.disable();

    text_status.set("TX STOPPED");
}

void CWRadioView::update_tx() {
    if (!transmitting_)
        return;

    uint32_t tone = field_tone.value();
    uint32_t fm_delta = (selected_modulation_ == 1) ? field_fm_delta.value() : default_fm_delta;

    baseband::set_morsetx_config(
        selected_modulation_,
        tone,
        fm_delta);
}

void CWRadioView::on_button_change(bool pressed) {
    if (!transmitting_)
        return;

    // Update key state
    key_is_down_ = pressed;
    on_tx_key_change(pressed);
    update_button_state();
}

void CWRadioView::on_encoder_change(int32_t delta) {
    if (!transmitting_)
        return;

    // Use encoder for keying as well
    // Any encoder movement keys down briefly
    if (delta != 0) {
        on_tx_key_change(true);
        chThdSleepMilliseconds(100);  // Brief key down
        on_tx_key_change(false);
    }
}

void CWRadioView::on_tx_key_change(bool key_down) {
    if (!transmitting_)
        return;

    key_is_down_ = key_down;

    // Send key state to baseband
    baseband::set_morsetx_key(key_down);

    // Update status display
    if (key_down) {
        text_status.set("TX - KEY DOWN");
    } else {
        text_status.set("TX ENABLED - Key to transmit");
    }
}

void CWRadioView::update_button_state() {
    if (key_is_down_) {
        button_key.set_text("KEY DOWN");
        button_key.set_style(Theme::getInstance()->bg_dark);
    } else {
        button_key.set_text("PRESS TO KEY");
        button_key.set_style(Theme::getInstance()->bg_dark);
    }
}

} /* namespace ui */
