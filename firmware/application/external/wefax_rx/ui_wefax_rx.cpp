/*
 * Copyright (C) 2024 HTotoo
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or  modify
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

#include "ui_wefax_rx.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "lcd_ili9341.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::wefax_rx {

void WeFaxRxView::focus() {
    field_frequency.focus();
}

WeFaxRxView::WeFaxRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  //&txt_status,  //not yet in use in baseband. but keep it
                  &labels,
                  &options_lpm,
                  &options_ioc,
                  &button_ss});

    options_lpm.on_change = [this](size_t index, int32_t v) {
        lpm_index = (uint8_t)index;
        (void)v;
        on_settings_changed();
    };
    options_ioc.on_change = [this](size_t index, int32_t v) {
        ioc_index = (uint8_t)index;
        (void)v;
        on_settings_changed();
    };

    field_frequency.set_step(100);
    audio::output::start();
    receiver_model.enable();
    txt_status.set("Waiting for signal.");

    button_ss.on_select = [this](Button&) {
        if (bmp.is_loaded()) {
            bmp.close();
            button_ss.set_text("Start");
            return;
        }
        bmp.create("/bmptest.bmp", WEFAX_PX_SIZE, 1);  // todo add a new name for it!
        button_ss.set_text("Stop");
    };

    options_lpm.set_selected_index(lpm_index, false);
    options_ioc.set_selected_index(ioc_index, true);
}

WeFaxRxView::~WeFaxRxView() {
    stopping = true;
    bmp.close();
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void WeFaxRxView::on_settings_changed() {
    baseband::set_wefax_config(options_lpm.selected_index_value(), options_ioc.selected_index_value());
}

void WeFaxRxView::on_status(WeFaxRxStatusDataMessage msg) {
    (void)msg;
    // not yet in use.
    // std::string tmp = "";
    // txt_status.set(tmp);
}

// this stores and displays the image. keep it as simple as you can. a bit more complexity will kill the sync
void WeFaxRxView::on_image(WeFaxRxImageDataMessage msg) {
    if ((line_num) >= 320 - 4 * 16) line_num = 0;  // for draw reset

    for (uint16_t i = 0; i < msg.cnt; i += 1) {
        Color pxl = {msg.image[i], msg.image[i], msg.image[i]};
        bmp.write_next_px(pxl);
        line_in_part++;
        if (line_in_part == WEFAX_PX_SIZE) {
            line_in_part = 0;
            line_num++;
            bmp.expand_y_delta(1);
        }

        uint16_t xpos = line_in_part / (WEFAX_PX_SIZE / 240);
        if (xpos > 240) xpos = 240;
        line_buffer[xpos] = pxl;
        if ((line_in_part == 0)) {
            portapack::display.render_line({0, line_num + 4 * 16}, 240, line_buffer);
        }
    }
}

}  // namespace ui::external_app::wefax_rx
