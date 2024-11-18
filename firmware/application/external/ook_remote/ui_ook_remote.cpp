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
    int32_t SYMBOL_DURATION_VALUE = field_bit_duration.value();
    int32_t REPEAT = field_repeat.value();
    int32_t PAUSE_SYMBOL = field_pause_symbol.value();

    transmitter_model.set_sampling_rate(SAMPLE_RATE_VALUE);  // Set the OOK sampling rate
    transmitter_model.set_baseband_bandwidth(1750000);       // Set the baseband bandwidth
    transmitter_model.enable();                              // Enable the transmitter

    // Configure OOK data and transmission characteristics
    baseband::set_ook_data(
        bitstream_length,                           // Length of the bitstream to transmit
        SAMPLE_RATE_VALUE / SYMBOL_DURATION_VALUE,  // Calculate symbol period based on repetition rate
        REPEAT,                                     // Set the number of times the whole bitstream is repeated
        PAUSE_SYMBOL                                // Set the pause between symbols
    );
    progressbar.set_max(REPEAT);                                            // Size the progress bar accordingly to the number of repeat
    is_transmitting = true;                                                 // set transmitting flag
    button_send_stop.set_text(LanguageHelper::currentMessages[LANG_STOP]);  // set button back to initial "start" state
}

// `stop_tx` method: Stops the transmission and resets the progress bar.
void OOKRemoteAppView::stop_tx() {
    is_transmitting = false;
    transmitter_model.disable();                                             // Disable the transmitter
    progressbar.set_value(0);                                                // Reset progress bar to 0
    button_send_stop.set_text(LanguageHelper::currentMessages[LANG_START]);  // set button back to initial "start" state
}

// `on_file_changed` method: Called when a new file is loaded; parses file data into variables
void OOKRemoteAppView::on_file_changed(const fs::path& new_file_path) {
    payload.clear();           // Clear previous payload content
    text_loaded_file.set("");  // Clear loaded file text field

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
        rf::Frequency frequency = std::stoull(frequency_str);
        field_frequency.set_value(frequency);
        transmitter_model.set_target_frequency(frequency);

        // Convert and assign symbols rate
        unsigned int bit_duration_value = static_cast<unsigned int>(atoi(symbols_rate_str.c_str()));

        field_bit_duration.set_value(bit_duration_value);

        // Convert and assign repeat count
        unsigned int repeat = static_cast<unsigned int>(atoi(repeat_str.c_str()));

        field_repeat.set_value(repeat);

        // Convert and assign pause per symbol
        unsigned int pause_symbol = static_cast<unsigned int>(atoi(pause_symbol_str.c_str()));

        field_pause_symbol.set_value(pause_symbol);

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
        button_send_stop.focus();
    } else {
        text_payload.set("parent not available");
    }
    text_loaded_file.set("Loaded: " + new_file_path.filename().string());
}

// `on_tx_progress` method: Updates the progress bar based on transmission progress.
void OOKRemoteAppView::on_tx_progress(const uint32_t progress, const bool done) {
    progressbar.set_value(progress);  // Update progress bar value
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

void OOKRemoteAppView::on_save_file(const std::string value) {
    // check if there is a payload, else Error
    if (payload.length() < 1) {
        text_loaded_file.set("Error: no payload !!");
        return;
    }

    ensure_directory(ook_remote_dir);
    auto new_path = ook_remote_dir / value + ".OOK";
    if (save_ook_to_file(new_path)) {
        text_loaded_file.set("Saved to " + new_path.string());
    } else {
        text_loaded_file.set("Error saving " + new_path.string());
    }
}

bool OOKRemoteAppView::save_ook_to_file(const std::filesystem::path& path) {
    // delete file if it exists
    delete_file(path);

    // Attempt to open, if it can't be opened. Create new.
    auto src = std::make_unique<File>();
    auto error = src->open(path, false, true);
    if (error) {
        return false;
    }

    // write informations
    src->write_line(to_string_dec_uint(field_frequency.value()) + " " +
                    field_sample_rate.selected_index_name() + " " +
                    to_string_dec_uint(field_bit_duration.value()) + " " +
                    to_string_dec_uint(field_repeat.value()) + " " +
                    to_string_dec_uint(field_pause_symbol.value()) + " " +
                    payload);

    // Close files
    src.reset();

    return true;
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

    add_children({&field_frequency,
                  &tx_view,
                  &button_send_stop,
                  &label_bit_duration_step,
                  &field_bit_duration_step,
                  &label_sample_rate,
                  &field_sample_rate,
                  &label_bit_duration,
                  &field_bit_duration,
                  &label_bit_duration_unit,
                  &text_payload,
                  &button_set,
                  &progressbar,
                  &label_repeat,
                  &field_repeat,
                  &label_pause_symbol,
                  &field_pause_symbol,
                  &label_payload,
                  &text_loaded_file,
                  &label_waveform,
                  &waveform,
                  &button_open,
                  &button_save});

    // Initialize default values for controls
    field_pause_symbol.set_value(100);
    field_repeat.set_value(4);

    button_open.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".OOK");
        ensure_directory(ook_remote_dir);
        open_view->push_dir(ook_remote_dir);
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            // Postpone `on_file_changed` call until `FileLoadView` is closed
            nav_.set_on_pop([this, new_file_path]() {
                on_file_changed(new_file_path);
                button_send_stop.focus();
                draw_waveform();
            });
        };
    };

    button_save.on_select = [this, &nav](const ui::Button&) {
        outputFileBuffer = "";
        text_prompt(
            nav,
            outputFileBuffer,
            64,
            [this](std::string& buffer) {
                on_save_file(buffer);
            });
    };

    // clean out loaded file name if field is changed
    field_bit_duration.on_change = [this](int32_t) {
        text_loaded_file.set("");  // Clear loaded file text field
    };
    // clean out loaded file name if field is changed
    field_repeat.on_change = [this](int32_t) {
        text_loaded_file.set("");  // Clear loaded file text field
    };
    // clean out loaded file name if field is changed
    field_pause_symbol.on_change = [this](int32_t) {
        text_loaded_file.set("");  // Clear loaded file text field
    };
    // clean out loaded file name if field is changed
    field_sample_rate.on_change = [this](size_t, int32_t) {
        text_loaded_file.set("");  // Clear loaded file text field
    };

    // setting up FrequencyField
    field_frequency.set_value(ook_remote_tx_freq);

    // clean out loaded file name if field is changed, save ook_remote_tx_freq
    field_frequency.on_change = [this](rf::Frequency f) {
        ook_remote_tx_freq = f;
        text_loaded_file.set("");  // Clear loaded file text field
    };

    // allow typing frequency number
    field_frequency.on_edit = [this]() {
        auto freq_view = nav_.push<FrequencyKeypadView>(field_frequency.value());
        freq_view->on_changed = [this](rf::Frequency f) {
            field_frequency.set_value(f);
            text_loaded_file.set("");  // Clear loaded file text field
        };
    };

    field_bit_duration_step.on_change = [this](size_t, int32_t value) {
        text_loaded_file.set("");  // Clear loaded file text field
        field_bit_duration.set_step(value);
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
                text_loaded_file.set("");  // Clear loaded file text field
            });
    };
    draw_waveform();
    button_send_stop.on_select = [this](Button&) {
        if (!is_transmitting) {
            start_tx(payload);  // Begin transmission
        } else {
            stop_tx();
        }
    };
}
}  // namespace ui::external_app::ook_remote
