/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_siggen.hpp"

#include "tonesets.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void SigGenView::focus() {
    options_shape.focus();
}

SigGenView::~SigGenView() {
    transmitter_model.disable();
    baseband::shutdown();
}

void SigGenView::update_config() {
    if (checkbox_stop.value())
        baseband::set_siggen_config(transmitter_model.channel_bandwidth(), (options_mod.selected_index_value() << 4) + options_shape.selected_index_value(), field_stop.value());
    else
        baseband::set_siggen_config(transmitter_model.channel_bandwidth(), (options_mod.selected_index_value() << 4) + options_shape.selected_index_value(), 0);
}

void SigGenView::update_tone() {
    baseband::set_siggen_tone(symfield_tone.to_integer());
}

void SigGenView::start_tx() {
    transmitter_model.enable();

    update_tone();

    /*auto duration = field_stop.value();
        if (!checkbox_auto.value())
                duration = 0;*/
    update_config();
}

void SigGenView::on_tx_progress(const uint32_t progress, const bool done) {
    (void)progress;

    if (done) {
        transmitter_model.disable();
        tx_view.set_transmitting(false);
    }
}

SigGenView::SigGenView(
    NavigationView& nav) {
    baseband::run_image(portapack::spi_flash::image_tag_siggen);

    add_children({&labels,
                  &options_mod,
                  &options_shape,
                  &text_shape,
                  &symfield_tone,
                  &button_update,
                  &checkbox_auto,
                  &checkbox_stop,
                  &field_stop,
                  &tx_view});

    symfield_tone.hidden(true);  // At first launch , by default we are in CW: Shape ignored, we are not using Tone modulation.
    options_shape.hidden(true);
    text_shape.hidden(true);
    symfield_tone.set_value(1000);  // Default: 1000 Hz
    options_shape.on_change = [this](size_t, OptionsField::value_t v) {
        text_shape.set(shape_strings[v]);
        if (auto_update)
            update_config();

        if (v == 5) {  // In Shape Pseudo Random Noise we are not using Tone modulation freq.
            symfield_tone.hidden(true);
        } else {
            symfield_tone.hidden(false);
        }

        set_dirty();
    };
    options_shape.set_selected_index(0);
    text_shape.set(shape_strings[0]);

    options_mod.on_change = [this](size_t, OptionsField::value_t v) {
        if (auto_update)
            update_config();

        if (v == 0) {  // In Modulation Options CW we are not using Tone modulation freq.
            symfield_tone.hidden(true);
        } else {
            symfield_tone.hidden(false);
        }

        if ((v == 0) || (v == 2) || (v == 3)) {  // In Modulation Options CW, QPSK, BPSK we are not using Shapes.
            options_shape.hidden(true);
            text_shape.hidden(true);
        } else {
            options_shape.hidden(false);
            text_shape.hidden(false);
        }

        set_dirty();
    };
    options_mod.set_selected_index(0);

    field_stop.set_value(1);

    symfield_tone.set_value(1000);  // Default: 1000 Hz
    symfield_tone.on_change = [this](SymField&) {
        if (auto_update)
            update_tone();
    };

    button_update.on_select = [this](Button&) {
        update_tone();
        update_config();
    };

    checkbox_auto.on_select = [this](Checkbox&, bool v) {
        auto_update = v;
    };

    tx_view.on_edit_frequency = [this, &nav]() {
        auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.target_frequency());
        new_view->on_changed = [this](rf::Frequency f) {
            transmitter_model.set_target_frequency(f);
        };
    };

    tx_view.on_start = [this]() {
        start_tx();
        tx_view.set_transmitting(true);
    };

    tx_view.on_stop = [this]() {
        transmitter_model.disable();
        tx_view.set_transmitting(false);
    };
}

} /* namespace ui */
