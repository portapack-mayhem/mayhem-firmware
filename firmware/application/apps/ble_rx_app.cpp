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

#include "ble_rx_app.hpp"

#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_textentry.hpp"

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

std::string pdu_type_to_string(ADV_PDU_TYPE type)
{
    std::string pduTypeStr = "";

    switch (type) {
        case ADV_IND:
            pduTypeStr += "ADV_IND";
            break;
        case ADV_DIRECT_IND:
            pduTypeStr += "ADV_DIRECT_IND";
            break;
        case ADV_NONCONN_IND:
            pduTypeStr += "ADV_NONCONN_IND";
            break;
        case SCAN_REQ:
            pduTypeStr += "SCAN_REQ";
            break;
        case SCAN_RSP:
            pduTypeStr += "SCAN_RSP";
            break;
        case CONNECT_REQ:
            pduTypeStr += "CONNECT_REQ";
            break;
        case ADV_SCAN_IND:
            pduTypeStr += "ADV_SCAN_IND";
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
            pduTypeStr += "RESERVED";
            break;
        default:
            pduTypeStr += "UNKNOWN";
            break;
    }

    return pduTypeStr;
}

template <>
void RecentEntriesTable<BleRecentEntries>::draw(
    const Entry& entry,
    const Rect& target_rect,
    Painter& painter,
    const Style& style) {
    std::string line{};
    line.reserve(30);

    if (!entry.nameString.empty() && entry.include_name) {
        line = entry.nameString;

        if (line.length() < 17) {
            line += pad_string_with_spaces(17 - line.length());
        } else {
            line = truncate(line, 17);
        }
    } else {
        line = to_string_mac_address(entry.packetData.macAddress, 6, false);
    }

    // Pushing single digit values down right justified.
    std::string hitsStr = to_string_dec_int(entry.numHits);
    int hitsDigits = hitsStr.length();
    uint8_t hits_spacing = 8 - hitsDigits;

    // Pushing single digit values down right justified.
    std::string dbStr = to_string_dec_int(entry.dbValue);
    int dbDigits = dbStr.length();
    uint8_t db_spacing = 5 - dbDigits;

    line += pad_string_with_spaces(hits_spacing) + hitsStr;

    line += pad_string_with_spaces(db_spacing) + dbStr;

    line.resize(target_rect.width() / 8, ' ');
    painter.draw_string(target_rect.location(), style, line);
}

BleRecentEntryDetailView::BleRecentEntryDetailView(NavigationView& nav, const BleRecentEntry& entry)
    : nav_{nav},
      entry_{entry} {
    add_children({&button_done,
                  &button_send,
                  &label_mac_address,
                  &text_mac_address,
                  &label_pdu_type,
                  &text_pdu_type,
                  &labels});

    text_mac_address.set(to_string_mac_address(entry.packetData.macAddress, 6, false));
    text_pdu_type.set(pdu_type_to_string(entry.pduType));

    button_done.on_select = [&nav](const ui::Button&) {
        nav.pop();
    };

    button_send.on_select = [this, &nav](const ui::Button&) {
        auto packetToSend = build_packet();
        nav.set_on_pop([packetToSend, &nav]() {
            nav.replace<BLETxView>(packetToSend);
        });
        nav.pop();
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

    auto field_rect = Rect{rect.left(), rect.top() + 64, rect.width(), 16};

    uint8_t type[total_data_lines];
    uint8_t length[total_data_lines];
    uint8_t data[total_data_lines][40];

    int currentByte = 0;
    int currentPacket = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    switch (entry_.pduType)
    {
        case ADV_IND:
        case ADV_NONCONN_IND:
        case SCAN_RSP:
        case ADV_SCAN_IND:
        {
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
                        if (!data_strings[k].empty()) {
                            field_rect = draw_field(painter, field_rect, s, "", pad_string_with_spaces(5) + data_strings[k]);
                        }
                    }
                }
            }
        }
        break;

        case ADV_DIRECT_IND:
        case SCAN_REQ:
        case CONNECT_REQ:
        default:
        {
            uint8_t type = 0xFF;

            for (currentByte = 0; (currentByte < entry_.packetData.dataLen); currentByte++) {
                    data[0][currentByte] = entry_.packetData.data[currentByte];
            }

            uint8_t number_data_lines = ceil((float)entry_.packetData.dataLen / 10.0);
            uint8_t current_line = 0;
            std::array<std::string, total_data_lines> data_strings{};

            for (j = 0; (j < (number_data_lines * 10)) && (j < entry_.packetData.dataLen); j++) {
                if ((j / 10) != current_line) {
                    current_line++;
                }

                data_strings[current_line] += to_string_hex(data[0][j], 2);
            }

            field_rect = draw_field(painter, field_rect, s, to_string_hex(entry_.packetData.dataLen), to_string_hex(type) + pad_string_with_spaces(3) + data_strings[0]);

            if (number_data_lines > 1) {
                for (k = 1; k < number_data_lines; k++) {
                    if (!data_strings[k].empty()) {
                        field_rect = draw_field(painter, field_rect, s, "", pad_string_with_spaces(5) + data_strings[k]);
                    }
                }
            }
        }
        break;
    }
}

void BleRecentEntryDetailView::set_entry(const BleRecentEntry& entry) {
    entry_ = entry;
    set_dirty();
}

BLETxPacket BleRecentEntryDetailView::build_packet() {
    BLETxPacket bleTxPacket;
    memset(&bleTxPacket, 0, sizeof(BLETxPacket));

    std::string macAddressStr = to_string_mac_address(entry_.packetData.macAddress, 6, true);

    strncpy(bleTxPacket.macAddress, macAddressStr.c_str(), 12);
    strncpy(bleTxPacket.advertisementData, entry_.dataString.c_str(), entry_.packetData.dataLen * 2);
    strncpy(bleTxPacket.packetCount, "50", 3);
    bleTxPacket.packet_count = 50;

    return bleTxPacket;
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
    options_channel.focus();
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
                  &check_name,
                  &label_sort,
                  &options_sort,
                  &button_filter,
                  &button_switch,
                  &recent_entries_view});

    recent_entries_view.on_select = [this](const BleRecentEntry& entry) {
        nav_.push<BleRecentEntryDetailView>(entry);
    };

    button_filter.on_select = [this](Button&) {
        text_prompt(
            nav_,
            filterBuffer,
            64,
            [this](std::string& buffer) {
                on_filter_change(buffer);
            });
    };

    button_switch.on_select = [&nav](Button&) {
        nav.replace<BLETxView>();
    };

    field_frequency.set_step(0);

    check_log.set_value(logging);

    check_log.on_select = [this](Checkbox&, bool v) {
        str_log = "";
        logging = v;

        if (logger && logging)
            logger->append(LOG_ROOT_DIR "/BLELOG_" + to_string_timestamp(rtc_time::now()) + ".TXT");
    };

    check_name.set_value(true);

    check_name.on_select = [this](Checkbox&, bool v) {
        setAllMembersToValue(recent, &BleRecentEntry::include_name, v);
        recent_entries_view.set_dirty();
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        field_frequency.set_value(get_freq_by_channel_number(i));
        channel_number = i;

        baseband::set_btlerx(channel_number);
    };

    options_sort.on_change = [this](size_t, int32_t index) {
        handle_entries_sort(index);
    };

    options_channel.set_selected_index(0, true);
    options_sort.set_selected_index(0, true);

    logger = std::make_unique<BLELogger>();

    // Auto-configure modem for LCR RX (will be removed later)
    baseband::set_btlerx(channel_number);

    receiver_model.enable();
}

void BLERxView::on_data(BlePacketData* packet) {
    std::string str_console = "";

    if (!logging) {
        str_log = "";
    }

    str_console += pdu_type_to_string((ADV_PDU_TYPE)packet->type);

    str_console += " Len:";
    str_console += to_string_dec_uint(packet->size);

    str_console += "\n";

    str_console += "Mac:";
    str_console += to_string_mac_address(packet->macAddress, 6, false);

    str_console += "\n";
    str_console += "Data:";

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        str_console += to_string_hex(packet->data[i], 2);
    }

    str_console += "\n";

    uint64_t macAddressEncoded = copy_mac_address_to_uint64(packet->macAddress);

    // Start of Packet stuffing.
    // Masking off the top 2 bytes to avoid invalid keys.
    auto& entry = ::on_packet(recent, macAddressEncoded & 0xFFFFFFFFFFFF);
    updateEntry(packet, entry, (ADV_PDU_TYPE)packet->type);

    // Add entries if they meet the criteria.
    auto value = filter;
    resetFilteredEntries(recent, [&value](const BleRecentEntry& entry) {
        return (entry.dataString.find(value) == std::string::npos) && (entry.nameString.find(value) == std::string::npos);
    });

    handle_entries_sort(options_sort.selected_index());

    // Log at End of Packet.
    if (logger && logging) {
        logger->log_raw_data(str_console);
    }
}

void BLERxView::on_filter_change(std::string value) {
    // New filter? Reset list from recent entries.
    if (filter != value) {
        resetFilteredEntries(recent, [&value](const BleRecentEntry& entry) {
            return (entry.dataString.find(value) == std::string::npos) && (entry.nameString.find(value) == std::string::npos);
        });
    }

    filter = value;
}

void BLERxView::handle_entries_sort(uint8_t index) {
    switch (index) {
        case 0:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.macAddress; }, true);
            break;
        case 1:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.numHits; }, false);
            break;
        case 2:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.dbValue; }, false);
            break;
        case 3:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.timestamp; }, false);
            break;
        case 4:
            sortEntriesBy(
                recent, [](const BleRecentEntry& entry) { return entry.nameString; }, true);
            break;
        default:
            break;
    }

    recent_entries_view.set_dirty();
}

void BLERxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height - switch_button_height};
    recent_entries_view.set_parent_rect(content_rect);
}

BLERxView::~BLERxView() {
    receiver_model.disable();
    baseband::shutdown();
}

void BLERxView::updateEntry(const BlePacketData* packet, BleRecentEntry& entry, ADV_PDU_TYPE pdu_type) {
    std::string data_string;

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        data_string += to_string_hex(packet->data[i], 2);
    }

    entry.dbValue = packet->max_dB;
    entry.timestamp = to_string_timestamp(rtc_time::now());
    entry.dataString = data_string;

    entry.packetData.type = packet->type;
    entry.packetData.size = packet->size;
    entry.packetData.dataLen = packet->dataLen;

    //Mac Address of sender.
    entry.packetData.macAddress[0] = packet->macAddress[0];
    entry.packetData.macAddress[1] = packet->macAddress[1];
    entry.packetData.macAddress[2] = packet->macAddress[2];
    entry.packetData.macAddress[3] = packet->macAddress[3];
    entry.packetData.macAddress[4] = packet->macAddress[4];
    entry.packetData.macAddress[5] = packet->macAddress[5];

    entry.numHits++;
    entry.pduType = pdu_type;

    //Data section of packet.
    for (int i = 0; i < packet->dataLen; i++) {
        entry.packetData.data[i] = packet->data[i];
    }

    entry.nameString = "";
    entry.include_name = check_name.value();

    //Only parse name for advertisment packets
    if (pdu_type == ADV_IND || pdu_type == ADV_NONCONN_IND || pdu_type == SCAN_RSP || pdu_type == ADV_SCAN_IND)
    {
        uint8_t currentByte = 0;
        uint8_t length = 0;
        uint8_t type = 0;

        bool stringFound = false;

        for (currentByte = 0; (currentByte < entry.packetData.dataLen);) {
            length = entry.packetData.data[currentByte++];
            type = entry.packetData.data[currentByte++];

            // Subtract 1 because type is part of the length.
            for (int i = 0; i < length - 1; i++) {
                if (((type == 0x08) || (type == 0x09)) && !stringFound) {
                    entry.nameString += (char)entry.packetData.data[currentByte];
                }

                currentByte++;
            }

            if (!entry.nameString.empty()) {
                stringFound = true;
            }
        }
    }
}

} /* namespace ui */
