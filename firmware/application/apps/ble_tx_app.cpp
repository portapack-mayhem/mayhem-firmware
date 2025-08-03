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

#include "ble_tx_app.hpp"
#include "ble_rx_app.hpp"

#include "ui_fileman.hpp"
#include "ui_modemsetup.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "io_file.hpp"
#include "modems.hpp"
#include "portapack_persistent_memory.hpp"
#include "rtc_time.hpp"
#include "string_format.hpp"
#include "file_path.hpp"
#include "usb_serial_asyncmsg.hpp"

using namespace portapack;
using namespace modems;
namespace fs = std::filesystem;

void BLELoggerTx::log_raw_data(const std::string& data) {
    log_file.write_entry(data);
}

bool hasValidHexPairs(const std::string& str, int totalPairs) {
    int validPairs = 0;

    for (int i = 0; i < totalPairs * 2; i += 2) {
        char c1 = str[i];
        char c2 = str[i + 1];

        if (!(std::isxdigit(c1) && std::isxdigit(c2))) {
            return false;  // Return false if any pair is invalid.
        }

        validPairs++;
    }

    return (validPairs == totalPairs);
}

std::vector<std::string> splitIntoStrings(const char* input) {
    std::vector<std::string> result;
    int length = std::strlen(input);
    int start = 0;

    while (start < length) {
        int remaining = length - start;
        int chunkSize = (remaining > 29) ? 29 : remaining;
        result.push_back(std::string(input + start, chunkSize));
        start += chunkSize;
    }

    return result;
}

uint32_t stringToUint32(const std::string& str) {
    size_t pos = 0;
    uint32_t result = 0;

    while (pos < str.size() && std::isdigit(str[pos])) {
        int digit = str[pos] - '0';

        // Check for overflow before adding the next digit
        if (result > (UINT32_MAX - digit) / 10) {
            return 0;
        }

        result = result * 10 + digit;
        pos++;
    }

    return result;
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

namespace ui {

PKT_TYPE get_pkt_type_from_string(const std::string& type_str) {
    if (type_str == "RAW") {
        return PKT_TYPE_RAW;
    } else if (type_str == "DISCOVERY") {
        return PKT_TYPE_DISCOVERY;
    } else if (type_str == "IBEACON") {
        return PKT_TYPE_IBEACON;
    } else if (type_str == "ADV_IND") {
        return PKT_TYPE_ADV_IND;
    } else if (type_str == "ADV_DIRECT_IND") {
        return PKT_TYPE_ADV_DIRECT_IND;
    } else if (type_str == "ADV_NONCONN_IND") {
        return PKT_TYPE_ADV_NONCONN_IND;
    } else if (type_str == "ADV_SCAN_IND") {
        return PKT_TYPE_ADV_SCAN_IND;
    } else if (type_str == "SCAN_REQ") {
        return PKT_TYPE_SCAN_REQ;
    } else if (type_str == "SCAN_RSP") {
        return PKT_TYPE_SCAN_RSP;
    } else if (type_str == "CONNECT_REQ") {
        return PKT_TYPE_CONNECT_REQ;
    }
    return PKT_TYPE_INVALID_TYPE;
}

void BLETxView::focus() {
    button_open.focus();
}

bool BLETxView::is_active() const {
    return (bool)is_running;
}

void BLETxView::file_error() {
    nav_.display_modal("Error", "File read error.");
}

bool BLETxView::saveFile(const std::filesystem::path& path) {
    File f;
    auto error = f.create(path);
    if (error)
        return false;

    for (uint32_t i = 0; i < num_packets; i++) {
        std::string macAddressStr = packets[i].macAddress;
        std::string advertisementDataStr = packets[i].advertisementData;
        std::string packetCountStr = packets[i].packetCount;

        std::string packetString = macAddressStr + ' ' + advertisementDataStr + ' ' + packetCountStr;

        // Are we on the last line?
        if (i != num_packets - 1) {
            packetString += '\n';
        }

        f.write(packetString.c_str(), packetString.length());
    }

    return true;
}

void BLETxView::toggle() {
    if (is_active()) {
        stop();
    } else {
        start();
    }
}

void BLETxView::send_packet() {
    // Generate new random Mac Address.
    transmitter_model.enable();
    generateRandomMacAddress(randomMac);

    char advertisementData[63] = {0};
    strcpy(advertisementData, packets[current_packet].advertisementData);

    // TODO: Make this a checkbox.
    if (!markedBytes.empty()) {
        for (size_t i = 0; i < strlen(advertisementData); i++) {
            bool found = false;

            auto it = std::find(markedBytes.begin(), markedBytes.end(), i);

            if (it != markedBytes.end()) {
                found = true;
            }

            if (found) {
                uint8_t hexDigit;
                switch (marked_data_sequence.selected_index_value()) {
                    case 0:
                        hexDigit = marked_counter++;
                        break;
                    case 1:
                        hexDigit = marked_counter--;
                        break;
                    case 2: {
                        uint8_t min = 0x00;
                        uint8_t max = 0x0F;

                        hexDigit = min + std::rand() % (max - min + 1);
                    } break;
                    default:
                        hexDigit = 0;
                        break;
                }

                advertisementData[i] = uint_to_char(hexDigit, 16);

                // Bounding to Hex.
                if (marked_counter == 16) {
                    marked_counter = 0;
                } else if (marked_counter == 255) {
                    marked_counter = 15;
                }
            }
        }
    }

    progressbar.set_value(packets[current_packet].packet_count - packet_counter);
    text_packets_sent.set(to_string_dec_uint(packet_counter));

    baseband::set_btletx(channel_number, random_mac ? randomMac : packets[current_packet].macAddress, advertisementData, packets[current_packet].pduType);

    packetDone = false;
}

void BLETxView::start() {
    if (file_path.empty()) {
        file_error();
        check_loop.set_value(false);
        return;
    }

    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);
    transmitter_model.enable();

    button_play.set_bitmap(&bitmap_stop);
    is_running = true;

    send_packet();
}

void BLETxView::stop() {
    transmitter_model.disable();
    baseband::shutdown();

    progressbar.set_value(0);
    button_play.set_bitmap(&bitmap_play);
    check_loop.set_value(false);

    update_current_packet(packets[0], 0);

    is_running = false;
}

// called each 1/60th of second, so 6 = 100ms
void BLETxView::on_timer() {
    if (++timer_count == timer_period) {
        timer_count = 0;

        if (is_active() && packetDone) {
            send_packet();
        }
    }
}

void BLETxView::on_tx_progress(const bool done, uint32_t progress) {
    if (done) {
        if (is_active()) {
            transmitter_model.disable();
            if (auto_channel) {
                switch (advCount) {
                    case 0:
                        channel_number = 37;
                        break;
                    case 1:
                        channel_number = 38;
                        break;
                    case 2:
                        channel_number = 39;
                        break;
                }

                field_frequency.set_value(get_freq_by_channel_number(channel_number));

                if (advCount == 3) {
                    channel_number = 37;
                    packet_counter--;
                    packetDone = true;
                    advCount = 0;
                } else {
                    send_packet();
                    advCount++;
                }
            } else {
                packet_counter--;
                packetDone = true;
            }
        }

        // Reached end of current packet repeats.
        if (packet_counter == 0) {
            // Done sending all packets.
            if (current_packet == (num_packets - 1)) {
                current_packet = 0;

                // If looping, restart from beginning.
                if (check_loop.value()) {
                    update_current_packet(packets[current_packet], current_packet);
                } else {
                    stop();
                }
            } else {
                current_packet++;
                update_current_packet(packets[current_packet], current_packet);
            }
        }
    }
}

BLETxView::BLETxView(NavigationView& nav)
    : nav_{nav} {
    add_children({&dataEditView,
                  &button_open,
                  &text_filename,
                  &progressbar,
                  &check_rand_mac,
                  &field_frequency,
                  &tx_view,  // now it handles previous rfgain, rfamp.
                  &check_loop,
                  &button_play,
                  &label_speed,
                  &options_speed,
                  &options_channel,
                  &label_marked_data,
                  &marked_data_sequence,
                  &label_packet_index,
                  &text_packet_index,
                  &label_packets_sent,
                  &text_packets_sent,
                  &label_mac_address,
                  &text_mac_address,
                  &label_data_packet,
                  &button_clear_marked,
                  &button_save_packet,
                  &button_switch});

    field_frequency.set_step(0);

    ensure_directory(packet_save_path);

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        // If we selected Auto don't do anything and Auto will handle changing.
        if (i == 40) {
            auto_channel = true;
            return;
        } else {
            auto_channel = false;
        }

        field_frequency.set_value(get_freq_by_channel_number(i));
        channel_number = i;
    };

    options_speed.on_change = [this](size_t, int32_t i) {
        timer_period = i;
        timer_count = 0;
    };

    options_speed.set_selected_index(0);
    options_channel.set_selected_index(3);

    check_rand_mac.set_value(false);
    check_rand_mac.on_select = [this](Checkbox&, bool v) {
        random_mac = v;
    };

    button_open.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);

            nav_.set_on_pop([this]() { button_play.focus(); });
        };
    };

    button_save_packet.on_select = [this, &nav](Button&) {
        packetFileBuffer = "";
        text_prompt(
            nav,
            packetFileBuffer,
            64,
            ENTER_KEYBOARD_MODE_ALPHA,
            [this](std::string& buffer) {
                on_save_file(buffer);
            });
    };

    button_switch.on_select = [&nav](Button&) {
        nav.replace<BLERxView>();
    };

    dataEditView.on_select = [this] {
        // Save last selected cursor.
        cursor_pos.line = dataEditView.line();
        cursor_pos.col = dataEditView.col();

        // Reject setting newline at index 29.
        if (cursor_pos.col != 29) {
            uint16_t dataBytePos = (cursor_pos.line * 29) + cursor_pos.col;

            auto it = std::find(markedBytes.begin(), markedBytes.end(), dataBytePos);

            if (it != markedBytes.end()) {
                markedBytes.erase(it);
            } else {
                markedBytes.push_back(dataBytePos);
            }

            dataEditView.cursor_mark_selected();
        }
    };

    dataEditView.on_change = [this](uint8_t value) {
        // Reject setting newline at index 29.
        if (cursor_pos.col != 29) {
            uint16_t dataBytePos = (cursor_pos.line * 29) + cursor_pos.col;

            packets[current_packet].advertisementData[dataBytePos] = uint_to_char(value, 16);

            update_current_packet(packets[current_packet], current_packet);
        }
    };

    dataEditView.on_cursor_moved = [this]() {
        // Save last selected cursor.
        cursor_pos.line = dataEditView.line();
        cursor_pos.col = dataEditView.col();
    };

    button_clear_marked.on_select = [this](Button&) {
        marked_counter = 0;
        markedBytes.clear();
        dataEditView.cursor_clear_marked();
    };
}

BLETxView::BLETxView(
    NavigationView& nav,
    BLETxPacket packet)
    : BLETxView(nav) {
    packets[0] = packet;

    update_current_packet(packets[0], 0);

    num_packets = 1;
    file_override = true;
}

void BLETxView::on_file_changed(const fs::path& new_file_path) {
    file_path = new_file_path;
    num_packets = 0;

    {  // Get the size of the data file.
        File data_file;
        auto error = data_file.open(file_path);
        if (error) {
            file_error();
            file_path = "";
            return;
        }

        do {
            readUntil(data_file, packets[num_packets].macAddress, mac_address_size_str, ' ');
            readUntil(data_file, packets[num_packets].advertisementData, max_packet_size_str, ' ');
            readUntil(data_file, packets[num_packets].packetType, max_packet_type_str, ' ');
            readUntil(data_file, packets[num_packets].packetCount, max_packet_repeat_str, '\n');

            uint64_t macAddressSize = strlen(packets[num_packets].macAddress);
            uint64_t advertisementDataSize = strlen(packets[num_packets].advertisementData);
            uint64_t packetCountSize = strlen(packets[num_packets].packetCount);
            uint64_t packetTypeSize = strlen(packets[num_packets].packetType);

            packets[num_packets].packet_count = stringToUint32(packets[num_packets].packetCount);
            packets[num_packets].pduType = (PKT_TYPE)get_pkt_type_from_string(packets[num_packets].packetType);

            // Verify Data.
            if ((macAddressSize == mac_address_size_str) &&
                (advertisementDataSize < max_packet_size_str) &&
                (packetCountSize < max_packet_repeat_str) &&
                hasValidHexPairs(packets[num_packets].macAddress, macAddressSize / 2) &&
                hasValidHexPairs(packets[num_packets].advertisementData, advertisementDataSize / 2) &&
                (packets[num_packets].packet_count >= 1) && (packets[num_packets].packet_count < max_packet_repeat_count) &&
                (packetTypeSize <= max_packet_type_str)) {
                text_filename.set(truncate(file_path.filename().string(), 12));

            } else {
                // Did not find any packets.
                if (num_packets == 0) {
                    file_path = "";
                    return;
                }

                break;
            }

            num_packets++;

        } while (num_packets < max_num_packets);

        update_current_packet(packets[0], 0);

        data_file.close();
    }
}

void BLETxView::on_save_file(const std::string value) {
    auto folder = packet_save_path.parent_path();
    auto ext = packet_save_path.extension();
    auto new_path = folder / value + ext;

    saveFile(new_path);
}

void BLETxView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "";

    if (is_data) {
        str_console += (char)(value);
    }
}

void BLETxView::update_current_packet(BLETxPacket packet, uint32_t currentIndex) {
    std::string formattedMacAddress = to_string_formatted_mac_address(packet.macAddress);

    std::vector<std::string> strings = splitIntoStrings(packet.advertisementData);

    text_packet_index.set(to_string_dec_uint(current_packet));

    text_packets_sent.set(to_string_dec_uint(packet.packet_count));

    text_mac_address.set(formattedMacAddress);

    progressbar.set_max(packet.packet_count);

    packet_counter = packet.packet_count;
    current_packet = currentIndex;

    dataFile.create(dataTempFilePath);

    for (const std::string& str : strings) {
        dataFile.write(str.c_str(), str.size());
        dataFile.write("\n", 1);
    }

    dataFile.close();

    auto result = FileWrapper::open(dataTempFilePath);

    if (!result)
        return;

    dataFileWrapper = *std::move(result);

    dataEditView.set_font_zoom(true);
    dataEditView.set_file(*dataFileWrapper);
    dataEditView.redraw(true, true);
    dataEditView.cursor_set(cursor_pos.line, cursor_pos.col);
}

void BLETxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height - switch_button_height};
    dataEditView.set_parent_rect(content_rect);
}

BLETxView::~BLETxView() {
    delete_file(dataTempFilePath);
    transmitter_model.disable();
    baseband::shutdown();
}

} /* namespace ui */
