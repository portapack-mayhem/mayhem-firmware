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

using namespace portapack;
using namespace modems;

void BLELogger::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

std::string pad_string_with_spaces(int snakes) {
    std::string paddedStr(snakes, ' ');
    return paddedStr;
}

uint64_t copy_mac_address_to_uint64(const uint8_t* macAddress) {
    uint64_t result = 0;

    // Copy each byte of the MAC address to the corresponding byte in the uint64_t.
    for (int i = 0; i < 6; ++i) {
        result |= static_cast<uint64_t>(macAddress[i]) << ((5 - i) * 8);
    }

    return result;
}

namespace ui {
template <>
void RecentEntriesTable<BleRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line = to_string_mac_address(entry.packetData.macAddress, 6);

    // Handle spacing for negative sign.
    uint8_t db_spacing = entry.dbValue > 0 ? 7 : 6;

    // Pushing single digit values down right justified.
    if (entry.dbValue > 9 || entry.dbValue < -9) {
        db_spacing--;
    }

    line += pad_string_with_spaces(db_spacing) + to_string_dec_int(entry.dbValue);

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

BleRecentEntryDetailView::BleRecentEntryDetailView(NavigationView& nav, const BleRecentEntry& entry)
    : nav_{nav},
      entry_{entry} {
    add_children({&button_done,
                  &labels});

    button_done.on_select = [this](const ui::Button&) {
        nav_.pop();
    };
}

void BleRecentEntryDetailView::update_data() {
}

void BleRecentEntryDetailView::focus() {
    button_done.focus();
}

Rect BleRecentEntryDetailView::draw_field(
    Painter& painter,
    const Rect& draw_rect,
    const Style& style,
    const std::string& label,
    const std::string& value) {
    const int label_length_max = 4;

    painter.draw_string(Point{draw_rect.left(), draw_rect.top()}, style, label);
    painter.draw_string(Point{draw_rect.left() + (label_length_max + 1) * 8, draw_rect.top()}, style, value);

    return {draw_rect.left(), draw_rect.top() + draw_rect.height(), draw_rect.width(), draw_rect.height()};
}

void BleRecentEntryDetailView::paint(Painter& painter) {
    View::paint(painter);

    const auto s = style();
    const auto rect = screen_rect();

    auto field_rect = Rect{rect.left(), rect.top() + 16, rect.width(), 16};

    uint8_t type[total_data_lines];
    uint8_t length[total_data_lines];
    uint8_t data[total_data_lines][40];

    int currentByte = 0;
    int currentPacket = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    for (currentByte = 0; (currentByte < entry_.packetData.dataLen) && (currentPacket < total_data_lines);) {
        length[currentPacket] = entry_.packetData.data[currentByte++];
        type[currentPacket] = entry_.packetData.data[currentByte++];

        // Subtract 1 because type is part of the length.
        for (i = 0; i < length[currentPacket] - 1; i++) {
            data[currentPacket][i] = entry_.packetData.data[currentByte++];
        }

        currentPacket++;
    }

    for (i = 0; i < currentPacket; i++) {
        uint8_t number_data_lines = ceil((float)(length[i] - 1) / 10.0);
        uint8_t current_line = 0;
        std::array<std::string, total_data_lines> data_strings{};

        for (j = 0; (j < (number_data_lines * 10)) && (j < length[i] - 1); j++) {
            if ((j / 10) != current_line) {
                current_line++;
            }

            data_strings[current_line] += to_string_hex(data[i][j], 2);
        }

        // Read the type back to the total length.
        field_rect = draw_field(painter, field_rect, s, to_string_hex(length[i]), to_string_hex(type[i]) + pad_string_with_spaces(3) + data_strings[0]);

        if (number_data_lines > 1) {
            for (k = 1; k < number_data_lines; k++) {
                if (data_strings[k].empty()) {
                    field_rect = draw_field(painter, field_rect, s, "", pad_string_with_spaces(5) + data_strings[k]);
                }
            }
        }
    }
}

void BleRecentEntryDetailView::set_entry(const BleRecentEntry& entry) {
    entry_ = entry;
    set_dirty();
}

static std::uint64_t get_freq_by_channel_number(uint8_t channel_number) {
    uint64_t freq_hz;

    switch (channel_number) {
        case 37:
            freq_hz = 2'402'000'000ull;
            break;
        case 38:
            freq_hz = 2'426'000'000ull;
            break;
        case 39:
            freq_hz = 2'480'000'000ull;
            break;
        case 0 ... 10:
            freq_hz = 2'404'000'000ull + channel_number * 2'000'000ull;
            break;
        case 11 ... 36:
            freq_hz = 2'428'000'000ull + (channel_number - 11) * 2'000'000ull;
            break;
        default:
            freq_hz = UINT64_MAX;
    }

    return freq_hz;
}

void BLERxView::focus() {
    field_frequency.focus();
}

BLERxView::BLERxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_btle_rx);

    add_children({&rssi,
                  &channel,
                  &field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &options_channel,
                  &field_frequency,
                  &check_log,
                  &label_sort,
                  &options_sort,
                  &recent_entries_view,
                  &recent_entry_detail_view});

    recent_entry_detail_view.hidden(true);

    recent_entries_view.on_select = [this](const BleRecentEntry& entry) {
        nav_.push<BleRecentEntryDetailView>(entry);
    };

    field_frequency.set_step(0);

    check_log.set_value(logging);

    check_log.on_select = [this](Checkbox&, bool v) {
        str_log = "";
        logging = v;
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        field_frequency.set_value(get_freq_by_channel_number(i));
        channel_number = i;

        baseband::set_btle(channel_number);
    };

    options_sort.on_change = [this](size_t, int32_t i) {
        switch (i) {
            case 0:
                sortEntriesBy(
                    recent, [](const BleRecentEntry& entry) { return entry.macAddress; }, true);
                break;
            case 1:
                sortEntriesBy(
                    recent, [](const BleRecentEntry& entry) { return entry.dbValue; }, true);
                break;
            case 2:
                sortEntriesBy(
                    recent, [](const BleRecentEntry& entry) { return entry.timestamp; }, false);
                break;
            default:
                break;
        }

        recent_entries_view.set_dirty();
    };

    options_channel.set_selected_index(0, true);
    options_sort.set_selected_index(0, true);

    logger = std::make_unique<BLELogger>();

    if (logger)
        logger->append(LOG_ROOT_DIR "/BLELOG_" + to_string_timestamp(rtc_time::now()) + ".TXT");

    // Auto-configure modem for LCR RX (will be removed later)
    baseband::set_btle(channel_number);

    receiver_model.enable();
}

void BLERxView::on_data(BlePacketData* packet) {
    std::string str_console = "";

    if (!logging) {
        str_log = "";
    }

    switch ((ADV_PDU_TYPE)packet->type) {
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

    // str_console += to_string_dec_uint(value);

    str_console += " Len:";
    str_console += to_string_dec_uint(packet->size);

    str_console += "\n";

    str_console += "Mac:";
    str_console += to_string_mac_address(packet->macAddress, 6);

    str_console += "\n";
    str_console += "Data:";

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        str_console += " " + to_string_hex(packet->data[i], 2);
    }

    str_console += "\n";

    uint64_t macAddressEncoded = copy_mac_address_to_uint64(packet->macAddress);

    // Start of Packet stuffing.
    // uint64_t macAddressEncoded = 0;

    // memcpy(&macAddressEncoded, packet->macAddress, sizeof(uint64_t));

    // Masking off the top 2 bytes to avoid invalid keys.
    auto& entry = ::on_packet(recent, macAddressEncoded & 0xFFFFFFFFFFFF);

    entry.dbValue = packet->max_dB;
    entry.timestamp = to_string_timestamp(rtc_time::now());

    entry.packetData.type = packet->type;
    entry.packetData.size = packet->size;
    entry.packetData.dataLen = packet->dataLen;

    entry.packetData.macAddress[0] = packet->macAddress[0];
    entry.packetData.macAddress[1] = packet->macAddress[1];
    entry.packetData.macAddress[2] = packet->macAddress[2];
    entry.packetData.macAddress[3] = packet->macAddress[3];
    entry.packetData.macAddress[4] = packet->macAddress[4];
    entry.packetData.macAddress[5] = packet->macAddress[5];

    for (int i = 0; i < packet->dataLen; i++) {
        entry.packetData.data[i] = packet->data[i];
    }

    // entry.update(packet);

    switch (options_sort.selected_index()) {
        case 0:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.macAddress; }, true);
            break;
        case 1:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.dbValue; }, true);
            break;
        case 2:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.timestamp; }, false);
            break;
        default:
            break;
    }

    recent_entries_view.set_dirty();

    // TODO: Crude hack, should be a more formal listener arrangement...
    if (entry.key() == recent_entry_detail_view.entry().key()) {
        recent_entry_detail_view.set_entry(entry);
    }

    // Log at End of Packet.
    if (logger && logging) {
        logger->log_raw_data(str_console);
    }
}

void BLERxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    recent_entries_view.set_parent_rect(content_rect);
    recent_entry_detail_view.set_parent_rect(content_rect);
}

BLERxView::~BLERxView() {
    receiver_model.disable();
    baseband::shutdown();
}

// BleRecentEntry
// void BleRecentEntry::update(const BlePacketData * packet)
// {

// }

} /* namespace ui */
