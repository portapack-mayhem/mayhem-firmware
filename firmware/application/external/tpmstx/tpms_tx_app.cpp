/*
 * Copyright (C) 2026
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

#include "tpms_tx_app.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "ui_fileman.hpp"
#include "ui_freqman.hpp"
#include "manchester.hpp"
#include "rtc_time.hpp"
#include "file_path.hpp"
#include "encoders.hpp"

using namespace portapack;
using namespace tpms;

namespace ui::external_app::tpmstx {

void TPMSTXView::focus() {
    options_packet_type.focus();
}

void TPMSTXView::update_packet_display() {
    // Only update status text - field values are already set by user interaction
    // or by explicit set_value calls when loading from file
    std::string status = "ID:" + to_string_hex(transponder_id_, 8);
    status += " " + to_string_dec_uint(pressure_kpa_) + "kPa";
    status += " " + to_string_dec_int(temperature_c_) + "C";
    text_status.set(status);
}

void TPMSTXView::encode_and_transmit() {
    if (!is_transmitting_) return;

    // Build TPMS packet data
    // This is a simplified encoder - actual TPMS encoding is protocol-specific
    // For Schrader protocol as an example:

    // Create packet_data structure (this would need to be protocol-specific)
    std::vector<uint8_t> packet_data;

    // Schrader format (simplified):
    // Byte 0: Flags/Status
    // Bytes 1-4: Transponder ID
    // Byte 5: Pressure (8-bit value in kPa / 4)
    // Byte 6: Temperature (8-bit signed, offset by 50)
    // Byte 7: Checksum

    packet_data.push_back(flags_);
    packet_data.push_back((transponder_id_ >> 24) & 0xFF);
    packet_data.push_back((transponder_id_ >> 16) & 0xFF);
    packet_data.push_back((transponder_id_ >> 8) & 0xFF);
    packet_data.push_back(transponder_id_ & 0xFF);

    // Pressure: convert kPa to protocol format (typically kPa/4 for Schrader)
    uint8_t pressure_byte = (pressure_kpa_ / 4) & 0xFF;
    packet_data.push_back(pressure_byte);

    // Temperature: offset by 50 degrees (common in TPMS)
    int8_t temp_offset = temperature_c_ + 50;
    packet_data.push_back((uint8_t)temp_offset);

    // Simple checksum (sum of all bytes)
    uint8_t checksum = 0;
    for (auto byte : packet_data) {
        checksum += byte;
    }
    packet_data.push_back(checksum);

    // Manchester encode the data
    std::vector<uint8_t> encoded_data;
    encoded_data.resize(packet_data.size() * 2);
    manchester_encode(encoded_data.data(), packet_data.data(), packet_data.size(), 0);

    // Convert to binary string for OOK transmission
    std::string binary_string;
    for (auto byte : encoded_data) {
        for (int i = 7; i >= 0; i--) {
            binary_string += ((byte >> i) & 1) ? '1' : '0';
        }
    }

    // Set up transmission parameters based on signal type
    uint32_t symbol_rate;
    if (signal_type_ == tpms::SignalType::FSK_19k2_Schrader) {
        symbol_rate = 19200;  // FSK mode
    } else if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader) {
        symbol_rate = 8192;  // OOK mode
    } else {
        symbol_rate = 8400;  // OOK 8k4 mode
    }

    // Convert binary string to bitstream
    size_t bitstream_length = encoders::make_bitstream(binary_string);

    // Calculate samples per bit
    uint32_t samples_per_bit = transmitter_model.sampling_rate() / symbol_rate;

    // Send via OOK baseband
    baseband::set_ook_data(
        bitstream_length,
        samples_per_bit,
        1,               // Single transmission (we handle repeats manually)
        pause_duration_  // Pause between repeats
    );

    current_repeat_++;
    progressbar.set_value(current_repeat_);

    if (current_repeat_ >= repeat_count_) {
        // All repeats done
        chThdSleepMilliseconds(pause_duration_);
        stop_tx();
    } else {
        // More repeats to go, schedule next one
        chThdSleepMilliseconds(pause_duration_);
    }
}

void TPMSTXView::handle_tx_complete() {
    if (is_transmitting_ && current_repeat_ < repeat_count_) {
        // Continue with next repeat
        encode_and_transmit();
    }
}

void TPMSTXView::start_tx() {
    if (is_transmitting_)
        return;

    is_transmitting_ = true;
    current_repeat_ = 0;

    progressbar.set_max(repeat_count_);
    progressbar.set_value(0);

    button_transmit.set_text("STOP TX");
    text_status.set("Transmitting...");

    transmitter_model.set_sampling_rate(2457600);
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.enable();

    // Start first transmission
    encode_and_transmit();
}

void TPMSTXView::stop_tx() {
    if (!is_transmitting_)
        return;

    is_transmitting_ = false;
    transmitter_model.disable();

    button_transmit.set_text("START TX");
    text_status.set("Transmission complete");
    progressbar.set_value(0);
}

TPMSTXView::TPMSTXView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({&labels,
                  &options_packet_type,
                  &field_transponder_id,
                  &field_pressure,
                  &field_temperature,
                  &field_flags,
                  &options_signal_type,
                  &field_repeat,
                  &button_load,
                  &button_save,
                  &tx_view,
                  &button_transmit,
                  &text_status,
                  &progressbar});

    // Initialize values
    options_packet_type.set_selected_index(0);
    options_signal_type.set_selected_index(0);
    field_repeat.set_value(repeat_count_);
    
    // Initialize field values from default member variables
    field_transponder_id.set_value(transponder_id_);
    field_pressure.set_value(pressure_kpa_);
    field_temperature.set_value(temperature_c_);
    field_flags.set_value(flags_);

    update_packet_display();

    // Event handlers
    options_packet_type.on_change = [this](size_t, int32_t value) {
        packet_type_ = static_cast<tpms::Reading::Type>(value);
        update_packet_display();
    };

    field_transponder_id.on_change = [this](SymField&) {
        transponder_id_ = field_transponder_id.to_integer();
        update_packet_display();
    };

    field_pressure.on_change = [this](int32_t value) {
        pressure_kpa_ = value;
        update_packet_display();
    };

    field_temperature.on_change = [this](int32_t value) {
        temperature_c_ = value;
        update_packet_display();
    };

    field_flags.on_change = [this](SymField&) {
        flags_ = field_flags.to_integer() & 0xFF;
        update_packet_display();
    };

    options_signal_type.on_change = [this](size_t, int32_t value) {
        signal_type_ = static_cast<tpms::SignalType>(value);
    };

    field_repeat.on_change = [this](int32_t value) {
        repeat_count_ = value;
    };

    button_transmit.on_select = [this](Button&) {
        if (is_transmitting_) {
            stop_tx();
        } else {
            start_tx();
        }
    };

    button_load.on_select = [this, &nav](Button&) {
        auto open_view = nav.push<FileLoadView>(".TXT");
        open_view->on_changed = [this](std::filesystem::path file_path) {
            // Load TPMS packet from file
            File f;
            auto error = f.open(file_path);
            if (!error.is_valid()) {
                char buffer[512];
                auto result = f.read(buffer, sizeof(buffer) - 1);
                if (result.is_ok()) {
                    buffer[result.value()] = 0;

                    // Parse the file format (key=value format, one per line)
                    std::string content(buffer);
                    size_t pos = 0;

                    auto parse_line = [&content, &pos](const std::string& key) -> std::string {
                        size_t key_pos = content.find(key + "=", pos);
                        if (key_pos != std::string::npos) {
                            size_t value_start = key_pos + key.length() + 1;
                            size_t value_end = content.find('\n', value_start);
                            if (value_end == std::string::npos) value_end = content.length();
                            return content.substr(value_start, value_end - value_start);
                        }
                        return "";
                    };

                    std::string type_str = parse_line("Type");
                    std::string id_str = parse_line("ID");
                    std::string pressure_str = parse_line("Pressure");
                    std::string temp_str = parse_line("Temperature");
                    std::string flags_str = parse_line("Flags");

                    if (!type_str.empty()) {
                        int type = std::stoi(type_str);
                        if (type >= static_cast<int>(tpms::Reading::Type::FLM_64) &&
                            type <= static_cast<int>(tpms::Reading::Type::GMC_96)) {
                            packet_type_ = static_cast<tpms::Reading::Type>(type);
                            options_packet_type.set_selected_index(type - 1);
                        }
                    }

                    if (!id_str.empty()) {
                        transponder_id_ = std::stoul(id_str, nullptr, 16);
                        field_transponder_id.set_value(transponder_id_);
                    }

                    if (!pressure_str.empty()) {
                        pressure_kpa_ = std::stoi(pressure_str);
                        field_pressure.set_value(pressure_kpa_);
                    }

                    if (!temp_str.empty()) {
                        temperature_c_ = std::stoi(temp_str);
                        field_temperature.set_value(temperature_c_);
                    }

                    if (!flags_str.empty()) {
                        flags_ = std::stoul(flags_str, nullptr, 16);
                        field_flags.set_value(flags_);
                    }

                    update_packet_display();
                    text_status.set("Loaded: " + file_path.filename().string());
                } else {
                    text_status.set("Error reading file");
                }
            } else {
                text_status.set("Error opening file");
            }
        };
    };

    button_save.on_select = [this](Button&) {
        // Save current TPMS packet to file
        auto timestamp = to_string_timestamp(rtc_time::now());
        std::string file_name = "TPMS_" + timestamp + ".TXT";
        ensure_directory(tpms_dir);
        auto file_path = tpms_dir / file_name;

        File f;
        auto error = f.create(file_path);
        if (!error.is_valid()) {
            std::string content = "Type=" + to_string_dec_uint(toUType(packet_type_), 1) + "\n";
            content += "ID=" + to_string_hex(transponder_id_, 8) + "\n";
            content += "Pressure=" + to_string_dec_uint(pressure_kpa_) + "\n";
            content += "Temperature=" + to_string_dec_int(temperature_c_) + "\n";
            content += "Flags=" + to_string_hex(flags_, 2) + "\n";

            f.write(content.c_str(), content.length());
            text_status.set("Saved: " + file_name);
        } else {
            text_status.set("Error saving file");
        }
    };

    transmitter_model.set_target_frequency(314900000);
}

TPMSTXView::~TPMSTXView() {
    stop_tx();
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::tpmstx
