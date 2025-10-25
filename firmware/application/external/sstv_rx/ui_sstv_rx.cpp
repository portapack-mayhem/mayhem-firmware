/*
 * Copyright (C) 2025 StarVore Labs
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

#include "ui_sstv_rx.hpp"

#include "portapack_persistent_memory.hpp"
#include "portapack.hpp"
#include "hackrf_hal.hpp"
#include <cstdint>
#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace modems;

namespace ui::external_app::sstv_rx {

// Radio configuration
constexpr uint32_t sampling_rate = 3072000;  // 3.072MHz
constexpr uint32_t baseband_bandwidth = 1750000; // 1.75MHz

void SstvRxView::init_temp_file() {
    // Create temp directory if it doesn't exist
    ensure_directory("/SSTV/TEMP");
    
    logger->log('D');

    // Generate temp filename with timestamp
    auto time = rtc_time::now();
    char filename[32];
    sprintf(filename, "/SSTV/TEMP/%04d%02d%02d_%02d%02d%02d.tmp",
            time.year(), time.month(), time.day(),
            time.hour(), time.minute(), time.second());
    
    temp_path = std::filesystem::path(filename);
    
    // Create and open temp file
    if (auto error = temp_file.create(temp_path)) {
        cleanup_temp_file();
        return;
    }
}

void SstvRxView::cleanup_temp_file() {
    temp_file.close();
    if (!temp_path.empty()) {
        delete_file(temp_path);
        temp_path = std::filesystem::path();
    }
}

SstvRxView::SstvRxView(ui::NavigationView& nav)
    : nav_(nav) {
    // Initialize logger
    logger = std::make_unique<SSTVLogger>();
    logger->append("/LOGS/SSTV_RX.txt");
    logger->log('S', 0);
    
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    add_children({
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &field_frequency,
        &field_volume,
        &field_bw,
        &audio,
        &start_btn,
        &stop_btn,
        &save_btn,
        &options_mode,
        &labels
    });

    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t mode_options;
    uint32_t c;

    // Start button handlers
    start_btn.on_select = [this](Button&) {
        start_btn.focus();
        on_start();
    };

    // Stop button handlers
    stop_btn.on_select = [this](Button&) {
        stop_btn.focus();
        on_stop();
    };

    // Save button handlers
    save_btn.on_select = [this](Button&) {
        save_btn.focus();
        save_image();
    };

    // Initialize frequency field
    if (field_frequency.value() == 0) {
        field_frequency.set_value(96100000);  // Default to 96.100 MHz
    }
    field_frequency.set_step(25000);

    // Populate mode list
    for (c = 0; c < SSTV_MODES_NB; c++)
        mode_options.emplace_back(sstv_modes[c].name, c);
    options_mode.set_options(mode_options);

    options_mode.on_change = [this](size_t i, int32_t) {
        this->on_mode_changed(i);
    };
    options_mode.set_selected_index(1);  // Scottie 2
    on_mode_changed(1);
}

SstvRxView::~SstvRxView() {
    on_stop();
    cleanup_temp_file();
}

void SstvRxView::on_show() {
    return;
}

void SstvRxView::on_message(const Message* const message) {
    if (!message) return;

    switch(message->id) {
        case Message::ID::SSTVLine:
            handle_line(*reinterpret_cast<const SSTVLineMessage*>(message));
            break;

        case Message::ID::RequestSignal: {
            const auto sig_msg = reinterpret_cast<const RequestSignalMessage*>(message);
            if (sig_msg->signal == RequestSignalMessage::Signal::FrameSync) {
                handle_frame_sync();
            }
            break;
        }

        default:
            break;
    }
}

void SstvRxView::focus() {
    field_frequency.focus();
}

void SstvRxView::on_mode_changed(const size_t index) {
    rx_sstv_mode = &sstv_modes[index];
    
    logger->log('M', index);
    
    // Reset state when mode changes
    cleanup_temp_file();
    line_buffer.fill(0);
    image_ready = false;
    current_line = 0;
    
    // Initialize new temp file for the new mode
    init_temp_file();
}

void SstvRxView::on_start() {
    logger->log('R');
    on_stop();
    start_audio();
}

void SstvRxView::on_stop() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void SstvRxView::start_audio() {
    if (!rx_sstv_mode) {
        logger->log('X');
        return;
    }

    // Configure receiver
    receiver_model.set_sampling_rate(sampling_rate);
    receiver_model.set_baseband_bandwidth(baseband_bandwidth);
    receiver_model.set_modulation(ReceiverModel::Mode::NarrowbandFMAudio);
    field_bw.set_by_value(2);  // 16k default
    receiver_model.set_nbfm_configuration(field_bw.selected_index_value());
    field_bw.on_change = [this](size_t index, OptionsField::value_t n) {
        radio_bw = index;
        receiver_model.set_nbfm_configuration(n);
    };
    receiver_model.enable();

        // Initialize processor after receiver is configured
        baseband::run_prepared_image(portapack::memory::map::m4_code.base());

        // Wait a moment for processor to initialize
        chThdSleepMilliseconds(100);

        // Configure SSTV RX processor
        const auto message = SSTVConfigureMessage {
            static_cast<uint8_t>(rx_sstv_mode->vis_code),
            static_cast<uint32_t>(rx_sstv_mode->samples_per_pixel)
        };
        shared_memory.application_queue.push(message);

        // Set audio sampling rate and start output
        audio_sampling_rate = audio::Rate::Hz_48000;
        audio::set_rate(audio_sampling_rate);
        audio::output::start();
    }

    void SstvRxView::handle_line(const SSTVLineMessage& message) {
        if (!rx_sstv_mode || !temp_file.is_ready() || current_line >= rx_sstv_mode->lines) {
            logger->log('E');
            return;
    }

    // Write line data to temp file
    temp_file.write(message.pixel_data.data(), rx_sstv_mode->pixels * 3);
    
    logger->log('L', current_line & 0xFF);
    
    current_line++;
    if (current_line >= rx_sstv_mode->lines) {
        temp_file.sync();
        image_ready = true;
    }
}

void SstvRxView::handle_frame_sync() {
    logger->log('S');
    cleanup_temp_file();
    init_temp_file();
    current_line = 0;
    image_ready = false;
}

void SstvRxView::save_image() {
    if (!image_ready || !temp_file.is_ready()) {
        nav_.display_modal("Error", "No decoded image\navailable to save.");
        return;
    }

    // Generate timestamp-based filename
    auto time = rtc_time::now();
    std::filesystem::path sstv_dir{u"/SSTV"};
    char filename_buf[32];
    sprintf(filename_buf, "%04d%02d%02d_%02d%02d%02d.bmp",
            time.year(), time.month(), time.day(),
            time.hour(), time.minute(), time.second());
    
    std::filesystem::path filename = sstv_dir / std::filesystem::path(filename_buf);

    // Ensure SSTV directory exists
    ensure_directory(sstv_dir);

    // Create BMP file
    File image_file;
    image_file.create(filename);

    // Write BMP header
    BMPHeader header;
    header.signature = 0x4D42;  // "BM"
    header.file_size = sizeof(BMPHeader) + (rx_sstv_mode->pixels * rx_sstv_mode->lines * 3);
    header.reserved = 0;
    header.data_offset = sizeof(BMPHeader);
    header.header_size = 40;
    header.width = rx_sstv_mode->pixels;
    header.height = rx_sstv_mode->lines;
    header.planes = 1;
    header.bits_per_pixel = 24;
    header.compression = 0;
    header.image_size = rx_sstv_mode->pixels * rx_sstv_mode->lines * 3;
    header.x_pixels_per_meter = 2835;
    header.y_pixels_per_meter = 2835;
    header.colors_used = 0;
    header.important_colors = 0;

    image_file.write(&header, sizeof(header));

    // Rewind temp file to start
    temp_file.seek(0);

    // Write image data (BGR format for BMP)
    const size_t row_size = rx_sstv_mode->pixels * 3;
    
    // BMP images are stored bottom-up, so we need to copy rows in reverse
    std::vector<uint32_t> row_offsets(rx_sstv_mode->lines);
    for (int32_t y = 0; y < rx_sstv_mode->lines; y++) {
        row_offsets[y] = y * row_size;
    }

    // Write rows in reverse order
    for (int32_t y = rx_sstv_mode->lines - 1; y >= 0; y--) {
        // Seek to the correct row in temp file
        temp_file.seek(row_offsets[y]);
        
        // Read RGB data
        temp_file.read(line_buffer.data(), row_size);

        // Convert RGB to BGR
        for (size_t x = 0; x < rx_sstv_mode->pixels; x++) {
            size_t idx = x * 3;
            std::swap(line_buffer[idx], line_buffer[idx + 2]);
        }
        
        // Write BGR data
        image_file.write(line_buffer.data(), row_size);

    }

    // Sync and close the files
    image_file.sync();
    image_file.close();

    nav_.display_modal("Success", "Image saved as " + filename.string());
    
    // Clean up temp file after successful save
    cleanup_temp_file();
}

} // namespace ui::external_app::sstv_rx