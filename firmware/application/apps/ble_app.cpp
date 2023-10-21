/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2023 TJ
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

#define USE_CONSOLE 0

using namespace portapack;
using namespace modems;

void BLELogger::log_raw_data(const std::string& data) 
{
    log_file.write_entry(data);
}

namespace ui 
{
    template <>
    void RecentEntriesTable<BleRecentEntries>::draw(
        const Entry& entry,
        const Rect& target_rect,
        Painter& painter,
        const Style& style) {
        std::string line = to_string_hex(entry.macAddress & 0xFF, 2);

        line += ":" + to_string_hex((entry.macAddress >> 8) & 0xFF, 2);
        line += ":" + to_string_hex((entry.macAddress >> 16) & 0xFF, 2);
        line += ":" + to_string_hex((entry.macAddress >> 24) & 0xFF, 2);
        line += ":" + to_string_hex((entry.macAddress >> 32) & 0xFF, 2);
        line += ":" + to_string_hex((entry.macAddress >> 40), 2);

        line += "        " + to_string_dec_int(entry.dbValue);

        line.resize(target_rect.width() / 8, ' ');
        painter.draw_string(target_rect.location(), style, line);
    }

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
                    &options_region,
                    &field_frequency,
                    &check_log,
                    #if USE_CONSOLE
                    &console});
                    #else
                    &recent_entries_view});
                    #endif

        //field_frequency.set_value(get_freq_by_channel_number(37));
        field_frequency.set_step(2000000);

        check_log.set_value(logging);

        check_log.on_select = [this](Checkbox&, bool v) 
        {
            str_log = "";
            logging = v;
        };

        options_region.on_change = [this](size_t, int32_t i) 
        {
            if (i == 0) 
            {
                field_frequency.set_value(get_freq_by_channel_number(37));
                channel_number = 37;
            } 
            else if (i == 1) 
            {
                field_frequency.set_value(get_freq_by_channel_number(38));
                channel_number = 38;
            } 
            else if (i == 2) 
            {
                field_frequency.set_value(get_freq_by_channel_number(39));
                channel_number = 39;
            }

            baseband::set_btle(channel_number);
        };

        options_region.set_selected_index(0, true);

        // recent_entries_view.on_select = [this](const BleRecentEntry& entry) {
        //     on_show_detail(entry);
        // };

        // recent_entry_detail_view.on_close = [this]() {
        //     on_show_list();
        // };

        logger = std::make_unique<BLELogger>();

        if (logger)
            logger->append(LOG_ROOT_DIR "/BLELOG_" + to_string_timestamp(rtc_time::now()) + ".TXT");

        // Auto-configure modem for LCR RX (will be removed later)
        baseband::set_btle(channel_number);

        receiver_model.enable();
    }

    void BLERxView::on_data(BlePacketData * packet)
    {
        std::string str_console = "";

        if (!logging)
        {
            str_log = "";
        }
        
        switch ((ADV_PDU_TYPE)packet->type) 
        {
            case ADV_IND:
                str_console += "ADV_IND";
                break;
            case ADV_DIRECT_IND:
                str_console += "ADV_DIRECT_IND";
                break;
            case ADV_NONCONN_IND:
                str_console += "ADV_NONCONN_IND";
                break;
            case SCAN_REQ:
                str_console += "SCAN_REQ";
                break;
            case SCAN_RSP:
                str_console += "SCAN_RSP";
                break;
            case CONNECT_REQ:
                str_console += "CONNECT_REQ";
                break;
            case ADV_SCAN_IND:
                str_console += "ADV_SCAN_IND";
                break;
            case RESERVED0:
            case RESERVED1:
            case RESERVED2:
            case RESERVED3:
            case RESERVED4:
            case RESERVED5:
            case RESERVED6:
            case RESERVED7:
            case RESERVED8:
                str_console += "RESERVED";
                break;
            default:
                str_console += "UNKNOWN";
                break;
        }

        //str_console += to_string_dec_uint(value);

        str_console += " Len: ";
        str_console += to_string_dec_uint(packet->size);

        str_console += "\n";

        str_console += "Mac";
        str_console += ":" + to_string_hex(packet->macAddress[0], 2);
        str_console += ":" + to_string_hex(packet->macAddress[1], 2);
        str_console += ":" + to_string_hex(packet->macAddress[2], 2);
        str_console += ":" + to_string_hex(packet->macAddress[3], 2);
        str_console += ":" + to_string_hex(packet->macAddress[4], 2);
        str_console += ":" + to_string_hex(packet->macAddress[5], 2);

        str_console += "\n";
        str_console += "Data:";

        int i;

        for (i = 0; i < packet->dataLen; i++)
        {
            str_console += " " + to_string_hex(packet->data[i], 2);
        }

        str_console += "\n";

        uint64_t macAddressEncoded = 0;

        memcpy(&macAddressEncoded, packet->macAddress, sizeof(uint64_t));

        #if USE_CONSOLE
        str_console += "\n" + to_string_dec_uint(macAddressEncoded) + "\n";
        console.write(str_console); 
        #else
        // Masking off the top 2 bytes to avoid invalid keys.
        auto& entry = ::on_packet(recent, macAddressEncoded & 0xFFFFFFFFFFFF);
        entry.dbValue = packet->max_dB;
        //entry.update(packet);
        recent_entries_view.set_dirty();
        #endif

        //Log at End of Packet.
        if (logger && logging) 
        {
            logger->log_raw_data(str_console);
        }
    }

    void BLERxView::set_parent_rect(const Rect new_parent_rect) 
    {
        View::set_parent_rect(new_parent_rect);
        const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
        recent_entries_view.set_parent_rect(content_rect);
    }

    BLERxView::~BLERxView() 
    {
        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }

    // BleRecentEntry
    // void BleRecentEntry::update(const BlePacketData * packet) 
    // {
       
    // }

} /* namespace ui */
