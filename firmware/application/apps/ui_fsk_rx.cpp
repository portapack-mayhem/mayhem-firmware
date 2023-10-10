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

#include "ui_fsk_rx.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "utility.hpp"

using namespace portapack;
namespace pmem = portapack::persistent_memory;

void FskRxLogger::log_raw_data(const std::string& data, const uint32_t frequency) 
{
    std::string entry = "Raw: F:" + to_string_dec_uint(frequency) + "Hz";

    // // Raw hex dump of all the codewords
    // for (size_t c = 0; c < 16; c++)
    //     entry += to_string_hex(packet[c], 8) + " ";

    log_file.write_entry(data + entry);
}

void FskRxLogger::log_decoded(Timestamp timestamp, const std::string& text) 
{
    log_file.write_entry(timestamp, text);
}

namespace ui 
{
    FskRxAppView::FskRxAppView(NavigationView& nav)
        : nav_{nav} 
    {
        baseband::run_image(portapack::spi_flash::image_tag_fskrx);

        add_children(
            {&rssi,
            &audio,
            &field_rf_amp,
            &field_lna,
            &field_vga,
            &field_frequency,
            &field_squelch,
            &field_volume,
            &console});

        // No app settings, use fallbacks from pmem.
        // if (!app_settings_.loaded()) 
        // {
        // }

        // if (!app_settings_.radio_loaded())
        // {
            field_frequency.set_value(initial_target_frequency);
        //}

        logger.append(LOG_ROOT_DIR "/FSKRX.TXT");

        field_squelch.set_value(receiver_model.squelch_level());
        field_squelch.on_change = [this](int32_t v) {
            receiver_model.set_squelch_level(v);
        };

        refresh_ui();
        audio::output::start();
        receiver_model.enable();

        baseband::set_fsk(4500, receiver_model.squelch_level());
    }

    void FskRxAppView::focus() 
    {
        field_frequency.focus();
    }

    FskRxAppView::~FskRxAppView() 
    {
        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }

    void FskRxAppView::refresh_ui()
    {

    }

    void FskRxAppView::handle_decoded(Timestamp timestamp, const std::string& prefix) 
    {
        if (logging()) 
        {
            logger.log_decoded(timestamp, prefix);
        }
    }

    void FskRxAppView::on_packet(uint32_t value, bool is_data)
    {
        if(is_data)
        {
            console.write("[0x" + to_string_hex(value, 2) + "] ");
        }
    }

} /* namespace ui */
