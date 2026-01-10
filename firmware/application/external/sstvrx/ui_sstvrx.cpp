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
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace modems;
using namespace ui;

#if SSTVRX_ENABLE_LOGGER
#define SSTVRX_LOG_INFO(msg)       \
    do {                           \
        if (logger) {              \
            logger->log_info(msg); \
        }                          \
    } while (0)
#define SSTVRX_LOG_ERROR(msg)       \
    do {                            \
        if (logger) {               \
            logger->log_error(msg); \
        }                           \
    } while (0)
#else
#define SSTVRX_LOG_INFO(msg) \
    do {                     \
        (void)sizeof(msg);   \
    } while (0)
#define SSTVRX_LOG_ERROR(msg) \
    do {                      \
        (void)sizeof(msg);    \
    } while (0)
#endif

// SSTV RX View Implementation
namespace ui::external_app::sstvrx {

static_assert(sizeof(shared_memory.bb_data.data) == 512,
              "SSTV shared buffer size mismatch");

#if SSTVRX_ENABLE_LOGGER
void SstvRxLogger::log_error(const std::string& error_message) {
    log_file.write_entry(rtc_time::now(), "ERROR: " + error_message);
}

void SstvRxLogger::log_info(const std::string& info_message) {
    log_file.write_entry(rtc_time::now(), "INFO: " + info_message);
}
#endif

SstvRxView::SstvRxView(ui::NavigationView& nav)
    : nav_(nav) {
    baseband::run_prepared_image(portapack::memory::map::m4_code.base());
    DISPLAY_HEIGHT = screen_height - SSTV_IMG_START_ROW * 16 - 16;
    DISPLAY_WIDTH = screen_width;
    add_children({&field_rf_amp,
                  &field_lna,
                  &field_vga,
                  &rssi,
                  &channel,
                  &field_frequency,
                  &field_volume,
                  &audio,
                  &start_stop_btn,
                  &options_mode,
                  &field_phase,
                  &field_slant,
                  &labels,
                  &text_calibration});

    // Initialize audio with proper rate for SSTV
    audio::set_rate(audio::Rate::Hz_48000);
    audio::output::start();

    // Configure receiver with optimal settings for SSTV
    // NOTE: Do NOT set modulation mode - SSTV uses a custom baseband processor
    // Standard sampling rate (3.072MHz) with wide bandwidth to capture full SSTV audio spectrum
    // SSTV uses 1200-2300 Hz tones, so we need wide baseband to avoid distortion
    receiver_model.set_sampling_rate(3072000);
    receiver_model.set_baseband_bandwidth(1750000);  // Standard wideband setting
    receiver_model.set_squelch_level(1);
    receiver_model.set_hidden_offset(0);  // No offset needed

    // Field values will be set in on_show() to ensure proper initialization

    using option_t = std::pair<std::string, int32_t>;
    using options_t = std::vector<option_t>;
    options_t mode_options;
    uint32_t c;

    // Start/Stop button handler - toggles between start and stop
    start_stop_btn.on_select = [this](Button&) {
        start_stop_btn.focus();
        on_start_stop();
    };

    // Initialize frequency field from settings or use default
    if (settings_.loaded() && settings_.raw().rx_frequency != 0) {
        field_frequency.set_value(settings_.raw().rx_frequency);
    } else if (field_frequency.value() == 0) {
        field_frequency.set_value(145800000);  // Default to 145.800 MHz (ISS)
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

    // Initialize phase and slant controls from loaded settings
    field_phase.set_value(phase_adjustment);
    field_phase.on_change = [this](int32_t v) {
        phase_adjustment = v;
        if (is_receiving) {
            baseband::set_sstvrx_phase_slant(phase_adjustment, slant_adjustment);
        } else if (max_received_line > 0) {
            // Auto-redraw when adjusting after reception
            redraw_image();
        }
    };

    field_slant.set_value(slant_adjustment);
    field_slant.on_change = [this](int32_t v) {
        slant_adjustment = v;
        if (is_receiving) {
            baseband::set_sstvrx_phase_slant(phase_adjustment, slant_adjustment);
        } else if (max_received_line > 0) {
            // Auto-redraw when adjusting after reception
            redraw_image();
        }
    };

#if SSTVRX_ENABLE_LOGGER
    logger = std::make_unique<SstvRxLogger>();
    if (logger) {
        logger->append(logs_dir / "SSTVRX.txt");
        logger->log_info("----------SSTV RX Started----------");
    }
#endif
}

// Destructor: Ensure reception is stopped
SstvRxView::~SstvRxView() {
    is_receiving = true;
    on_stop();
    baseband::shutdown();
    SSTVRX_LOG_INFO("SSTV RX Stopped");
}

void SstvRxView::on_show() {
    // Update field values from receiver model to reflect loaded settings
    field_lna.set_value(receiver_model.lna());
    field_vga.set_value(receiver_model.vga());
    field_rf_amp.set_value(receiver_model.rf_amp());
    field_volume.set_value(receiver_model.normalized_headphone_volume());
}

void SstvRxView::focus() {
    field_frequency.focus();
}

// Combined start/stop handler - toggles based on current state
void SstvRxView::on_start_stop() {
    if (is_receiving) {
        // Currently receiving - stop it
        on_stop();
        start_stop_btn.set_text("Start RX");
    } else {
        // Currently stopped - start reception
        SSTVRX_LOG_INFO("Starting SSTV RX Reception");
        start_audio();
        start_stop_btn.set_text("Stop RX");
    }
}

// Stop NFM audio reception
void SstvRxView::on_stop() {
    SSTVRX_LOG_INFO("Stopping SSTV RX Reception");
    if (is_receiving) {
        // Stop in reverse order of start
        receiver_model.disable();
        audio::output::stop();

        // Reset state
        is_receiving = false;

        // Close image file if still open
        bmp.close();

        pending_line_valid = false;
        pending_chunk_mask = 0;
        std::fill(pending_line_rgb.begin(), pending_line_rgb.end(), 0);
        *reinterpret_cast<volatile uint8_t*>(&shared_memory.bb_data.data[CHUNK_FLAG_INDEX]) = 0;

        SSTVRX_LOG_INFO("SSTV RX Reception Stopped");
    } else {
        SSTVRX_LOG_ERROR("SSTV RX Reception Not Running");
    }
}

// Start NFM audio reception
void SstvRxView::start_audio() {
    SSTVRX_LOG_INFO("Configuring SSTV RX Audio Reception");

    // Configure the baseband processor with VIS code
    if (rx_sstv_mode) {
        baseband::set_sstvrx_data(rx_sstv_mode->vis_code);
        SSTVRX_LOG_INFO("Sent VIS code to processor: " + to_string_dec_uint(rx_sstv_mode->vis_code));
    }

    // Send phase and slant adjustments
    baseband::set_sstvrx_phase_slant(phase_adjustment, slant_adjustment);

    // Initialize audio path
    audio::output::stop();
    audio::set_rate(audio::Rate::Hz_48000);

    // Set audio routing and volume
    // audio::output::start();

    // Clear display area and reset line counter
    portapack::display.fill_rectangle(
        {0, SSTV_IMG_START_ROW * 16, DISPLAY_WIDTH, DISPLAY_HEIGHT},
        {0, 0, 0});
    line_num = 0;
    file_line_num = 0;
    max_received_line = 0;
    pending_line_valid = false;
    pending_chunk_mask = 0;
    std::fill(pending_line_rgb.begin(), pending_line_rgb.end(), 0);
    *reinterpret_cast<volatile uint8_t*>(&shared_memory.bb_data.data[CHUNK_FLAG_INDEX]) = 0;

    // Clear calibration display
    text_calibration.set("Calibrating...");

    // Initialize new image file
    current_line_rx = 0;
    auto timestamp = to_string_timestamp(rtc_time::now());
    auto dir_error = ensure_directory(sstv_dir / "RX");
    if (!dir_error.ok()) {
        SSTVRX_LOG_ERROR("Failed to create directory: SSTV/RX");
    }
    current_image_path = sstv_dir / ("RX/SSTV_" + timestamp + ".bmp");
    auto ok = bmp.create(current_image_path, IMAGE_WIDTH, 1);
    if (!ok) {
        SSTVRX_LOG_ERROR("Failed to create file: " + current_image_path.string());
        bmp.close();
    }

    // Start audio output
    audio::output::start();
    audio::headphone::set_volume(persistent_memory::headphone_volume());

    // Keep ReceiverModel in capture mode so it doesn't override our custom baseband/audio configuration.
    receiver_model.set_modulation(ReceiverModel::Mode::Capture);

    // Enable receiver last
    receiver_model.enable();
    is_receiving = true;
    SSTVRX_LOG_INFO("SSTV RX Started");
}

void SstvRxView::on_mode_changed(const size_t index) {
    rx_sstv_mode = &sstv_modes[index];
}

void SstvRxView::write_line_to_file(uint16_t line_num, const uint8_t* rgb_line) {
    (void)line_num;
    if (!bmp.is_loaded()) return;
    // Ensure BMP height is sufficient
    if (bmp.get_real_height() <= file_line_num) {
        bmp.expand_y(file_line_num + 1);
    }
    bmp.seek(0, file_line_num);

    // Write RGB data in BGR order
    for (uint16_t x = 0; x < IMAGE_WIDTH; x++) {
        uint8_t r = rgb_line[x * 3 + 0];
        uint8_t g = rgb_line[x * 3 + 1];
        uint8_t b = rgb_line[x * 3 + 2];
        Color px(g, b, r);
        bmp.write_next_px(px);
    }
    file_line_num++;
}

void SstvRxView::update_display(uint16_t current_line, const uint8_t* rgb_line) {
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

        // Get RGB values from interleaved data [R,G,B,R,G,B,...]
        uint8_t r = rgb_line[src_x * 3 + 0];
        uint8_t g = rgb_line[src_x * 3 + 1];
        uint8_t b = rgb_line[src_x * 3 + 2];
        // Display uses BGR order like BMP format
        // line_buffer[x] = Color(b, r, g);
        line_buffer[x] = Color(g, b, r);
    }

    // Render the line at the current position
    portapack::display.render_line(
        {0, line_num + SSTV_IMG_START_ROW * 16},
        DISPLAY_WIDTH,
        line_buffer);

    // Increment line counter
    line_num++;
}
void SstvRxView::redraw_image() {
    // Disabled: Post-reception redraw requires 245KB buffer which exceeds M0 memory
    // Phase and slant adjustments must be set before reception starts
}

void SstvRxView::on_progress(uint16_t line, uint16_t total_lines) {
    if (!is_receiving) {
        if (line < 0xFFF0) {
            *reinterpret_cast<volatile uint8_t*>(&shared_memory.bb_data.data[CHUNK_FLAG_INDEX]) = 0;
        }
        return;
    }

    // Handle debug messages
    if (line == 0xFFFF) {
        SSTVRX_LOG_ERROR("Processor not configured");
        return;
    }
    if (line == 0xFFFE) {
        SSTVRX_LOG_INFO("Sync pulse duration: " + to_string_dec_uint(total_lines) + " samples");
        return;
    }
    if (line == 0xFFFD) {
        SSTVRX_LOG_INFO("Sync detected, count=" + to_string_dec_uint(total_lines));
        text_calibration.set("Syncs: " + to_string_dec_uint(total_lines));
        return;
    }
    if (line == 0xFFFC) {
        SSTVRX_LOG_INFO("Expected interval: " + to_string_dec_uint(total_lines) + " samples");
        return;
    }
    if (line == 0xFFFB) {
        SSTVRX_LOG_INFO("Actual interval: " + to_string_dec_uint(total_lines) + " samples");
        return;
    }
    if (line == 0xFFFA) {
        SSTVRX_LOG_INFO("SYNC TIMEOUT after " + to_string_dec_uint(total_lines) + " samples - continuing without sync");
        return;
    }
    if (line == 0xFFF9) {
        SSTVRX_LOG_INFO("Detected frequency at sync: " + to_string_dec_uint(total_lines) + " Hz");
        return;
    }
    if (line == 0xFFF8) {
        SSTVRX_LOG_INFO("OUTLIER REJECTED: interval=" + to_string_dec_uint(total_lines) + " samples (expected 10000-15000)");
        return;
    }
    if (line == 0xFFF7) {
        SSTVRX_LOG_INFO("PRE-RECORD: sync_history_count=" + to_string_dec_uint(total_lines));
        return;
    }
    if (line == 0xFFF6) {
        SSTVRX_LOG_INFO("MAX_SYNC_HISTORY EXCEEDED: count=" + to_string_dec_uint(total_lines));
        return;
    }
    if (line == 0xFFF5) {
        // Decode rejection reason bit flags: 0x1=interval, 0x2=freq, 0x4=duration
        std::string reasons = "";
        if (total_lines & 0x1) reasons += "interval ";
        if (total_lines & 0x2) reasons += "freq ";
        if (total_lines & 0x4) reasons += "duration ";
        if (reasons.empty()) reasons = "unknown";
        SSTVRX_LOG_INFO("FALSE SYNC PAIR REJECTED on Line 0: " + reasons);
        return;
    }
    if (line == 0xFFF4) {
        SSTVRX_LOG_INFO("STARTING LINE 0 DECODE after sync_sample_count=" + to_string_dec_uint(total_lines));
        return;
    }

    std::array<uint8_t, CHUNK_COPY_BYTES> chunk{};
    memcpy(chunk.data(), shared_memory.bb_data.data, chunk.size());
    *reinterpret_cast<volatile uint8_t*>(&shared_memory.bb_data.data[CHUNK_FLAG_INDEX]) = 0;

    const uint16_t line_num_encoded = chunk[0] | (chunk[1] << 8);
    const bool is_second_chunk = (line_num_encoded & 1) == 1;
    const uint16_t actual_line_num = line_num_encoded / 2;

    if (actual_line_num >= IMAGE_HEIGHT) {
        return;
    }

    const bool multi_chunk_line = PIXELS_PER_LINE > MAX_CHUNK_PIXELS;
    if (!pending_line_valid || pending_line_number != actual_line_num) {
        pending_line_number = actual_line_num;
        pending_line_valid = true;
        pending_chunk_mask = 0;
        std::fill(pending_line_rgb.begin(), pending_line_rgb.end(), 0);
    }

    const uint16_t chunk_pixels = multi_chunk_line
                                      ? (is_second_chunk ? (PIXELS_PER_LINE - MAX_CHUNK_PIXELS) : MAX_CHUNK_PIXELS)
                                      : PIXELS_PER_LINE;
    const uint16_t dest_pixel_offset = (multi_chunk_line && is_second_chunk) ? MAX_CHUNK_PIXELS : 0;
    const uint16_t chunk_bytes = chunk_pixels * 3;
    const uint16_t max_copy_bytes = static_cast<uint16_t>(chunk.size() > CHUNK_HEADER_BYTES ? (chunk.size() - CHUNK_HEADER_BYTES) : 0);
    const uint16_t bytes_to_copy = (chunk_bytes < max_copy_bytes) ? chunk_bytes : max_copy_bytes;
    const uint16_t dest_byte_offset = dest_pixel_offset * 3;

    if (bytes_to_copy > 0 && (dest_byte_offset + bytes_to_copy) <= pending_line_rgb.size()) {
        memcpy(pending_line_rgb.data() + dest_byte_offset, chunk.data() + CHUNK_HEADER_BYTES, bytes_to_copy);
    }

    const uint8_t chunk_bit = (multi_chunk_line && is_second_chunk) ? 0x2 : 0x1;
    pending_chunk_mask |= chunk_bit;
    const uint8_t required_mask = multi_chunk_line ? 0x3 : 0x1;
    if (pending_chunk_mask != required_mask) {
        return;
    }

    pending_line_valid = false;
    pending_chunk_mask = 0;
    current_line_rx = actual_line_num;
    if (actual_line_num < IMAGE_HEIGHT) {
        write_line_to_file(actual_line_num, pending_line_rgb.data());
        update_display(actual_line_num, pending_line_rgb.data());
        max_received_line = max_received_line > (actual_line_num + 1) ? max_received_line : (actual_line_num + 1);
    }

#if SSTVRX_ENABLE_LOGGER
    if (logger && (actual_line_num % 10 == 0)) {
        logger->log_info("Line " + to_string_dec_uint(actual_line_num) + "/" + to_string_dec_uint(total_lines));
    }
#endif

    // if (actual_line_num >= (total_lines - 1)) { //don't auto finish image upon end, user need to manually stop. this method is not reliable enough.
    //     finish_image();
    // }
}

void SstvRxView::finish_image() {
    bmp.close();
    SSTVRX_LOG_INFO("Image completed: " + current_image_path.string());
}

void SstvRxView::on_calibration(int16_t suggested_phase, int16_t suggested_slant, uint16_t sync_count) {
    if (!is_receiving) return;

    // Display calibration suggestions to the user
    if (sync_count >= 4) {
        std::string cal_text = "Try Slant=" + to_string_dec_int(suggested_slant);
        text_calibration.set(cal_text);
        text_calibration.set_dirty();

        // Don't auto-apply yet - just suggest for now until we verify the values are correct
        // User can manually adjust if needed

        // Log the suggestion
        SSTVRX_LOG_INFO("Calibration suggestion: phase=" + to_string_dec_int(suggested_phase) +
                        " slant=" + to_string_dec_int(suggested_slant) +
                        " (from " + to_string_dec_uint(sync_count) + " syncs)");
    } else {
        // Log that we received calibration but not enough syncs yet
        SSTVRX_LOG_INFO("Calibration received: " + to_string_dec_uint(sync_count) + " syncs (need 4+)");
    }
}

}  // namespace ui::external_app::sstvrx