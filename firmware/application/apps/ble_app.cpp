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

#include "ble_app.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;
using namespace modems;

void BLELogger::log_raw_data(const std::string& data) 
{
    log_file.write_entry(data);
}

namespace ui 
{
    static std::uint64_t get_freq_by_channel_number(uint8_t channel_number) 
    {
        uint64_t freq_hz;

        switch (channel_number) 
        {
            case 37:
                freq_hz = 2402000000ull;
                break;
            case 38:
                freq_hz = 2426000000ull;
                break;
            case 39:
                freq_hz = 2480000000ull;
                break;
            case 0 ... 10:
                freq_hz = 2404000000ull + channel_number * 2000000ull;
                break;
            case 11 ... 36:
                freq_hz = 2428000000ull + (channel_number - 11) * 2000000ull;
                break;
            default:
                freq_hz = 0xffffffffffffffff;
        }

        return freq_hz;
    }

    void BLERxView::focus() 
    {
        field_frequency.focus();
    }

    BLERxView::BLERxView(NavigationView& nav)
    : nav_{nav} 
    {
        baseband::run_image(portapack::spi_flash::image_tag_btle_rx);

        add_children({&rssi,
                    &channel,
                    &field_rf_amp,
                    &field_lna,
                    &field_vga,
                    &field_frequency,
                    &check_log,
                    &text_debug,
                    &button_modem_setup,
                    &console});

        field_frequency.set_value(get_freq_by_channel_number(37));

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

        check_log.on_select = [this](Checkbox&, bool v) 
        {
            logging = v;
        };

        button_modem_setup.on_select = [&nav](Button&) 
        {
            nav.push<ModemSetupView>();
        };

        logger = std::make_unique<BLELogger>();

        if (logger)
            logger->append(LOG_ROOT_DIR "/BLELOG.TXT");

        button_modem_setup.on_select = [&nav](Button&) {
            nav.push<ModemSetupView>();
        };

        // Auto-configure modem for LCR RX (will be removed later)
        baseband::set_btle(persistent_memory::modem_baudrate(), 8, 0, false);

        audio::set_rate(audio::Rate::Hz_24000);
        audio::output::start();

        receiver_model.enable();
    }

    void BLERxView::on_data(uint32_t value, bool is_data) 
    {
        std::string str_console = "";
        if (is_data) 
        {
            str_console += (char)value;
            //str_console += "[" + to_string_hex(value, 2) + "] ";  // Not printable
            console.write(str_console);
        } 
        else 
        {
            if (value == 'A') 
            {
                console.write("mac");
            } 
            else if (value == 'B') 
            {
                console.writeln("");
            }
        }
    }

    BLERxView::~BLERxView() 
    {
        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }

} /* namespace ui */
