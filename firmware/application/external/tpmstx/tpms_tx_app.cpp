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

using namespace portapack;
using namespace tpms;

namespace ui::external_app::tpmstx {

void TPMSTXView::focus() {
    options_packet_type.focus();
}

void TPMSTXView::update_packet_display() {
    // Update display fields
    field_transponder_id.set_value(transponder_id_);
    field_pressure.set_value(pressure_kpa_);
    field_temperature.set_value(temperature_c_);
    field_flags.set_value(flags_);

    // Update status
    std::string status = "ID: " + to_string_hex(transponder_id_, 8);
    status += " P:" + to_string_dec_uint(pressure_kpa_) + "kPa";
    status += " T:" + to_string_dec_int(temperature_c_) + "C";
    text_status.set(status);
}

void TPMSTXView::encode_and_transmit() {
    if (is_transmitting_) {
        // Build TPMS packet data
        // This is a simplified encoder - actual TPMS encoding is protocol-specific
        // For Schrader FSK protocol as an example:
        
        // Create a basic packet structure (this would need to be protocol-specific)
        std::vector<uint8_t> packet_data;
        
        // Schrader format (simplified):
        // Byte 0: Flags/Status
        // Bytes 1-4: Transponder ID
        // Bytes 5-6: Pressure (8-bit value in kPa / 4)
        // Byte 7: Temperature (8-bit signed, offset by 50)
        // Byte 8: Checksum
        
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

        // Send to baseband for transmission
        if (signal_type_ == tpms::SignalType::FSK_19k2_Schrader) {
            // FSK transmission
            baseband::set_fsk_data(
                encoded_data.size() * 8,  // Number of bits
                9600,                      // Symbol rate
                19200,                     // Deviation
                50                         // Pause between repeats (ms)
            );
        } else {
            // OOK transmission
            baseband::set_ook_data(
                encoded_data.size() * 8,  // Number of bits
                2000,                     // Samples per bit
                current_repeat_,          // Repeat count (handled manually)
                pause_duration_          // Pause between repeats (ms)
            );
        }

        current_repeat_++;
        progressbar.set_value(current_repeat_);

        if (current_repeat_ >= repeat_count_) {
            stop_tx();
        }
    }
}

void TPMSTXView::handle_tx_complete() {
    if (is_transmitting_ && current_repeat_ < repeat_count_) {
        // Continue with next repeat
        chThdSleepMilliseconds(pause_duration_);
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

    add_children({
        &labels,
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
        &progressbar
    });

    // Initialize values
    options_packet_type.set_selected_index(0);
    options_signal_type.set_selected_index(0);
    field_repeat.set_value(repeat_count_);
    
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
                char buffer[256];
                auto result = f.read(buffer, sizeof(buffer) - 1);
                if (result.is_ok()) {
                    buffer[result.value()] = 0;
                    
                    // Parse the file format (simple text format)
                    // Format: ID=<hex> P=<kPa> T=<C> F=<hex>
                    std::string content(buffer);
                    
                    // Simple parsing (you could make this more robust)
                    size_t id_pos = content.find("ID=");
                    size_t p_pos = content.find("P=");
                    size_t t_pos = content.find("T=");
                    size_t f_pos = content.find("F=");
                    
                    if (id_pos != std::string::npos) {
                        transponder_id_ = std::stoul(content.substr(id_pos + 3, 8), nullptr, 16);
                    }
                    if (p_pos != std::string::npos) {
                        pressure_kpa_ = std::stoi(content.substr(p_pos + 2));
                    }
                    if (t_pos != std::string::npos) {
                        temperature_c_ = std::stoi(content.substr(t_pos + 2));
                    }
                    if (f_pos != std::string::npos) {
                        flags_ = std::stoul(content.substr(f_pos + 2, 2), nullptr, 16);
                    }
                    
                    update_packet_display();
                    text_status.set("Loaded from file");
                }
            }
        };
    };

    button_save.on_select = [this](Button&) {
        // Save current TPMS packet to file
        auto timestamp = to_string_timestamp(rtc_time::now());
        std::string file_name = "TPMS_" + timestamp + ".TXT";
        std::string file_path = logs_dir.string() + "/" + file_name;
        
        File f;
        auto error = f.create(file_path);
        if (!error.is_valid()) {
            std::string content = "ID=" + to_string_hex(transponder_id_, 8);
            content += " P=" + to_string_dec_uint(pressure_kpa_);
            content += " T=" + to_string_dec_int(temperature_c_);
            content += " F=" + to_string_hex(flags_, 2);
            content += "\n";
            
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
