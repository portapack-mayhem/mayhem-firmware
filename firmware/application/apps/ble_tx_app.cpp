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
        int chunkSize = (remaining > 30) ? 30 : remaining;
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

void readUntil(File& file, char* result, std::size_t maxBufferSize, char delimiter) {
    std::size_t bytesRead = 0;

    while (true) {
        char ch;
        File::Result<File::Size> readResult = file.read(&ch, 1);

        if (readResult.is_ok() && readResult.value() > 0) {
            if (ch == delimiter) {
                // Found a space character, stop reading
                break;
            } else if (bytesRead < maxBufferSize) {
                // Append the character to the result if there's space
                result[bytesRead++] = ch;
            } else {
                // Buffer is full, break to prevent overflow
                break;
            }
        } else {
            break;  // End of file or error
        }
    }

    // Null-terminate the result string
    result[bytesRead] = '\0';
}

void generateRandomMacAddress(char* macAddress) {
    const char hexDigits[] = "0123456789ABCDEF";

    // Generate 12 random hexadecimal characters
    for (int i = 0; i < 12; i++) {
        int randomIndex = rand() % 16;
        macAddress[i] = hexDigits[randomIndex];
    }
    macAddress[12] = '\0';  // Null-terminate the string
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

void BLETxView::focus() {
    button_open.focus();
}

bool BLETxView::is_active() const {
    return (bool)is_running;
}

void BLETxView::file_error() {
    nav_.display_modal("Error", "File read error.");
}

void BLETxView::toggle() {
    if (is_active()) {
        stop();
    } else {
        start();
    }
}

void BLETxView::start() {
    // Generate new random Mac Address.
    generateRandomMacAddress(randomMac);

    if (!is_active()) {
        // Check if file is present before continuing.
        File data_file;

        auto error = data_file.open(file_path);
        if (error && !file_override) {
            file_error();
            check_loop.set_value(false);
            return;
        } else {
            // Send first or single packet.
            packet_counter = packets[current_packet].packet_count;
            progressbar.set_max(packets[current_packet].packet_count);
            button_play.set_bitmap(&bitmap_stop);

            baseband::set_btletx(channel_number, random_mac ? randomMac : packets[current_packet].macAddress, packets[current_packet].advertisementData, pduType);
            transmitter_model.enable();

            is_running = true;
        }
    } else {
        // Send next packet.
        baseband::set_btletx(channel_number, random_mac ? randomMac : packets[current_packet].macAddress, packets[current_packet].advertisementData, pduType);
    }

    if ((packet_counter % 10) == 0) {
        text_packets_sent.set(to_string_dec_uint(packet_counter));
    }

    packet_counter--;

    progressbar.set_value(packets[current_packet].packet_count - packet_counter);
}

void BLETxView::stop() {
    transmitter_model.disable();
    progressbar.set_value(0);
    button_play.set_bitmap(&bitmap_play);
    check_loop.set_value(false);

    current_packet = 0;
    text_packets_sent.set(to_string_dec_uint(packets[0].packet_count));
    packet_counter = packets[0].packet_count;
    update_packet_display(packets[0]);

    is_running = false;
}

void BLETxView::on_tx_progress(const bool done) {
    if (done) {
        if (is_active()) {
            // Reached end of current packet repeats.
            if (packet_counter == 0) {
                // Done sending all packets.
                if (current_packet == (num_packets - 1)) {
                    // If looping, restart from beginning.
                    if (check_loop.value()) {
                        current_packet = 0;
                        packet_counter = packets[current_packet].packet_count;
                        update_packet_display(packets[current_packet]);
                    } else {
                        stop();
                    }
                } else {
                    current_packet++;
                    packet_counter = packets[current_packet].packet_count;
                    update_packet_display(packets[current_packet]);
                }
            } else {
                if ((timer_count % timer_period) == 0) {
                    start();
                }
            }

            timer_count++;
        }
    }
}

BLETxView::BLETxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);

    add_children({&button_open,
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
                  &options_adv_type,
                  &label_packet_index,
                  &text_packet_index,
                  &label_packets_sent,
                  &text_packets_sent,
                  &label_mac_address,
                  &text_mac_address,
                  &label_data_packet,
                  &button_switch,
                  &console});

    field_frequency.set_step(0);

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        field_frequency.set_value(get_freq_by_channel_number(i));
        channel_number = i;
    };

    options_speed.on_change = [this](size_t, int32_t i) {
        timer_period = i;
    };

    options_adv_type.on_change = [this](size_t, int32_t i) {
        pduType = (PKT_TYPE)i;
    };

    options_speed.set_selected_index(0);
    options_channel.set_selected_index(0);
    options_adv_type.set_selected_index(0);

    check_rand_mac.set_value(false);
    check_rand_mac.on_select = [this](Checkbox&, bool v) {
        random_mac = v;
    };

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);

            nav_.set_on_pop([this]() { button_play.focus(); });
        };
    };

    button_switch.on_select = [this, &nav](Button&) {
        nav_.set_on_pop([this]() { nav_.push<BLERxView>(); });
        nav_.pop();
    };
}

BLETxView::BLETxView(
    NavigationView& nav,
    BLETxPacket packet)
    : BLETxView(nav) {
    packets[0] = packet;
    update_packet_display(packets[0]);

    num_packets = 1;
    file_override = true;
}

void BLETxView::on_file_changed(const fs::path& new_file_path) {
    file_path = fs::path(u"/") + new_file_path;
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
            readUntil(data_file, packets[num_packets].packetCount, max_packet_repeat_str, '\n');

            uint64_t macAddressSize = strlen(packets[num_packets].macAddress);
            uint64_t advertisementDataSize = strlen(packets[num_packets].advertisementData);
            uint64_t packetCountSize = strlen(packets[num_packets].packetCount);

            packets[num_packets].packet_count = stringToUint32(packets[num_packets].packetCount);
            packet_counter = packets[num_packets].packet_count;

            // Verify Data.
            if ((macAddressSize == mac_address_size_str) && (advertisementDataSize < max_packet_size_str) && (packetCountSize < max_packet_repeat_str) &&
                hasValidHexPairs(packets[num_packets].macAddress, macAddressSize / 2) && hasValidHexPairs(packets[num_packets].advertisementData, advertisementDataSize / 2) && (packets[num_packets].packet_count >= 50) && (packets[num_packets].packet_count < max_packet_repeat_count)) {
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

        update_packet_display(packets[0]);
    }
}

void BLETxView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "";

    if (is_data) {
        str_console += (char)(value);
    }

    console.write(str_console);
}

void BLETxView::update_packet_display(BLETxPacket packet) {
    std::string formattedMacAddress = to_string_formatted_mac_address(packet.macAddress);

    std::vector<std::string> strings = splitIntoStrings(packet.advertisementData);

    text_packet_index.set(to_string_dec_uint(current_packet));

    text_packets_sent.set(to_string_dec_uint(packet.packet_count));

    text_mac_address.set(formattedMacAddress);

    console.clear(true);

    for (const std::string& str : strings) {
        console.writeln(str);
    }
}

void BLETxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height - switch_button_height};
    console.set_parent_rect(content_rect);
}

BLETxView::~BLETxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

} /* namespace ui */
