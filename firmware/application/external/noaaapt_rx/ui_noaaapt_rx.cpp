/*
 * Copyright (C) 2025 HTotoo
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

/*
TODOS LATER:
 - add load data from wav file (maybe to a separate app, not this)
 - AGC?!?
 - Auto doppler correction
 - fix and enable sync detection
 - auto start / stop bmp save on each image
*/

#include "ui_noaaapt_rx.hpp"

#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "lcd_ili9341.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::noaaapt_rx {

void NoaaAptRxView::focus() {
    field_frequency.focus();
}

NoaaAptRxView::NoaaAptRxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({&rssi,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &txt_status,
                  //&check_wav,  // enable this or the record view, but not both. yet it has some error, says "Invalid object" a lot. so disabled it
                  //&record_view,  //
                  &button_ss});

    record_view.set_filename_date_frequency(true);
    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };
    record_view.set_sampling_rate(12000);

    field_frequency.set_step(100);
    field_frequency.on_edit_shown = [this]() {
        paused = true;
    };
    field_frequency.on_edit_hidden = [this]() {
        paused = false;
    };
    audio::set_rate(audio::Rate::Hz_12000);
    audio::output::start();
    receiver_model.set_hidden_offset(NOAAAPT_FREQ_OFFSET);
    receiver_model.set_baseband_bandwidth(1750000);
    receiver_model.set_sampling_rate(3072000);
    receiver_model.enable();
    txt_status.set("Waiting for signal.");

    button_ss.on_select = [this](Button&) {
        if (bmp.is_loaded()) {
            bmp.close();
            button_ss.set_text(LanguageHelper::currentMessages[LANG_START]);
            if (check_wav.value()) {
                record_view.stop();
            }
            return;
        }
        if (check_wav.value()) {
            record_view.start();
        }
        ensure_directory("/BMP");
        bmp.create("/BMP/noaa_" + to_string_timestamp(rtc_time::now()) + ".bmp", NOAAAPT_PX_SIZE, 1);

        button_ss.set_text(LanguageHelper::currentMessages[LANG_STOP]);
    };
    on_settings_changed();
}

NoaaAptRxView::~NoaaAptRxView() {
    stopping = true;
    receiver_model.set_hidden_offset(0);
    bmp.close();
    receiver_model.disable();
    baseband::shutdown();
    audio::output::stop();
    record_view.stop();
}

void NoaaAptRxView::on_settings_changed() {
    baseband::set_noaaapt_config();
}

void NoaaAptRxView::on_status(NoaaAptRxStatusDataMessage msg) {
    (void)msg;
    std::string tmp = "";
    if (msg.state == 0) {
        tmp = "Waiting for signal.";
    } else if (msg.state == 1) {
        tmp = "Synced.";
    } else if (msg.state == 2) {
        tmp = "Image arriving.";
    }
    txt_status.set(tmp);
}

// this stores and displays the image. keep it as simple as you can. a bit more complexity will kill the sync
void NoaaAptRxView::on_image(NoaaAptRxImageDataMessage msg) {
    if ((line_num) >= UI_POS_HEIGHT_REMAINING(NOAA_IMG_START_ROW)) line_num = 0;  // for draw reset

    for (uint16_t i = 0; i < msg.cnt; i += 1) {
        Color pxl = {msg.image[i], msg.image[i], msg.image[i]};
        bmp.write_next_px(pxl);
        line_in_part++;
        if (line_in_part == NOAAAPT_PX_SIZE) {
            line_in_part = 0;
            line_num++;
            bmp.expand_y_delta(1);
        }

        uint16_t xpos = line_in_part / (NOAAAPT_PX_SIZE / 240);
        if (xpos >= 240) xpos = 239;
        line_buffer[xpos] = pxl;
        if ((line_in_part == 0)) {
            portapack::display.render_line({0, line_num + NOAA_IMG_START_ROW * 16}, 240, line_buffer);
        }
    }
}

}  // namespace ui::external_app::noaaapt_rx
