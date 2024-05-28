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

#include "ui_sigfrx.hpp"
#include "ui_receiver.hpp"

#include "ch.h"
#include "evtimer.h"

#include "event_m0.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "string_format.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void SIGFRXView::focus() {
    button_exit.focus();
}

SIGFRXView::~SIGFRXView() {
    receiver_model.disable();
}

void SIGFRXView::paint(Painter& painter) {
    uint8_t i, xp;

    // portapack::display.drawBMP({0, 302-160}, fox_bmp);
    portapack::display.fill_rectangle({0, 16, 240, 160 - 16}, Theme::getInstance()->bg_darkest->foreground);
    for (i = 0; i < 6; i++) {
        xp = sigfrx_marks[i * 3];
        painter.draw_string({(ui::Coord)sigfrx_marks[(i * 3) + 1], 144 - 20}, style_white, to_string_dec_uint(sigfrx_marks[(i * 3) + 2]));
        portapack::display.draw_line({xp, 144 - 4}, {xp, 144}, Theme::getInstance()->bg_darkest->background);
    }
}

void SIGFRXView::on_channel_spectrum(const ChannelSpectrum& spectrum) {
    portapack::display.fill_rectangle({0, 144, 240, 4}, Theme::getInstance()->bg_darkest->foreground);

    uint8_t xmax = 0, imax = 0;
    size_t i;

    for (i = 0; i < 120; i++) {
        if (spectrum.db[i] > xmax) {
            xmax = spectrum.db[i];
            imax = i;
        }
    }
    for (i = 136; i < 256; i++) {
        if (spectrum.db[i - 16] > xmax) {
            xmax = spectrum.db[i - 16];
            imax = i - 16;
        }
    }

    if ((imax >= last_channel - 2) && (imax <= last_channel + 2)) {
        if (detect_counter >= 5) {
            // Latched !
        } else {
            detect_counter++;
        }
    } else {
        if (detect_counter >= 5) text_channel.set("...        ");
        detect_counter = 0;
    }

    last_channel = imax;

    portapack::display.fill_rectangle({(ui::Coord)(imax - 2), 144, 4, 4}, Theme::getInstance()->fg_red->foreground);
}

void SIGFRXView::on_show() {
    /*EventDispatcher::message_map().register_handler(Message::ID::ChannelSpectrum,
                [this](const Message* const p) {
                        this->on_channel_spectrum(reinterpret_cast<const ChannelSpectrumMessage*>(p)->spectrum);
                }
        );*/
}

void SIGFRXView::on_hide() {
    // EventDispatcher::message_map().unregister_handler(Message::ID::ChannelSpectrum);
}

SIGFRXView::SIGFRXView(
    NavigationView& nav) {
    receiver_model.set_baseband_configuration({
        .mode = 255,  // DEBUG
        .sampling_rate = 3072000,
        .decimation_factor = 4,
    });
    // TODO: use settings.
    receiver_model.set_lna(0);
    receiver_model.set_vga(0);
    receiver_model.set_target_frequency(868110000);

    add_children({&text_type,
                  &text_channel,
                  &text_data,
                  &button_exit});

    text_type.set_style(Theme::getInstance()->bg_lightest);
    text_channel.set_style(Theme::getInstance()->bg_lightest);
    text_data.set_style(Theme::getInstance()->bg_lightest);

    button_exit.on_select = [&nav](Button&) {
        nav.pop();
    };

    receiver_model.enable();
}

} /* namespace ui */
