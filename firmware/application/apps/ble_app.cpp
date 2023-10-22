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


std::string padStringWithSpaces(int n) 
{
    std::string paddedStr(n, ' ');
    return paddedStr;
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

        // Handle spacing for negitive sign.
        uint8_t db_spacing = entry.dbValue > 0 ? 7 : 6;

        // Pushing single digit values down right justified.
        if (entry.dbValue > 9 || entry.dbValue < -9)
        {
            db_spacing--;
        }

        line += padStringWithSpaces(db_spacing) + to_string_dec_int(entry.dbValue);

        line.resize(target_rect.width() / 8, ' ');
        painter.draw_string(target_rect.location(), style, line);
    }

    BleRecentEntryDetailView::BleRecentEntryDetailView() {
        add_children({
            &button_done,
            &labels
        });

        button_done.on_select = [this](const ui::Button&) {
            if (on_close) {
                on_close();
            }
        };
    }

    BleRecentEntryDetailView::BleRecentEntryDetailView(const BleRecentEntryDetailView& Entry)
        : View() 
    {
        (void)Entry;
    }

    BleRecentEntryDetailView& BleRecentEntryDetailView::operator=(const BleRecentEntryDetailView& Entry) 
    {
        (void)Entry;
        return *this;
    }

    void BleRecentEntryDetailView::update_data() 
    {

    }

    void BleRecentEntryDetailView::focus() 
    {
        button_done.focus();
    }

    Rect BleRecentEntryDetailView::draw_field(
        Painter& painter,
        const Rect& draw_rect,
        const Style& style,
        const std::string& label,
        const std::string& value) 
    {
        const int label_length_max = 4;

        painter.draw_string(Point{draw_rect.left(), draw_rect.top()}, style, label);
        painter.draw_string(Point{draw_rect.left() + (label_length_max + 1) * 8, draw_rect.top()}, style, value);

        return {draw_rect.left(), draw_rect.top() + draw_rect.height(), draw_rect.width(), draw_rect.height()};
    }

    void BleRecentEntryDetailView::paint(Painter& painter) 
    {
        View::paint(painter);

        const auto s = style();
        const auto rect = screen_rect();

        auto field_rect = Rect{rect.left(), rect.top() + 16, rect.width(), 16};

        uint8_t type[5];
        uint8_t length[5];
        uint8_t data[5][40];

        int currentByte = 0;
        int currentPacket = 0;
        int i = 0;
        int j = 0;
        int k = 0;
        
        for (currentByte = 0; (currentByte < entry_.packetData.dataLen) && (currentPacket < 5);)
        {
            length[currentPacket] = entry_.packetData.data[currentByte++];
            type[currentPacket] = entry_.packetData.data[currentByte++];

            // This should never happen, but in here just in case. 
            // Break because we can't trust rest of data.
            // if (length[currentPacket] > entry_.packetData.dataLen)
            // {
            //     break;
            // }

            // Subtract 1 because type is part of the length.
            for (i = 0; i < length[currentPacket] - 1; i++)
            {
                data[currentPacket][i] = entry_.packetData.data[currentByte++];
            }

            currentPacket++;
        }

        for (i = 0; i < currentPacket; i++)
        { 
            uint8_t number_data_lines = ceil((float)(length[i] - 1) / 10.0);
            uint8_t current_line = 0;
            std::array<std::string, 5> data_strings{{""}};

            for (j = 0; (j < (number_data_lines * 10)) && (j < length[i] - 1); j++)
            {
                if ((j / 10) != current_line)
                {
                    current_line++;
                }

                data_strings[current_line] += to_string_hex(data[i][j], 2);
            }

            // Readd the type back to the total length.
            field_rect = draw_field(painter, field_rect, s, to_string_hex(length[i]), to_string_hex(type[i]) + padStringWithSpaces(3) + data_strings[0]);

            if(number_data_lines > 1)
            {
                for (k = 1; k < number_data_lines; k++)
                {
                    if (data_strings[k] != "")
                    {
                        field_rect = draw_field(painter, field_rect, s, "", padStringWithSpaces(5) + data_strings[k]);
                    }
                }           
            }
        }
    }

    void BleRecentEntryDetailView::set_entry(const BleRecentEntry& entry) 
    {
        entry_ = entry;
        set_dirty();
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
                    &recent_entries_view,
                    &recent_entry_detail_view});
                    #endif

        recent_entry_detail_view.hidden(true);

        recent_entries_view.on_select = [this](const BleRecentEntry& entry) 
        {
            on_show_detail(entry);
        };

        recent_entry_detail_view.on_close = [this]() 
        {
            on_show_list();
        };

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


        //Start of Packet stuffing.
        uint64_t macAddressEncoded = 0;

        memcpy(&macAddressEncoded, packet->macAddress, sizeof(uint64_t));

        #if USE_CONSOLE
        str_console += "\n" + to_string_dec_uint(macAddressEncoded) + "\n";
        console.write(str_console); 
        #else
        // Masking off the top 2 bytes to avoid invalid keys.
        auto& entry = ::on_packet(recent, macAddressEncoded & 0xFFFFFFFFFFFF);

        entry.dbValue = packet->max_dB;
        entry.packetData.type = packet->type;
        entry.packetData.size = packet->size;
        entry.packetData.dataLen = packet->dataLen;

        entry.packetData.macAddress[0] = packet->macAddress[0];
        entry.packetData.macAddress[1] = packet->macAddress[1];
        entry.packetData.macAddress[2] = packet->macAddress[2];
        entry.packetData.macAddress[3] = packet->macAddress[3];
        entry.packetData.macAddress[4] = packet->macAddress[4];
        entry.packetData.macAddress[5] = packet->macAddress[5];

        for (int i = 0; i < packet->dataLen; i++)
        {
            entry.packetData.data[i] = packet->data[i];
        }
   
        //entry.update(packet);
        recent_entries_view.set_dirty();
        #endif

        // TODO: Crude hack, should be a more formal listener arrangement...
        if (entry.key() == recent_entry_detail_view.entry().key()) 
        {
            recent_entry_detail_view.set_entry(entry);
        }

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
        recent_entry_detail_view.set_parent_rect(content_rect);
    }

    BLERxView::~BLERxView() 
    {
        audio::output::stop();
        receiver_model.disable();
        baseband::shutdown();
    }

    void BLERxView::on_show_list() 
    {
        recent_entries_view.hidden(false);
        recent_entry_detail_view.hidden(true);
        recent_entries_view.focus();
    }

    void BLERxView::on_show_detail(const BleRecentEntry& entry) 
    {
        recent_entries_view.hidden(true);
        recent_entry_detail_view.hidden(false);
        recent_entry_detail_view.set_entry(entry);
        recent_entry_detail_view.focus();
    }

    // BleRecentEntry
    // void BleRecentEntry::update(const BlePacketData * packet) 
    // {
       
    // }

} /* namespace ui */
