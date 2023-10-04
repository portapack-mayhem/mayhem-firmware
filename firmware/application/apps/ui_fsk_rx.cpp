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
            &image_status,
            &text_packet_count,
            &widget_baud,
            &widget_bits,
            &widget_frames,
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
        baseband::set_fsk();
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
            console.write("[0x" + to_string_hex(value, 4) + "] ");
        }
    }

    // void BaudIndicator::paint(Painter& painter) 
    // {
    //     auto p = screen_pos();
    //     char top = '-';
    //     char bot = '-';

    //     if (rate_ > 0) {
    //         auto r = rate_ / 100;
    //         top = (r / 10) + '0';
    //         bot = (r % 10) + '0';
    //     }

    //     painter.draw_char(p, Styles::white_small, top);
    //     painter.draw_char({p.x(), p.y() + 8}, Styles::white_small, bot);
    // }

    // void BitsIndicator::paint(Painter&) 
    // {
    //     auto p = screen_pos();
    //     for (size_t i = 0; i < sizeof(bits_) * 8; ++i) {
    //         auto is_set = ((bits_ >> i) & 0x1) == 1;

    //         int x = p.x() + (i / height);
    //         int y = p.y() + (i % height);
    //         display.draw_pixel({x, y}, is_set ? Color::white() : Color::black());
    //     }
    // }

    // void FrameIndicator::paint(Painter& painter) 
    // {
    //     auto p = screen_pos();
    //     painter.draw_rectangle({p, {2, height}}, has_sync_ ? Color::green() : Color::grey());

    //     for (size_t i = 0; i < height; ++i) {
    //         auto p2 = p + Point{2, 15 - (int)i};
    //         painter.draw_hline(p2, 2, i < frame_count_ ? Color::white() : Color::black());
    //     }
    // }
} /* namespace ui */
