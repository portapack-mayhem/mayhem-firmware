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
            return false; // Return false if any pair is invalid.
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

    // Check if there are any non-digit characters left
    if (pos < str.size()) {
        return 0;
    }

    return result;
}

void readUntilSpace(File& file, char* result, std::size_t maxBufferSize) {
    std::size_t bytesRead = 0;

    while (true) {
        char ch;
        File::Result<File::Size> readResult = file.read(&ch, 1);

        if (readResult.is_ok() && readResult.value() > 0) {
            if (ch == ' ') {
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
            break; // End of file or error
        }
    }

    // Null-terminate the result string
    result[bytesRead] = '\0';
}

namespace ui {

void BLETxView::focus() {
    field_frequency.focus();
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

    if (!is_active())
    {
        // Check if file is present before continuing.
        File data_file;

        auto error = data_file.open(file_path);
        if (error) {
            file_error();
            check_loop.set_value(false);
            return;
        }
        else
        {
            // Send first or single packet.
            progressbar.set_max(packet_count);
            button_play.set_bitmap(&bitmap_stop);
            baseband::set_btletx(channel_number, macAddress, advertisementData);
            transmitter_model.enable();

            is_running = true;
        }
    }
    else
    {
        // Send next packet.
        baseband::set_btletx(channel_number, macAddress, advertisementData);
    }
    
    if ((packet_counter % 10) == 0) {
        text_packets_sent.set(to_string_dec_uint(packet_counter));
    }

    packet_counter--;

    progressbar.set_value(packet_count - packet_counter);
}

void BLETxView::stop() {
    transmitter_model.disable();
    progressbar.set_value(0);
    button_play.set_bitmap(&bitmap_play);
    check_loop.set_value(false);
    text_packets_sent.set(to_string_dec_uint(packet_count));
    packet_counter = packet_count;
    is_running = false;
}

void BLETxView::on_tx_progress(const uint32_t progress, const bool done) {
    if (done) {
        if (check_loop.value() && (packet_counter != 0) && is_active()) {
            if ((timer_count % timer_period) == 0) {
                start();
            }
        } else {
            if (is_active())
            {
                stop();
            }
        }

        timer_count++;
    }
}

BLETxView::BLETxView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_btle_tx);

    add_children({&button_open,
                  &text_filename,
                  &progressbar,
                  &field_frequency,
                  &tx_view,  // now it handles previous rfgain, rfamp.
                  &check_loop,
                  &button_play,
                  &label_speed,
                  &options_speed,
                  &options_channel,
                  &label_packets_sent,
                  &text_packets_sent,
                  &label_mac_address,
                  &text_mac_address,
                  &label_data_packet,
                  &console});

    field_frequency.set_step(0);

    button_play.on_select = [this](ImageButton&) {
        this->toggle();
    };

    options_channel.on_change = [this](size_t, int32_t i) {
        channel_number = i;
    };

    options_speed.on_change = [this](size_t, int32_t i) {
        timer_period = i;
    };

    options_speed.set_selected_index(0);

    button_open.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            on_file_changed(new_file_path);
        };
    };
}

void BLETxView::on_file_changed(const fs::path& new_file_path) {
    file_path = fs::path(u"/") + new_file_path;

    {  // Get the size of the data file.
        File data_file;
        auto error = data_file.open(file_path);
        if (error) {
            file_error();
            file_path = "";
            return;
        }

        readUntilSpace(data_file, macAddress, mac_address_size_str);
        readUntilSpace(data_file, advertisementData, max_packet_size_str);
        readUntilSpace(data_file, packetCount, max_packet_count_str);

        uint64_t macAddressSize = strlen(macAddress);
        uint64_t advertisementDataSize = strlen(advertisementData);
        uint64_t packetCountSize = strlen(packetCount);

        packet_count = stringToUint32(packetCount);
        packet_counter = packet_count;

         console.writeln(macAddress);
         console.writeln(advertisementData);
         console.writeln(packetCount);

         console.writeln(to_string_dec_uint(macAddressSize));
         console.writeln(to_string_dec_uint(advertisementDataSize));
         console.writeln(to_string_dec_uint(packetCountSize));
         console.writeln(to_string_dec_uint(packet_count));

        // Verify Data.
        if ((macAddressSize == mac_address_size_str) && (advertisementDataSize < max_packet_size_str) && (packetCountSize < max_packet_count_str) &&
            hasValidHexPairs(macAddress, macAddressSize / 2) && hasValidHexPairs(advertisementData, advertisementDataSize / 2) && (packet_count > 0) && (packet_count < UINT32_MAX))
        {
            text_packets_sent.set(to_string_dec_uint(packet_count));

            std::string formattedMacAddress = to_string_formatted_mac_address(macAddress);

            text_mac_address.set(formattedMacAddress);

            std::vector<std::string> strings = splitIntoStrings(advertisementData);

            console.clear(true);

            for (const std::string& str : strings) {
                console.writeln(str);
            }

            text_filename.set(truncate(file_path.filename().string(), 12));
        }
        else
        {
          //  file_error();
            file_path = "";
            return;
        }
    }
}

void BLETxView::on_data(uint32_t value, bool is_data) {
    std::string str_console = "";

    if (is_data) {
        str_console += (char)(value);
    } 

    console.write(str_console);
}

void BLETxView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);
    const Rect content_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    console.set_parent_rect(content_rect);
}

BLETxView::~BLETxView() {
    transmitter_model.disable();
    baseband::shutdown();
}

} /* namespace ui */
