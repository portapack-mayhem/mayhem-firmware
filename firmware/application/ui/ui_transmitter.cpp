/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

// TODO: Consolidate and make TX Widgets instead like ui_receiver.

#include "ui_transmitter.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "string_format.hpp"
#include "max2837.hpp"

using namespace portapack;

#define POWER_THRESHOLD_HIGH 47
#define POWER_THRESHOLD_MED 38
#define POWER_THRESHOLD_LOW 17

namespace ui {

/* Gets a style indicating total TX gain level. */
static const Style* get_style_for_gain(uint8_t tot_gain) {
    if (tot_gain > POWER_THRESHOLD_HIGH) return Theme::getInstance()->fg_red;

    if (tot_gain > POWER_THRESHOLD_MED)
        return Theme::getInstance()->fg_orange;

    if (tot_gain > POWER_THRESHOLD_LOW)
        return Theme::getInstance()->fg_yellow;

    return nullptr;  // Uses default.
}

/* TransmitterView *******************************************************/

void TransmitterView::paint(Painter& painter) {
    size_t c;
    Point pos = {0, screen_pos().y()};

    for (c = 0; c < 20; c++) {
        painter.draw_bitmap(
            pos,
            bitmap_stripes,
            Theme::getInstance()->fg_yellow->foreground,
            Theme::getInstance()->fg_yellow->background);
        if (c != 9)
            pos += {24, 0};
        else
            pos = {0, screen_pos().y() + 32 + 8};
    }
}

void TransmitterView::on_target_frequency_changed(rf::Frequency f) {
    transmitter_model.set_target_frequency(f);
}

void TransmitterView::on_channel_bandwidth_changed(uint32_t channel_bandwidth) {
    // TODO: this doesn't actually affect the radio through the model.
    transmitter_model.set_channel_bandwidth(channel_bandwidth);
}

void TransmitterView::on_tx_gain_changed(int32_t tx_gain) {
    transmitter_model.set_tx_gain(tx_gain);
    update_gainlevel_styles();
}

void TransmitterView::on_tx_amp_changed(bool rf_amp) {
    transmitter_model.set_rf_amp(rf_amp);
    update_gainlevel_styles();
}

void TransmitterView::update_gainlevel_styles() {
    int8_t tot_gain = transmitter_model.tx_gain() + (transmitter_model.rf_amp() ? 14 : 0);
    auto style = get_style_for_gain(tot_gain);

    field_gain.set_style(style);
    text_gain.set_style(style);
    field_amp.set_style(style);
    text_amp.set_style(style);
}

void TransmitterView::set_transmitting(const bool transmitting) {
    if (transmitting) {
        button_start.set_text("STOP");
        button_start.set_style(&style_stop);
    } else {
        button_start.set_text("START");
        button_start.set_style(&style_start);
    }

    transmitting_ = transmitting;
}

void TransmitterView::on_show() {
    field_frequency.set_value(transmitter_model.target_frequency());

    field_gain.set_value(transmitter_model.tx_gain());
    field_amp.set_value(transmitter_model.rf_amp() ? 14 : 0);

    update_gainlevel_styles();
}

void TransmitterView::focus() {
    button_start.focus();
}

TransmitterView::TransmitterView(
    const Coord y,
    const uint64_t frequency_step,
    const uint32_t channel_bandwidth,
    const bool lock)
    : lock_{lock} {
    set_parent_rect({0, y, 30 * 8, 6 * 8});

    add_children({
        &field_frequency,
        &field_frequency_step,
        &text_gain,
        &field_gain,
        &button_start,
        &text_amp,
        &field_amp,
    });

    set_transmitting(false);

    if (lock_) {
        field_frequency.set_focusable(false);
        field_frequency.set_style(&style_locked);
    } else {
        // TODO: Make a widget.
        if (channel_bandwidth) {
            add_children({&text_bw,
                          &field_bw});

            field_bw.on_change = [this](int32_t v) {
                on_channel_bandwidth_changed(v * 1000);
            };
            field_bw.set_value(channel_bandwidth);
        }
    }

    field_frequency.on_change = [this](rf::Frequency f) {
        on_target_frequency_changed(f);
    };
    field_frequency.on_edit = [this]() {
        if (on_edit_frequency)
            on_edit_frequency();
    };

    field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
        receiver_model.set_frequency_step(v);
        this->field_frequency.set_step(v);
    };
    // TODO: Shouldn't be a ctor parameter because it doesn't work with app settings.
    field_frequency_step.set_by_value(frequency_step);

    field_gain.on_change = [this](uint32_t tx_gain) {
        on_tx_gain_changed(tx_gain);
    };

    field_amp.on_change = [this](uint32_t rf_amp) {
        on_tx_amp_changed((bool)rf_amp);
    };

    button_start.on_select = [this](Button&) {
        if (transmitting_) {
            if (on_stop)
                on_stop();
        } else {
            if (on_start)
                on_start();
        }
    };
}

TransmitterView::~TransmitterView() {
    // TODO: Does this make sense? Seems wrong to have
    // what's basically a widget control the radio.
    audio::output::stop();
    transmitter_model.disable();
    baseband::shutdown();
}

/* TransmitterView2 *******************************************************/

TransmitterView2::TransmitterView2(Point pos, bool short_ui) {
    // There are two modes, short and !short
    // Short:  "G:XX A:YY"
    // !Short: "Gain:XX Amp:YY"

    Dim width = short_ui ? (9 * 8) : (14 * 8);
    set_parent_rect({pos, {width, 16}});

    add_children({
        &text_labels,
        &field_gain,
        &field_amp,
    });

    // Set up controls depending UI mode.
    text_labels.set(short_ui ? "G:   A:" : "Gain:   Amp:");
    text_labels.set_parent_rect(
        short_ui
            ? Rect{0 * 8, 0 * 16, 7 * 8, 1 * 16}
            : Rect{0 * 8, 0 * 16, 12 * 8, 1 * 16});
    field_gain.set_parent_rect(
        short_ui
            ? Rect{2 * 8, 0 * 16, 2 * 8, 1 * 16}
            : Rect{5 * 8, 0 * 16, 2 * 8, 1 * 16});
    field_amp.set_parent_rect(
        short_ui
            ? Rect{7 * 8, 0 * 16, 2 * 8, 1 * 16}
            : Rect{12 * 8, 0 * 16, 2 * 8, 1 * 16});

    field_gain.set_value(transmitter_model.tx_gain());
    field_gain.on_change = [this](uint32_t tx_gain) {
        transmitter_model.set_tx_gain(tx_gain);
        update_gainlevel_styles();
    };

    field_amp.set_value(transmitter_model.rf_amp() ? 14 : 0);
    field_amp.on_change = [this](uint32_t rf_amp) {
        transmitter_model.set_rf_amp(rf_amp > 0);
        update_gainlevel_styles();
    };

    update_gainlevel_styles();
}

void TransmitterView2::update_gainlevel_styles() {
    int8_t tot_gain = transmitter_model.tx_gain() + (transmitter_model.rf_amp() ? 14 : 0);
    auto style = get_style_for_gain(tot_gain);

    text_labels.set_style(style);
    field_gain.set_style(style);
    field_amp.set_style(style);
}

} /* namespace ui */
