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
    // baseband::run_image(portapack::spi_flash::image_tag_wefaxrx);
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &txt_status,
                  &labels,
                  &options_lpm,
                  &options_ioc});

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

    options_lpm.set_selected_index(lpm_index, false);
    options_ioc.set_selected_index(ioc_index, true);

    field_frequency.set_step(10);
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    receiver_model.enable();
    txt_status.set("Waiting for signal.");
}

WeFaxRxView::~WeFaxRxView() {
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
}

void WeFaxRxView::on_settings_changed() {
    baseband::set_wefax_config(options_lpm.selected_index_value(), options_ioc.selected_index_value());
}

void WeFaxRxView::on_status(WeFaxRxStatusDataMessage msg) {
    std::string tmp = to_string_dec_int(msg.freq) + " " + to_string_dec_int(msg.freqavg) + " " + to_string_dec_int(msg.freqmin) + " " + to_string_dec_int(msg.freqmax);
    txt_status.set(tmp);
}

void WeFaxRxView::on_image(WeFaxRxImageDataMessage msg) {
    line_num++;
    if (line_num >= 320 - 4 * 16) line_num = 0;
    ui::Color line_buffer[240];
    for (uint16_t i = 0; i < 480; i += 2) {
        uint8_t px = (msg.image[i] + msg.image[i + 1]) / 2;
        line_buffer[i / 2] = {px, px, px};
    }
    portapack::display.render_line({0, line_num + 4 * 16}, 240, line_buffer);
}

}  // namespace ui::external_app::wefax_rx
