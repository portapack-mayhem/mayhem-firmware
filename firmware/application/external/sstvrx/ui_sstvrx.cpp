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

#include "ui_sstvrx.hpp"

#include "portapack_persistent_memory.hpp"
#include "portapack.hpp"
#include "hackrf_hal.hpp"
#include "file_path.hpp"
#include "message.hpp"
#include <cstdint>
#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace modems;
using namespace ui;

// SSTV RX View Implementation
namespace ui::external_app::sstvrx {

void SstvRxLogger::log_error(const std::string& error_message) {
    log_file.write_entry(rtc_time::now(), "ERROR: " + error_message);
}

void SstvRxLogger::log_info(const std::string& info_message) {
    log_file.write_entry(rtc_time::now(), "INFO: " + info_message);
}

SstvRxView::SstvRxView(ui::NavigationView& nav)
    : nav_(nav) {
    
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    
    add_children({
        &field_rf_amp,
        &field_lna,
        &field_vga,
        &rssi,
        &field_frequency,
        &field_volume,
        &audio,
        &start_btn,
        &stop_btn,
        &options_mode,
        &labels
    });
    
    // Initialize audio with proper rate for SSTV
    audio::set_rate(audio::Rate::Hz_24000);
    audio::output::start();
    
    // Configure receiver with optimal settings for SSTV
    // NOTE: Do NOT set modulation mode - SSTV uses a custom baseband processor
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(16000);  // Wide enough for SSTV but not too wide
    receiver_model.set_squelch_level(0);
    receiver_model.set_hidden_offset(2500);  // Small offset to avoid DC spike but minimize distortion
    
    // Set initial control values
    field_vga.set_value(40);
    field_lna.set_value(32);
    field_rf_amp.set_value(false);
    field_volume.set_value(35);

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

    logger = std::make_unique<SstvRxLogger>();
    if (logger) {
        logger->append(logs_dir / "SSTVRX.txt");
        logger->log_info("SSTV RX Started");
    }
}

// Destructor: Ensure reception is stopped
SstvRxView::~SstvRxView() {
    is_receiving = true;
    on_stop();
    baseband::shutdown();
    if (logger) {
        logger->log_info("SSTV RX Stopped");
    }
}

void SstvRxView::on_show() {
    // No additional initialization needed
}

void SstvRxView::focus() {
    field_frequency.focus();
}

// Start NFM audio reception if not already started
void SstvRxView::on_start() {
    if (logger) {
        logger->log_info("Starting SSTV RX Reception");
    }
    
    if (!is_receiving) {
        start_audio();
        return;
    } else if (logger) {
        logger->log_error("SSTV RX Reception Already Started");
    }
}

// Stop NFM audio reception
void SstvRxView::on_stop() {
    if (logger) {
        logger->log_info("Stopping SSTV RX Reception");
    }
    if (is_receiving) {
        // Stop in reverse order of start
        receiver_model.disable();
        audio::output::stop();
        
        // Reset state
        is_receiving = false;
        
        // Clear the display area
        portapack::display.fill_rectangle(
            {0, SSTV_IMG_START_ROW * 16, DISPLAY_WIDTH, DISPLAY_HEIGHT},
            {0, 0, 0}  // Black
        );
        line_num = 0;
        
        // Close image file if still open
        if (image_file) {
            image_file.reset();
        }
        
        if (logger) {
            logger->log_info("SSTV RX Reception Stopped");
        }
    } else if (logger) {
        logger->log_error("SSTV RX Reception Not Running");
    }
}

// Start NFM audio reception
void SstvRxView::start_audio() {
    if (logger) {
        logger->log_info("Configuring SSTV RX Audio Reception");
    }
    
    // Configure the baseband processor with VIS code
    if (rx_sstv_mode) {
        baseband::set_sstvrx_data(rx_sstv_mode->vis_code);
        if (logger) {
            logger->log_info("Sent VIS code to processor: " + to_string_dec_uint(rx_sstv_mode->vis_code));
        }
    }
    
    // Initialize audio path
    audio::output::stop();
    audio::set_rate(audio::Rate::Hz_24000);
    
    // Set audio routing and volume
    //audio::output::start();
    
    // Clear display area and reset line counter
    portapack::display.fill_rectangle(
        {0, SSTV_IMG_START_ROW * 16, DISPLAY_WIDTH, DISPLAY_HEIGHT},
        {0, 0, 0}  // Black
    );
    line_num = 0;
    
    // Initialize new image file
    current_line_rx = 0;
    auto timestamp = to_string_timestamp(rtc_time::now());
    auto dir_error = ensure_directory(sstv_dir / "RX");
    if (!dir_error.ok()) {
        if (logger) {
            logger->log_error("Failed to create directory: SSTV/RX");
        }
    }
    current_image_path = sstv_dir / ("RX/SSTV_" + timestamp + ".bmp");
    
    image_file = std::make_unique<File>();
    auto error = image_file->create(current_image_path);
    if (error) {
        if (logger) {
            logger->log_error("Failed to create file: " + current_image_path.string());
        }
        image_file.reset();
    } else {
        // Write BMP header immediately
        write_bmp_header();
        if (logger) {
            logger->log_info("Created image file: " + current_image_path.string());
        }
    }
    
    // Start audio output
    audio::output::start();
    audio::headphone::set_volume(persistent_memory::headphone_volume());
    
    // Enable receiver last
    receiver_model.enable();
    is_receiving = true;

    if (logger) {
        logger->log_info("SSTV RX Started");
    }
}

void SstvRxView::on_mode_changed(const size_t index) {
    rx_sstv_mode = &sstv_modes[index];
}

void SstvRxView::update_display(uint16_t current_line, const uint8_t* data_ptr) {
    if (current_line >= IMAGE_HEIGHT) return;
    
    // Reset line counter if we reach the bottom of display
    if (line_num >= DISPLAY_HEIGHT) {
        line_num = 0;
    }
    
    // Scale line to display width
    for (uint16_t x = 0; x < DISPLAY_WIDTH; x++) {
        // Scale x coordinate
        uint16_t src_x = (x * IMAGE_WIDTH) / DISPLAY_WIDTH;
        if (src_x >= IMAGE_WIDTH) continue;
        
        // Get RGB values and create color
        uint8_t r = data_ptr[2 + src_x];
        uint8_t g = data_ptr[2 + PIXELS_PER_LINE + src_x];
        uint8_t b = data_ptr[2 + PIXELS_PER_LINE * 2 + src_x];
        line_buffer[x] = {r, g, b};  // Color constructor handles conversion
    }
    
    // Render the line at the current position
    portapack::display.render_line(
        {0, line_num + SSTV_IMG_START_ROW * 16},
        DISPLAY_WIDTH,
        line_buffer
    );
    
    // Increment line counter
    line_num++;
}

void SstvRxView::on_progress(uint16_t line, uint16_t total_lines) {
    // Handle debug messages
    if (line == 0xFFFF) {
        if (logger) {
            logger->log_error("Processor not configured");
        }
        return;
    }
    if (line == 0xFFFE) {
        if (logger) {
            logger->log_info("Audio peak level: " + to_string_dec_uint(total_lines));
        }
        return;
    }
    
    // Normal line processing
    current_line_rx = line;
    
    // Read line data from shared memory
    uint8_t* data_ptr = shared_memory.bb_data.data;
    
    // Extract line number
    uint16_t line_num = data_ptr[0] | (data_ptr[1] << 8);
    
    // Write line directly to file (BMP is bottom-up)
    if (image_file && line_num < IMAGE_HEIGHT) {
        uint8_t row_data[IMAGE_WIDTH * 3];
        
        // BMP stores as BGR
        for (uint16_t x = 0; x < IMAGE_WIDTH; x++) {
            row_data[x * 3 + 0] = data_ptr[2 + PIXELS_PER_LINE * 2 + x]; // B
            row_data[x * 3 + 1] = data_ptr[2 + PIXELS_PER_LINE + x];     // G
            row_data[x * 3 + 2] = data_ptr[2 + x];                        // R
        }
        
        // Seek to correct position (BMP is bottom-up, so invert line number)
        uint32_t line_offset = 54 + ((IMAGE_HEIGHT - 1 - line_num) * IMAGE_WIDTH * 3);
        image_file->seek(line_offset);
        image_file->write(row_data, IMAGE_WIDTH * 3);
        
        // Update live display
        update_display(line_num, data_ptr);
    }
    
    if (logger && (line % 10 == 0)) {  // Log every 10 lines to reduce overhead
        logger->log_info("Line " + to_string_dec_uint(line) + "/" + to_string_dec_uint(total_lines));
    }
    
    // When image is complete, close file
    if (line >= (total_lines - 1)) {
        finish_image();
    }
}

void SstvRxView::write_bmp_header() {
    if (!image_file) return;
    
    // BMP Header (14 bytes)
    uint32_t file_size = 54 + (IMAGE_WIDTH * IMAGE_HEIGHT * 3);
    uint8_t bmp_header[54] = {
        'B', 'M',  // Signature
        static_cast<uint8_t>(file_size), static_cast<uint8_t>(file_size >> 8),
        static_cast<uint8_t>(file_size >> 16), static_cast<uint8_t>(file_size >> 24),  // File size
        0, 0, 0, 0,  // Reserved
        54, 0, 0, 0,  // Pixel data offset
        
        // DIB Header (40 bytes)
        40, 0, 0, 0,  // Header size
        static_cast<uint8_t>(IMAGE_WIDTH), static_cast<uint8_t>(IMAGE_WIDTH >> 8), 0, 0,  // Width
        static_cast<uint8_t>(IMAGE_HEIGHT), static_cast<uint8_t>(IMAGE_HEIGHT >> 8), 0, 0,  // Height
        1, 0,  // Color planes
        24, 0,  // Bits per pixel
        0, 0, 0, 0,  // Compression (none)
        0, 0, 0, 0,  // Image size (can be 0 for uncompressed)
        0, 0, 0, 0,  // X pixels per meter
        0, 0, 0, 0,  // Y pixels per meter
        0, 0, 0, 0,  // Colors in palette
        0, 0, 0, 0   // Important colors
    };
    
    image_file->write(bmp_header, 54);
    
    // Pre-fill with black pixels
    uint8_t black_row[IMAGE_WIDTH * 3];
    memset(black_row, 0, sizeof(black_row));
    for (uint16_t y = 0; y < IMAGE_HEIGHT; y++) {
        image_file->write(black_row, IMAGE_WIDTH * 3);
    }
}

void SstvRxView::finish_image() {
    if (image_file) {
        image_file.reset();
        if (logger) {
            logger->log_info("Image completed: " + current_image_path.string());
        }
    }
}

void SstvRxView::save_image() {
    // Deprecated - now using incremental write in on_progress
}

}  // namespace ui::external_app::sstvrx