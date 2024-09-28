/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_random.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "file_path.hpp"

using namespace portapack;
using namespace modems;
using namespace ui;

namespace ui::external_app::random {

void RandomLogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

void RandomView::focus() {
    field_frequency.focus();
}

RandomView::RandomView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &field_volume,
                  &field_frequency,
                  &check_log,
                  &text_debug,
                  &button_modem_setup,
                  &console});

    // Auto-configure modem for LCR RX (TODO remove)
    field_frequency.set_value(467225500);
    auto def_bell202 = &modem_defs[0];
    persistent_memory::set_modem_baudrate(def_bell202->baudrate);
    serial_format_t serial_format;
    serial_format.data_bits = 7;
    serial_format.parity = EVEN;
    serial_format.stop_bits = 1;
    serial_format.bit_order = LSB_FIRST;
    persistent_memory::set_serial_format(serial_format);

    field_frequency.set_step(100);

    check_log.set_value(logging);
    check_log.on_select = [this](Checkbox&, bool v) {
        logging = v;
    };

    button_modem_setup.on_select = [&nav](Button&) {
        nav.push<ModemSetupView>();
    };

    logger = std::make_unique<RandomLogger>();
    if (logger)
        logger->append(logs_dir / u"AFSK.TXT");

    // Auto-configure modem for LCR RX (will be removed later)
    baseband::set_afsk(persistent_memory::modem_baudrate(), 8, 0, false);

    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();

    receiver_model.enable();
}

void RandomView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "\x1B";
    std::string str_byte = "";

    if (is_data) {
        // Colorize differently after message splits
        str_console += (char)((console_color & 3) + 9);

        // Directly print the received value without decoding
        str_console += "[" + to_string_hex(value, 2) + "]";
        str_byte += "[" + to_string_hex(value, 2) + "]";

        console.write(str_console);

        if (logger && logging) str_log += str_byte;

        if ((value != 0x7F) && (prev_value == 0x7F)) {
            // Message split
            console.writeln("");
            console_color++;

            if (logger && logging) {
                logger->log_raw_data(str_log);
                str_log = "";
            }
        }
        prev_value = value;
    } else {
        // Baudrate estimation
        text_debug.set("Baudrate estimation: ~" + to_string_dec_uint(value));
    }
}

void RandomView::on_freqchg(int64_t freq) {
    field_frequency.set_value(freq);
}

RandomView::~RandomView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::random
