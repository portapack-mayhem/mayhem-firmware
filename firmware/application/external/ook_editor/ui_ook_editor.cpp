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

#include "ui_ook_editor.hpp"

using namespace portapack;
using namespace ui;

namespace fs = std::filesystem;

namespace ui::external_app::ook_editor {

// give focus to set button
void OOKEditorAppView::focus() {
    button_set.focus();
}

// update internal ook_data with GUI values
void OOKEditorAppView::update_ook_data_from_app() {
    ook_data.frequency = field_frequency.value();
    ook_data.sample_rate = field_sample_rate.selected_index_value();
    ook_data.symbol_rate = field_symbol_rate.value();
    ook_data.repeat = field_repeat.value();
    ook_data.pause_symbol_duration = field_pause_symbol_duration.value();
}

// `start_tx` method: Configures and begins OOK data transmission with a specific message.
void OOKEditorAppView::start_tx() {
    // check if there is a payload
    if (ook_data.payload.length() < 1) {
        text_app_status.set("Error: no payload to tx !!");
        return;
    }
    progressbar.set_max(field_repeat.value());                              // Size the progress bar accordingly to the number of repeat
    is_transmitting = true;                                                 // set transmitting flag
    button_send_stop.set_text(LanguageHelper::currentMessages[LANG_STOP]);  // set button back to initial "start" state
    start_ook_file_tx(ook_data);                                            // start the transmission
}

// `stop_tx` method: Stops the transmission and resets the progress bar.
void OOKEditorAppView::stop_tx() {
    // TODO: model stopped but message still spamming.
    is_transmitting = false;                                                 // set transmitting flag
    stop_ook_file_tx();                                                      // stop transmission
    progressbar.set_value(0);                                                // Reset progress bar to 0
    button_send_stop.set_text(LanguageHelper::currentMessages[LANG_START]);  // set button back to initial "start" state
}

// `on_file_changed` method: Called when a new file is loaded; parses file data into variables
void OOKEditorAppView::on_file_changed(const fs::path& new_file_path) {
    ook_data.payload.clear();  // Clear previous payload content
    if (!read_ook_file(new_file_path, ook_data)) {
        text_app_status.set("Error loading " + new_file_path.filename().string());
        return;
    }
    field_frequency.set_value(ook_data.frequency);
    field_symbol_rate.set_value(ook_data.symbol_rate);
    field_repeat.set_value(ook_data.repeat);
    field_pause_symbol_duration.set_value(ook_data.pause_symbol_duration);
    field_sample_rate.set_by_value(ook_data.sample_rate);
    text_payload.set(ook_data.payload);
    button_send_stop.focus();
    text_app_status.set("Loaded: " + new_file_path.filename().string());
}

// `on_tx_progress` method: Updates the progress bar based on transmission progress.
void OOKEditorAppView::on_tx_progress(const uint32_t progress, const bool done) {
    if (is_transmitting) progressbar.set_value(progress);  // Update progress bar value
    if (done) {
        stop_tx();  // Stop transmission when progress reaches maximum
    }
}

// `draw_waveform` method: Draws the waveform on the UI based on the payload data
void OOKEditorAppView::draw_waveform() {
    // Padding reason:
    // In real-world scenarios, the signal would always start low and return low after turning off the radio.
    // `waveform_buffer` only controls drawing; the actual send logic is handled by frame_fragments.
    size_t length = ook_data.payload.length();

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
        waveform_buffer[n + PADDING_LEFT] = (ook_data.payload[n] == '0') ? 0 : 1;
    }

    // Right padding
    for (size_t i = length + PADDING_LEFT; i < WAVEFORM_BUFFER_SIZE; i++) {
        waveform_buffer[i] = 0;
    }

    waveform.set_length(length + PADDING_LEFT + PADDING_RIGHT);
    waveform.set_dirty();
}

// build a new path+file, make some tests, call save_ook_to_file
void OOKEditorAppView::on_save_file(const std::string value) {
    // check if there is a payload, else Error
    if (ook_data.payload.length() < 1) {
        text_app_status.set("Err: can't save, no payload !");
        return;
    }
    ensure_directory(ook_editor_dir);
    auto new_path = ook_editor_dir / value + ".OOK";
    if (save_ook_to_file(new_path)) {
        text_app_status.set("Saved to " + new_path.string());
    } else {
        text_app_status.set("Error saving " + new_path.string());
    }
}

// update ook_data from GUI and save
bool OOKEditorAppView::save_ook_to_file(const std::filesystem::path& path) {
    update_ook_data_from_app();
    return save_ook_file(ook_data, path);
}

// Destructor for `OOKEditorAppView`: Disables the transmitter and shuts down the baseband
OOKEditorAppView::~OOKEditorAppView() {
    stop_ook_file_tx();
    baseband::shutdown();
}

// Constructor for `OOKEditorAppView`: Sets up the app view and initializes UI elements
OOKEditorAppView::OOKEditorAppView(NavigationView& nav)
    : nav_{nav} {
    // load OOK baseband
    baseband::run_image(portapack::spi_flash::image_tag_ook);

    // add all the widgets
    add_children({&field_frequency,
                  &tx_view,
                  &button_send_stop,
                  &label_step,
                  &field_step,
                  &label_sample_rate,
                  &field_sample_rate,
                  &label_symbol_rate,
                  &field_symbol_rate,
                  &label_symbol_rate_unit,
                  &text_payload,
                  &button_set,
                  &progressbar,
                  &label_repeat,
                  &field_repeat,
                  &label_pause_symbol_duration,
                  &field_pause_symbol_duration,
                  &label_pause_symbol_duration_unit,
                  &label_payload,
                  &text_app_status,
                  &label_waveform,
                  &waveform,
                  &button_open,
                  &button_save});

    // Initialize default values for controls
    field_symbol_rate.set_value(100);
    field_pause_symbol_duration.set_value(100);
    field_repeat.set_value(4);

    // Configure open ook file button
    button_open.on_select = [this](Button&) {
        auto open_view = nav_.push<FileLoadView>(".OOK");
        ensure_directory(ook_editor_dir);
        open_view->push_dir(ook_editor_dir);
        open_view->on_changed = [this](std::filesystem::path new_file_path) {
            // Postpone `on_file_changed` call until `FileLoadView` is closed
            nav_.set_on_pop([this, new_file_path]() {
                on_file_changed(new_file_path);
                button_send_stop.focus();
                draw_waveform();
            });
        };
    };

    // Configure save to ook file button
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
    field_symbol_rate.on_change = [this](int32_t) {
        text_app_status.set("");  // Clear loaded file text field
    };
    // clean out loaded file name if field is changed
    field_repeat.on_change = [this](int32_t) {
        text_app_status.set("");  // Clear loaded file text field
    };
    // clean out loaded file name if field is changed
    field_pause_symbol_duration.on_change = [this](int32_t) {
        text_app_status.set("");  // Clear loaded file text field
    };
    // clean out loaded file name if field is changed
    field_sample_rate.on_change = [this](size_t, int32_t) {
        text_app_status.set("");  // Clear loaded file text field
    };

    // setting up FrequencyField
    field_frequency.set_value(ook_editor_tx_freq);

    // clean out loaded file name if field is changed, save ook_editor_tx_freq
    field_frequency.on_change = [this](rf::Frequency f) {
        ook_editor_tx_freq = f;
        text_app_status.set("");  // Clear loaded file text field
    };

    // allow typing frequency number
    field_frequency.on_edit = [this]() {
        auto freq_view = nav_.push<FrequencyKeypadView>(field_frequency.value());
        freq_view->on_changed = [this](rf::Frequency f) {
            field_frequency.set_value(f);
            text_app_status.set("");  // Clear loaded file text field
        };
    };

    // allow different steps on symbol_rate and pause_symbol_duration
    field_step.on_change = [this](size_t, int32_t value) {
        text_app_status.set("");  // Clear loaded file text field
        field_symbol_rate.set_step(value);
        field_pause_symbol_duration.set_step(value);
    };

    // Configure button to manually set payload through text input
    button_set.on_select = [this, &nav](Button&) {
        text_prompt(
            nav,
            ook_data.payload,
            100,
            [this](std::string& s) {
                text_payload.set(s);
                draw_waveform();
                text_app_status.set("");  // Clear loaded file text field
            });
    };

    // Configure button to start or stop the transmission
    button_send_stop.on_select = [this](Button&) {
        if (!is_transmitting) {
            update_ook_data_from_app();
            start_tx();  // Begin transmission
        } else {
            stop_tx();
        }
    };

    // initial waveform drawing (should be a single line)
    draw_waveform();
}
}  // namespace ui::external_app::ook_editor
