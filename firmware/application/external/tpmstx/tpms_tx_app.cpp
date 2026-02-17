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

    // Build TPMS packet based on signal type
    std::string binary_string;
    uint32_t symbol_rate;
    uint32_t sample_rate = 2000000;  // 2 MHz sample rate

    if (signal_type_ == tpms::SignalType::OOK_8k192_Schrader) {
        // Schrader OOK 8k192 format (Type = Schrader)
        // Preamble: 11*2, 01*14, 11, 10
        // Data: 3 bits flags, 24 bits ID, 8 bits pressure, 2 bits checksum
        // Total: 37 Manchester symbols (74 bits after encoding)
        
        symbol_rate = 8192;

        // Preamble as expected by RX decoder
        binary_string = "1111";  // 11*2
        for (int i = 0; i < 14; i++) {
            binary_string += "01";
        }
        binary_string += "1110";  // 11, 10

        // Build data bits (pre-Manchester)
        uint64_t data = 0;
        
        // 3-bit flags
        uint8_t flags_3bit = flags_ & 0x07;
        data |= ((uint64_t)flags_3bit << 34);
        
        // 24-bit ID (only use lower 24 bits)
        uint32_t id_24bit = transponder_id_ & 0x00FFFFFF;
        data |= ((uint64_t)id_24bit << 10);
        
        // 8-bit pressure (convert kPa to raw: kPa * 3/4)
        uint8_t pressure_raw = (pressure_kpa_ * 3 / 4) & 0xFF;
        data |= ((uint64_t)pressure_raw << 2);
        
        // Calculate 2-bit checksum: sum all 2-bit pairs, result & 3 must equal 3
        uint32_t checksum_calc = (data >> 36) & 1;  // First bit
        for (size_t i = 1; i < 37; i += 2) {
            checksum_calc += (data >> (37 - i - 2)) & 3;
        }
        uint8_t checksum_2bit = (3 - (checksum_calc & 3)) & 3;
        data |= checksum_2bit;

        // Manchester encode the 37 data bits
        for (int i = 36; i >= 0; i--) {
            if ((data >> i) & 1) {
                binary_string += "10";  // '1' = 10 in Manchester
            } else {
                binary_string += "01";  // '0' = 01 in Manchester
            }
        }
        
    } else if (signal_type_ == tpms::SignalType::OOK_8k4_Schrader) {
        // GMC_96 OOK 8k4 format
        symbol_rate = 8400;
        
        // Preamble: 01*40
        for (int i = 0; i < 40; i++) {
            binary_string += "01";
        }
        
        // System ID: first nibble is 0x4, then 20 more bits
        binary_string += "01";  // 0
        binary_string += "10";  // 1 
        binary_string += "01";  // 0
        binary_string += "01";  // 0  (0100 = 0x4)
        for (int i = 0; i < 20; i++) {
            binary_string += "01";  // Padding
        }
        
        // 32-bit ID (Manchester encoded)
        for (int i = 31; i >= 0; i--) {
            if ((transponder_id_ >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }
        
        // Pressure: kPa * 4/11
        uint8_t pressure_gmc = (pressure_kpa_ * 4 / 11) & 0xFF;
        for (int i = 7; i >= 0; i--) {
            if ((pressure_gmc >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }
        
        // Temperature: temp + 61
        uint8_t temp_gmc = (temperature_c_ + 61) & 0xFF;
        for (int i = 7; i >= 0; i--) {
            if ((temp_gmc >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }
        
        // Checksum: sum of all bytes
        uint8_t checksum = 0x44;  // First nibble 0x4 << 4 | next nibble
        checksum += pressure_gmc;
        checksum += temp_gmc;
        for (int i = 7; i >= 0; i--) {
            if ((checksum >> i) & 1) {
                binary_string += "10";
            } else {
                binary_string += "01";
            }
        }
        
    } else {
        // FSK_19k2_Schrader - not properly supported yet
        symbol_rate = 19200;
        text_status.set("FSK not implemented");
        stop_tx();
        return;
    }

    // Convert binary string to bitstream
    size_t bitstream_length = encoders::make_bitstream(binary_string);

    // Calculate samples per bit
    uint32_t samples_per_bit = sample_rate / symbol_rate;

    // Update status to show we're sending
    text_status.set("TX: " + to_string_dec_uint(binary_string.length()) + " bits");

    // Send via OOK baseband
    baseband::set_ook_data(
        bitstream_length,
        samples_per_bit,
        repeat_count_,
        pause_duration_
    );
}

void TPMSTXView::handle_tx_complete() {
    // Transmission complete
    stop_tx();
}

void TPMSTXView::start_tx() {
    if (is_transmitting_)
        return;

    is_transmitting_ = true;

    progressbar.set_max(repeat_count_);
    progressbar.set_value(0);

    button_transmit.set_text("STOP TX");
    text_status.set("Transmitting...");

    // Configure transmitter
    transmitter_model.set_sampling_rate(2000000);  // 2 MHz
    transmitter_model.set_baseband_bandwidth(1750000);
    transmitter_model.enable();

    // Start transmission
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
                  &options_frequency,
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
    options_frequency.set_by_value(transmitter_model.target_frequency());
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
    options_frequency.on_change = [this](size_t, OptionsField::value_t v) {
        transmitter_model.set_target_frequency(v);
    };

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
}

TPMSTXView::~TPMSTXView() {
    stop_tx();
    transmitter_model.disable();
    baseband::shutdown();
}

}  // namespace ui::external_app::tpmstx
