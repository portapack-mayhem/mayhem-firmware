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

#include "ui_fileman.hpp"
#include "ui_modemsetup.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "io_file.hpp"
#include "modems.hpp"
#include "portapack_persistent_memory.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"

using namespace portapack;
using namespace modems;
namespace fs = std::filesystem;

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

void BLETxView::file_error() {
    nav_.display_modal("Error", "File read error.");
}

void BLETxView::toggle() {
    if (is_active()) {
        stop();
    } else {
        start();
    }
}

void BLETxView::start() {

    if (!is_running)
    {
        File data_file;

        auto error = data_file.open(file_path);
        if (error) {
            file_error();
            check_loop.set_value(false);
            return;
        }

        if ((strlen(advertisementData) > 62) || (strlen(macAddress) != 12))
        {
            file_error();
            check_loop.set_value(false);
            return;
        }
        else
        {
            progressbar.set_max(20);
            button_play.set_bitmap(&bitmap_stop);
            transmitter_model.enable();

            is_running = true;
        }
    }

    if (is_running)
    {
        baseband::set_btletx(channel_number, macAddress, advertisementData);

        if ((packet_count % 10) == 0) {
            text_packets_sent.set(to_string_dec_uint(packet_count));
        }

        packet_count++;
    }
}

void BLETxView::stop() {
    transmitter_model.disable();
    progressbar.set_value(0);
    button_play.set_bitmap(&bitmap_play);
    is_running = false;
}

void BLETxView::on_tx_progress(const uint32_t progress, const bool done) {
    repeatLoop = check_loop.value();

    if (done) {
        if (repeatLoop) {
            if ((timer_count % timer_period) == 0) {
                progressbar.set_value(0);
                start();
            }
        } else {
            packet_count = 0;
            stop();
        }

        timer_count++;
    } else
        progressbar.set_value(progress);
}

BLETxView::BLETxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);

    add_children({&button_open,
                  &text_filename,
                  &text_sample_rate,
                  &text_duration,
                  &progressbar,
                  &field_frequency,
                  &tx_view,  // now it handles previous rfgain, rfamp.
                  &check_loop,
                  &button_play,
                  &label_speed,
                  &options_speed,
                  &label_packets_sent,
                  &text_packets_sent,
                  &label_mac_address,
                  &text_mac_address,
                  &console});

    field_frequency.set_step(0);

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    options_speed.on_change = [this](size_t, int32_t i) {
        timer_period = i;
    };

    options_speed.set_selected_index(0);

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);
        };
    };
}

void BLETxView::on_file_changed(const fs::path& new_file_path) {
    file_path = fs::path(u"/") + new_file_path;

    {  // Get the size of the data file.
        File data_file;
        auto error = data_file.open(file_path);
        if (error) {
            file_error();
            return;
        }

        // Read Mac Address.
        data_file.read(macAddress, 12);
        
        uint8_t spaceChar;
        // Skip space.
        data_file.read(&spaceChar, 1);

        // Read Advertisement Data.
        data_file.read(advertisementData, 62);

        if ((strlen(advertisementData) < 62) && (strlen(macAddress) == 12))
        {
            char macAddressFormated[18] = {
                macAddress[0], macAddress[1], ':',
                macAddress[2], macAddress[3], ':',
                macAddress[4], macAddress[5], ':',
                macAddress[6], macAddress[7], ':',
                macAddress[8], macAddress[9], ':',
                macAddress[10], macAddress[11]
            };

            text_mac_address.set(macAddressFormated);
            console.clear(true);
            console.writeln(macAddress);
            console.writeln(advertisementData);
            text_filename.set(truncate(file_path.filename().string(), 12));
        }
        else
        {
            file_error();
        }
    }
}

void BLETxView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "";

    if (is_data) {
        str_console += (char)(value);
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
