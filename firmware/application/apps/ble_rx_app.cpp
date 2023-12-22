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
#include "ble_rx_app.hpp"
#include "ui_modemsetup.hpp"

#include "modems.hpp"
#include "audio.hpp"
#include "io_file.hpp"
#include "rtc_time.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_fileman.hpp"
#include "ui_textentry.hpp"

using namespace portapack;
using namespace modems;
namespace fs = std::filesystem;

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

void reverse_byte_array(uint8_t* arr, int length) {
    int start = 0;
    int end = length - 1;

    while (start < end) {
        // Swap elements at start and end
        uint8_t temp = arr[start];
        arr[start] = arr[end];
        arr[end] = temp;

        // Move the indices towards the center
        start++;
        end--;
    }
}

namespace ui {

std::string pdu_type_to_string(ADV_PDU_TYPE type) {
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
                  &button_save,
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

    button_save.on_select = [this, &nav](const ui::Button&) {
        auto packetToSave = build_packet();

        packetFileBuffer = "";
        text_prompt(
            nav,
            packetFileBuffer,
            64,
            [this, packetToSave](std::string& buffer) {
                on_save_file(buffer, packetToSave);
            });
    };
}

void BleRecentEntryDetailView::on_save_file(const std::string value, BLETxPacket packetToSave) {
    ensure_directory(packet_save_path);
    auto folder = packet_save_path.parent_path();
    auto ext = packet_save_path.extension();
    auto new_path = folder / value + ext;

    saveFile(new_path, packetToSave);
}

bool BleRecentEntryDetailView::saveFile(const std::filesystem::path& path, BLETxPacket packetToSave) {
    File f;
    auto error = f.create(path);
    if (error)
        return false;

    std::string macAddressStr = packetToSave.macAddress;
    std::string advertisementDataStr = packetToSave.advertisementData;
    std::string packetCountStr = packetToSave.packetCount;

    std::string packetString = macAddressStr + ' ' + advertisementDataStr + ' ' + packetCountStr;

    f.write(packetString.c_str(), packetString.length());

    return true;
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

    switch (entry_.pduType) {
        case ADV_IND:
        case ADV_NONCONN_IND:
        case SCAN_RSP:
        case ADV_SCAN_IND: {
            ADV_PDU_PAYLOAD_TYPE_0_2_4_6* advertiseData = (ADV_PDU_PAYLOAD_TYPE_0_2_4_6*)entry_.packetData.data;

            for (currentByte = 0; (currentByte < entry_.packetData.dataLen) && (currentPacket < total_data_lines);) {
                length[currentPacket] = advertiseData->Data[currentByte++];
                type[currentPacket] = advertiseData->Data[currentByte++];

                // Subtract 1 because type is part of the length.
                for (i = 0; i < length[currentPacket] - 1; i++) {
                    data[currentPacket][i] = advertiseData->Data[currentByte++];
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
        } break;

        case ADV_DIRECT_IND:
        case SCAN_REQ: {
            ADV_PDU_PAYLOAD_TYPE_1_3* directed_mac_data = (ADV_PDU_PAYLOAD_TYPE_1_3*)entry_.packetData.data;
            uint8_t type = 0xFF;
            field_rect = draw_field(painter, field_rect, s, to_string_hex(entry_.packetData.dataLen), to_string_hex(type) + pad_string_with_spaces(3) + to_string_mac_address(directed_mac_data->A1, 6, false));
        } break;

        case CONNECT_REQ:
        default: {
            uint8_t type = 0xFF;

            // TODO: Display Connect Request Information. For right now just printing full hex data.
            // This struct will eventually be used to break apart containing data of Connect Request.
            // ADV_PDU_PAYLOAD_TYPE_5 * connect_req = (ADV_PDU_PAYLOAD_TYPE_5 *)entry_.packetData.data;

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
        } break;
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

void BLERxView::file_error() {
    nav_.display_modal("Error", "File read error.");
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
                  &button_find,
                  &check_name,
                  &label_sort,
                  &options_sort,
                  &label_found,
                  &text_found_count,
                  &check_serial_log,
                  &button_filter,
                  &button_save_list,
                  &button_clear_list,
                  &button_switch,
                  &recent_entries_view});

    recent_entries_view.on_select = [this](const BleRecentEntry& entry) {
        nav_.push<BleRecentEntryDetailView>(entry);
    };

    check_serial_log.on_select = [this](Checkbox&, bool v) {
        serial_logging = v;
        if (v) {
            usb_serial_thread = std::make_unique<UsbSerialThread>();
        } else {
            usb_serial_thread.reset();
        }
    };
    check_serial_log.set_value(serial_logging);

    ensure_directory(find_packet_path);
    ensure_directory(log_packets_path);
    ensure_directory(packet_save_path);

    filterBuffer = filter;

    button_filter.on_select = [this](Button&) {
        text_prompt(
            nav_,
            filterBuffer,
            64,
            [this](std::string& buffer) {
                on_filter_change(buffer);
            });
    };

    logger = std::make_unique<BLELogger>();

    check_log.on_select = [this](Checkbox&, bool v) {
        str_log = "";
        logging = v;

        if (logger && logging)
            logger->append(
                "BLERX/Logs"
                "/BLELOG_" +
                to_string_timestamp(rtc_time::now()) + ".TXT");
    };
    check_log.set_value(logging);

    button_save_list.on_select = [this, &nav](const ui::Button&) {
        listFileBuffer = "";
        text_prompt(
            nav,
            listFileBuffer,
            64,
            [this](std::string& buffer) {
                on_save_file(buffer);
            });
    };

    button_clear_list.on_select = [this](Button&) {
        recent.clear();
    };

    button_switch.on_select = [&nav](Button&) {
        nav.replace<BLETxView>();
    };

    field_frequency.set_step(0);

    check_name.set_value(name_enable);

    check_name.on_select = [this](Checkbox&, bool v) {
        name_enable = v;
        setAllMembersToValue(recent, &BleRecentEntry::include_name, v);
        recent_entries_view.set_dirty();
    };

    options_channel.on_change = [this](size_t index, int32_t v) {
        channel_index = (uint8_t)index;

        // If we selected Auto don't do anything and Auto will handle changing.
        if (v == 40) {
            auto_channel = true;
            return;
        } else {
            auto_channel = false;
        }

        field_frequency.set_value(get_freq_by_channel_number(v));
        channel_number = v;

        baseband::set_btlerx(channel_number);
    };

    options_sort.on_change = [this](size_t index, int32_t v) {
        sort_index = (uint8_t)index;
        handle_entries_sort(v);
    };

    options_channel.set_selected_index(channel_index, true);
    options_sort.set_selected_index(sort_index, true);

    button_find.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);

            // nav_.set_on_pop([this]() { button_play.focus(); });
        };
    };

    // Auto-configure modem for LCR RX (will be removed later)
    baseband::set_btlerx(channel_number);

    receiver_model.enable();
}

std::string BLERxView::build_line_str(BleRecentEntry entry) {
    std::string macAddressStr = to_string_mac_address(entry.packetData.macAddress, 6, false) + ",";
    std::string timestameStr = entry.timestamp + ",";
    std::string nameStr = entry.nameString + ",";
    std::string pduStr = pdu_type_to_string(entry.pduType) + ",";
    std::string dataStr = "0x" + entry.dataString + ",";
    std::string hitsStr = to_string_dec_int(entry.numHits) + ",";
    std::string dbStr = to_string_dec_int(entry.dbValue) + ",";
    std::string channelStr = to_string_dec_int(entry.channelNumber) + ",";

    std::string lineStr = timestameStr + macAddressStr + nameStr + pduStr + dataStr + hitsStr + dbStr + channelStr;
    lineStr += pad_string_with_spaces(maxLineLength - lineStr.length());

    return lineStr;
}

void BLERxView::on_save_file(const std::string value) {
    auto folder = packet_save_path.parent_path();
    auto ext = packet_save_path.extension();
    auto new_path = folder / value + ext;

    saveFile(new_path);
}

bool BLERxView::saveFile(const std::filesystem::path& path) {
    // Check to see if file was previously saved.
    bool file_existed = file_exists(path);

    // Attempt to open, if it can't be opened. Create new.
    auto src = std::make_unique<File>();
    auto error = src->open(path, false, true);

    if (error) {
        return false;
    }

    for (const auto& entry : recent) {
        tempList.emplace_back(entry);
    }

    if (!file_existed) {
        src->write_line(headerStr.c_str());

        auto it = tempList.begin();

        while (it != tempList.end()) {
            BleRecentEntry entry = (BleRecentEntry)*it;
            src->write_line(build_line_str(entry).c_str());
            it++;
        }
    } else {
        // Check file for macAddressStr before adding.
        char currentLine[maxLineLength];
        uint64_t startPos = headerStr.length();
        uint64_t bytesRead = 0;
        uint64_t bytePos = 0;

        File::Size currentSize = src->size();

        auto dst = std::make_unique<File>();
        const std::filesystem::path tempFilePath = path + "~";
        auto error = dst->open(tempFilePath, false, true);

        if (error) {
            return false;
        }

        dst->write_line(headerStr.c_str());

        src->seek(startPos);

        // Look for ones found and rewrite.
        do {
            memset(currentLine, 0, maxLineLength);

            bytesRead = readUntil(*src, currentLine, currentSize, '\n');

            if (!bytesRead) {
                break;
            }

            bytePos += bytesRead;

            std::string lineStr = "";
            std::string macAddressStr = "";
            BleRecentEntry foundEntry;

            char* token;
            token = strtok(currentLine, ",");

            while (token != NULL) {
                auto it = tempList.begin();

                while (it != tempList.end()) {
                    BleRecentEntry& entry = reinterpret_cast<BleRecentEntry&>(*it);

                    macAddressStr = to_string_mac_address(entry.packetData.macAddress, 6, false);

                    if (strstr(token, macAddressStr.c_str()) != NULL) {
                        entry.entryFound = true;
                        foundEntry = entry;
                        break;
                    }

                    it++;
                }

                if (foundEntry.entryFound) {
                    break;
                }

                token = strtok(NULL, ",");
            }

            if (foundEntry.entryFound) {
                dst->write_line(build_line_str(foundEntry).c_str());
            }

        } while (bytePos <= currentSize);

        // Write the ones not found.
        auto it = tempList.begin();

        while (it != tempList.end()) {
            BleRecentEntry entry = (BleRecentEntry)*it;

            if (!entry.entryFound) {
                dst->write_line(build_line_str(entry).c_str());
            }

            it++;
        }

        // Close files before renaming/deleting.
        src.reset();
        dst.reset();

        // Delete original and overwrite with temp file.
        delete_file(path);
        rename_file(tempFilePath, path);
    }

    tempList.clear();

    return true;
}

void BLERxView::on_data(BlePacketData* packet) {
    if (!logging) {
        str_log = "";
    }

    str_console += pdu_type_to_string((ADV_PDU_TYPE)packet->type);
    str_console += " Len:";
    str_console += to_string_dec_uint(packet->size);
    str_console += " Mac:";
    str_console += to_string_mac_address(packet->macAddress, 6, false);
    str_console += " Data:";

    int i;

    for (i = 0; i < packet->dataLen; i++) {
        str_console += to_string_hex(packet->data[i], 2);
    }

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
        logger->log_raw_data(str_console + "\r\n");
    }

    if (serial_logging) {
        usb_serial_thread->serial_str = str_console + "\r\n";
        usb_serial_thread->str_ready = true;
    }
    str_console = "";

    if (!searchList.empty()) {
        auto it = searchList.begin();

        while (it != searchList.end()) {
            std::string searchStr = (std::string)*it;

            if (entry.dataString.find(searchStr) != std::string::npos) {
                searchList.erase(it);
                found_count++;
                break;
            }

            it++;
        }

        text_found_count.set(to_string_dec_uint(found_count) + "/" + to_string_dec_uint(total_count));
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

void BLERxView::on_file_changed(const std::filesystem::path& new_file_path) {
    file_path = fs::path(u"/") + new_file_path;
    found_count = 0;
    total_count = 0;
    searchList.clear();

    {  // Get the size of the data file.
        File data_file;
        auto error = data_file.open(file_path, true, false);
        if (error) {
            file_error();
            file_path = "";
            return;
        }

        uint64_t bytesRead = 0;
        uint64_t bytePos = 0;
        char currentLine[maxLineLength];

        do {
            memset(currentLine, 0, maxLineLength);

            bytesRead = readUntil(data_file, currentLine, maxLineLength, '\n');

            // Remove return if found.
            if (currentLine[strlen(currentLine)] == '\r') {
                currentLine[strlen(currentLine)] = '\0';
            }

            if (!bytesRead) {
                break;
            }

            searchList.push_back(currentLine);
            total_count++;

            bytePos += bytesRead;

        } while (bytePos <= data_file.size());
    }
}

// called each 1/60th of second, so 6 = 100ms
void BLERxView::on_timer() {
    if (++timer_count == timer_period) {
        timer_count = 0;

        if (auto_channel) {
            int min = 37;
            int max = 39;

            int randomChannel = min + std::rand() % (max - min + 1);

            field_frequency.set_value(get_freq_by_channel_number(randomChannel));
            baseband::set_btlerx(randomChannel);
        }
    }
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

    // Mac Address of sender.
    entry.packetData.macAddress[0] = packet->macAddress[0];
    entry.packetData.macAddress[1] = packet->macAddress[1];
    entry.packetData.macAddress[2] = packet->macAddress[2];
    entry.packetData.macAddress[3] = packet->macAddress[3];
    entry.packetData.macAddress[4] = packet->macAddress[4];
    entry.packetData.macAddress[5] = packet->macAddress[5];

    entry.numHits++;
    entry.pduType = pdu_type;
    entry.channelNumber = channel_number;

    // Parse Data Section into buffer to be interpretted later.
    for (int i = 0; i < packet->dataLen; i++) {
        entry.packetData.data[i] = packet->data[i];
    }

    entry.include_name = check_name.value();

    // Only parse name for advertisment packets and empty name entries
    if ((pdu_type == ADV_IND || pdu_type == ADV_NONCONN_IND || pdu_type == SCAN_RSP || pdu_type == ADV_SCAN_IND) && entry.nameString.empty()) {
        ADV_PDU_PAYLOAD_TYPE_0_2_4_6* advertiseData = (ADV_PDU_PAYLOAD_TYPE_0_2_4_6*)entry.packetData.data;

        uint8_t currentByte = 0;
        uint8_t length = 0;
        uint8_t type = 0;

        std::string decoded_data;
        for (currentByte = 0; (currentByte < entry.packetData.dataLen);) {
            length = advertiseData->Data[currentByte++];
            type = advertiseData->Data[currentByte++];

            // Subtract 1 because type is part of the length.
            for (int i = 0; i < length - 1; i++) {
                if (type == 0x08 || type == 0x09) {
                    decoded_data += (char)advertiseData->Data[currentByte];
                }
                currentByte++;
            }
            if (!decoded_data.empty()) {
                entry.nameString = std::move(decoded_data);
                break;
            }
        }
    } else if (pdu_type == ADV_DIRECT_IND || pdu_type == SCAN_REQ) {
        ADV_PDU_PAYLOAD_TYPE_1_3* directed_mac_data = (ADV_PDU_PAYLOAD_TYPE_1_3*)entry.packetData.data;
        reverse_byte_array(directed_mac_data->A1, 6);
    }
}

} /* namespace ui */
