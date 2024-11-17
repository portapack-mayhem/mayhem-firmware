/*
 * Copyright (C) 2024 Samir Sánchez Garnica @sasaga92
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

#include "ui_ook_remote.hpp"
#include "portapack.hpp"
#include "io_file.hpp"
#include "ui_fileman.hpp"
#include "file_path.hpp"
#include <cstring>
#include "baseband_api.hpp"
#include "ui_textentry.hpp"
#include "encoders.hpp"      // Includes data encoding functions for transmission
#include "baseband_api.hpp"  // Includes baseband API for handling transmission settings
#include "file_reader.hpp"

#define PADDING_LEFT 1
#define PADDING_RIGHT 1

using namespace portapack;
using namespace ui;
namespace fs = std::filesystem;

namespace ui::external_app::ook_remote {

void OOKRemoteAppView::focus() {
    button_set.focus();
}

// `start_tx` method: Configures and begins OOK data transmission with a specific message.
void OOKRemoteAppView::start_tx(const std::string& message) {
    size_t bitstream_length = encoders::make_bitstream(const_cast<std::string&>(message));  // Convert the message into a bitstream
    // Retrieve selected sample rate using selected_index_value()
    int32_t SAMPLE_RATE_VALUE = field_sample_rate.selected_index_value();
    int32_t SYMBOL_RATE_VALUE = cant_symbol_rate.value();
    int32_t REPEAT = cant_repeat.value();
    int32_t PAUSE_SYMBOL = cant_pause_symbol.value();

    transmitter_model.set_sampling_rate(SAMPLE_RATE_VALUE);  // Set the OOK sampling rate
    transmitter_model.set_baseband_bandwidth(1750000);       // Set the baseband bandwidth
    transmitter_model.enable();                              // Enable the transmitter

    // Configure OOK data and transmission characteristics
    baseband::set_ook_data(
        bitstream_length,                       // Length of the bitstream to transmit
        SAMPLE_RATE_VALUE / SYMBOL_RATE_VALUE,  // Calculate symbol period based on repetition rate
        REPEAT,                                 // Set the number of repetitions per symbol
        PAUSE_SYMBOL                            // Set the pause between symbols
    );
}

// `stop_tx` method: Stops the transmission and resets the progress bar.
void OOKRemoteAppView::stop_tx() {
    transmitter_model.disable();        // Disable the transmitter
    progressBar_progress.set_value(0);  // Reset progress bar to 0
}

// `on_file_changed` method: Called when a new file is loaded; parses file data into variables

void OOKRemoteAppView::on_file_changed(const fs::path& new_file_path) {
    payload.clear();  // Clear previous payload content

    File data_file;
    auto open_result = data_file.open(new_file_path);
    if (open_result) {
        nav_.display_modal("Error", "Unable to open file.");
        return;
    }

    FileLineReader reader(data_file);
    for (const auto& line : reader) {
        // Split the line into segments to extract each value
        size_t first_space = line.find(' ');
        size_t second_space = line.find(' ', first_space + 1);
        size_t third_space = line.find(' ', second_space + 1);
        size_t fourth_space = line.find(' ', third_space + 1);
        size_t fifth_space = line.find(' ', fourth_space + 1);

        // Extract each component of the line
        std::string frequency_str = line.substr(0, first_space);
        std::string sample_rate_str = line.substr(first_space + 1, second_space - first_space - 1);
        std::string symbols_rate_str = line.substr(second_space + 1, third_space - second_space - 1);
        std::string repeat_str = line.substr(third_space + 1, fourth_space - third_space - 1);
        std::string pause_symbol_str = line.substr(fourth_space + 1, fifth_space - fourth_space - 1);
        std::string payload_data = line.substr(fifth_space + 1);  // Extract binary payload as final value

        // Convert and assign frequency
        unsigned long long frequency = std::stoull(frequency_str);
        field_frequency.set_value(frequency);
        transmitter_model.set_target_frequency(frequency);

        // Convert and assign symbols rate
        unsigned int symbols_rate = static_cast<unsigned int>(atoi(symbols_rate_str.c_str()));

        cant_symbol_rate.set_value(symbols_rate);

        // Convert and assign repeat count
        unsigned int repeat = static_cast<unsigned int>(atoi(repeat_str.c_str()));

        cant_repeat.set_value(repeat);

        // Convert and assign pause per symbol
        unsigned int pause_symbol = static_cast<unsigned int>(atoi(pause_symbol_str.c_str()));

        cant_pause_symbol.set_value(pause_symbol);

        // Select sample rate based on value read from file
        if (sample_rate_str == "250k") {
            field_sample_rate.set_by_value(250000U);
        } else if (sample_rate_str == "1M") {
            field_sample_rate.set_by_value(1000000U);
        } else if (sample_rate_str == "2M") {
            field_sample_rate.set_by_value(2000000U);
        } else if (sample_rate_str == "5M") {
            field_sample_rate.set_by_value(5000000U);
        } else if (sample_rate_str == "10M") {
            field_sample_rate.set_by_value(10000000U);
        } else if (sample_rate_str == "20M") {
            field_sample_rate.set_by_value(20000000U);
        } else {
            nav_.display_modal("Error", "Invalid sample rate.");
            return;
        }

        // Update payload with binary data
        payload = payload_data;

        // Process only the first line
        break;
    }

    // Ensure UI elements are initialized before use
    if (parent()) {
        text_payload.set(payload);
        button_start.focus();
    } else {
        text_payload.set("parent not available");
    }
}

// `on_tx_progress` method: Updates the progress bar based on transmission progress.
void OOKRemoteAppView::on_tx_progress(const uint32_t progress, const bool done) {
    progressBar_progress.set_value(progress);  // Update progress bar value
    if (done) {
        stop_tx();  // Stop transmission when progress reaches maximum
    }
}

// `draw_waveform` method: Draws the waveform on the UI based on the payload data
void OOKRemoteAppView::draw_waveform() {
    // Padding reason:
    // In real-world scenarios, the signal would always start low and return low after turning off the radio.
    // `waveform_buffer` only controls drawing; the actual send logic is handled by frame_fragments.
    size_t length = payload.length();

    // Ensure waveform length does not exceed buffer size
    if (length + (PADDING_LEFT + PADDING_RIGHT) >= WAVEFORM_BUFFER_SIZE) {
        length = WAVEFORM_BUFFER_SIZE - (PADDING_LEFT + PADDING_RIGHT);
    }

    // Left padding
    for (size_t i = 0; i < PADDING_LEFT; i++) {
        waveform_buffer[i] = 0;
    }

    // Draw the actual waveform
    for (size_t n = 0; n < length; n++) {
        waveform_buffer[n + PADDING_LEFT] = (payload[n] == '0') ? 0 : 1;
    }

    // Right padding
    for (size_t i = length + PADDING_LEFT; i < WAVEFORM_BUFFER_SIZE; i++) {
        waveform_buffer[i] = 0;
    }

    waveform.set_length(length + PADDING_LEFT + PADDING_RIGHT);
    waveform.set_dirty();
}

// Destructor for `OOKRemoteAppView`: Disables the transmitter and shuts down the baseband
OOKRemoteAppView::~OOKRemoteAppView() {
    transmitter_model.disable();
    baseband::shutdown();
}

// Constructor for `OOKRemoteAppView`: Sets up the app view and initializes UI elements
OOKRemoteAppView::OOKRemoteAppView(NavigationView& nav)
    : nav_{nav} {
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    add_children({
        &field_frequency,
        &tx_view,
        &button_start,
        &sample_rate,
        &step_symbol_rate,
        &cant_step_symbol_rate,
        &field_sample_rate,
        &field_symbol_rate,
        &field_symbol_us_rate,
        &cant_symbol_rate,
        &text_payload,
        &button_set,
        &progressBar_progress,
        &repeat,
        &cant_repeat,
        &field_pause_symbol,
        &cant_pause_symbol,
        &label_waveform,
        &waveform,
        &button_open,
    });

    // Initialize default values for controls
    cant_pause_symbol.set_value(100);
    cant_repeat.set_value(4);

    button_open.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".OOK");
        ensure_directory(ook_remote_dir);
        open_view->push_dir(ook_remote_dir);
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            // Postpone `on_file_changed` call until `FileLoadView` is closed
            nav_.set_on_pop([this, new_file_path]() {
                on_file_changed(new_file_path);
                button_start.focus();
                draw_waveform();
            });
        };
    };

    // Set up changes for symbol rate and step
    cant_symbol_rate.on_change = [this](int32_t value) {
        if (value != cant_symbol_rate.value())
            cant_symbol_rate.set_value(value);
    };

    cant_step_symbol_rate.on_change = [this](size_t, int32_t value) {
        cant_symbol_rate.set_step(value);
    };

    // Configure button to manually set payload through text input
    button_set.on_select = [this, &nav](Button&) {
        text_prompt(
            nav,
            payload,
            100,
            [this](std::string& s) {
                text_payload.set(s);
                draw_waveform();
            });
    };
    draw_waveform();
    button_start.on_select = [this](Button&) {
        start_tx(payload);  // Begin transmission
    };
}
}  // namespace ui::external_app::ook_remote
