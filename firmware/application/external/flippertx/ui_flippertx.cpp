/*
 * Copyright (C) 2024 HTotoo
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

#include "ui_flippertx.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace ui;

namespace ui::external_app::flippertx {

void FlipperTxView::focus() {
    button_startstop.focus();
}

FlipperTxView::FlipperTxView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &button_startstop,
        &field_frequency,
        &tx_view,

    });

    button_startstop.on_select = [this](Button&) {
        if (is_running) {
            is_running = false;
            stop();
        } else {
            is_running = true;
            start();
        }
    };

    update_start_stop(0);
}

void FlipperTxView::generate_packet() {
}

void FlipperTxView::stop() {
    transmitter_model.disable();
    baseband::shutdown();
    button_startstop.set_text(LanguageHelper::currentMessages[LANG_START]);
}

void FlipperTxView::start() {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    transmitter_model.enable();
    button_startstop.set_text(LanguageHelper::currentMessages[LANG_STOP]);
    generate_packet();
}

void FlipperTxView::on_tx_progress(const bool done) {
    if (done) {
        if (is_running) {
            stop();
        }
    }
}

FlipperTxView::~FlipperTxView() {
    is_running = false;
    stop();
}

}  // namespace ui::external_app::flippertx
