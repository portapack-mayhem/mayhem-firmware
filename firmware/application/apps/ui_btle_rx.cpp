/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2020 Shao
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

#include "ui_btle_rx.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;

namespace ui {

void BTLERxView::focus() {
    field_frequency.focus();
}

BTLERxView::BTLERxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_btle_rx);

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_frequency,
                  &button_modem_setup,
                  &console});

    // Auto-configure modem for LCR RX (TODO: remove later)
    auto def_bell202 = &modem_defs[0];
    persistent_memory::set_modem_baudrate(def_bell202->baudrate);
    serial_format_t serial_format;
    serial_format.data_bits = 7;
    serial_format.parity = EVEN;
    serial_format.stop_bits = 1;
    serial_format.bit_order = LSB_FIRST;
    persistent_memory::set_serial_format(serial_format);

    field_frequency.set_step(100);

    button_modem_setup.on_select = [&nav](Button&) {
        nav.push<ModemSetupView>();
    };

    // Auto-configure modem for LCR RX (will be removed later)
    baseband::set_btle(persistent_memory::modem_baudrate(), 8, 0, false);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    receiver_model.enable();
}

void BTLERxView::on_data(uint32_t value, bool is_data) {
    // std::string str_console = "\x1B";
    std::string str_console = "";
    if (is_data) {
        // Colorize differently after message splits
        // str_console += (char)((console_color & 3) + 9);

        // value &= 0xFF;											// ABCDEFGH
        // value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4);	// EFGHABCD
        // value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);	// GHEFCDAB
        // value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);	// HGFEDCBA
        // value &= 0x7F;											// Ignore parity, which is the MSB now

        // if ((value >= 32) && (value < 127)) {
        //	str_console += (char)value;							// Printable
        // }

        // str_console += (char)'A';
        // str_console += (char)value;
        // str_console += "[" + to_string_hex(value, 2) + "]";
        str_console += ":" + to_string_hex(value, 2);
        console.write(str_console);

        /*if ((value != 0x7F) && (prev_value == 0x7F)) {
                        // Message split
                        console.writeln("");
                        console_color++;


                }*/
        // prev_value = value;
    } else {
        // Baudrate estimation
        // text_debug.set("~" + to_string_dec_uint(value));
        if (value == 'A') {
            console.write("mac");
        } else if (value == 'B') {
            console.writeln("");
        }
        // console.writeln("");
    }
}

BTLERxView::~BTLERxView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

} /* namespace ui */
