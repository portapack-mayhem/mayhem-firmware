/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 TJ Baginski
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

#include "ble_tx_app.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;

void BLELoggerTx::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

namespace ui {

void BLETxView::focus() {
    field_frequency.focus();
}

bool BLETxView::is_active() const {
    return (bool)is_running;
}

void BLETxView::toggle() {
    if (is_active()) {
        stop(false);
    } else {
        start();
    }
}

void BLETxView::start() {
    stop(false);
    progressbar.set_max(20);
    button_play.set_bitmap(&bitmap_stop);
    baseband::set_btletx(channel_number);
    transmitter_model.enable();
    //console.clear(true);

    is_running = true;
}

void BLETxView::stop(const bool do_loop) {

    if (do_loop) {
        start();
    } else {
        transmitter_model.disable();
        progressbar.set_value(0);
        button_play.set_bitmap(&bitmap_play);
        is_running = false;
    }
}

void BLETxView::on_tx_progress(const uint32_t progress, const bool done) {
    if (done) {
        //console.writeln("Sent Packet :)");
        stop(check_loop.value());
    } else
        progressbar.set_value(progress);
}

BLETxView::BLETxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);

    add_children({
            &button_open,
            &text_filename,
            &text_sample_rate,
            &text_duration,
            &progressbar,
            &field_frequency,
            &tx_view,  // now it handles previous rfgain, rfamp.
            &check_loop,
            &button_play,
            &console});

    field_frequency.set_step(0);

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    logger = std::make_unique<BLELoggerTx>();

   // if (logger && logging)
        //logger->append(LOG_ROOT_DIR "/BLELOGTX_" + to_string_timestamp(rtc_time::now()) + ".TXT");
}

void BLETxView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "";

    if (is_data)
    {
        str_log += to_string_hex(value, 2);
        str_console += to_string_dec_uint(value) + " ";
    }
    else
    {
        logging_done = true;
    }

    if (!logging) {
        str_log = "";
    }

    // Log at End of Packet.
    if (0) {
        logger->log_raw_data(str_log);
        logging_done = false;
        str_log = "";
    }

    console.write(str_console);
}

void BLETxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    console.set_parent_rect(content_rect);
}

BLETxView::~BLETxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

} /* namespace ui */
