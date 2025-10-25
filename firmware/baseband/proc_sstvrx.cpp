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

#include "proc_sstvrx.hpp"
#include "event_m4.hpp"
#include "utility.hpp"

#include <cstdint>
#include <array>
#include <cmath>

void SSTVRXProcessor::execute(const buffer_c8_t& buffer) {
    if (!status.configured) return;
    if (!buffer.p || buffer.count == 0) return;

    // Enforce safety limits with reduced ranges
    if (line_width > MAX_LINE_WIDTH ||
        samples_per_pixel > MAX_SAMPLES_PER_PIXEL ||
        sync_width > MAX_SYNC_WIDTH) {
        reset_rx_state();
        status.configured = 0;
        return;
    }

    // Process each sample using fixed-point math
    for (size_t i = 0; i < buffer.count; i++) {
        // Fast fixed-point arctangent approximation
        const int16_t y = buffer.p[i].imag();
        const int16_t x = buffer.p[i].real();
        int16_t angle;
        
        // Fast CORDIC-like angle calculation
        if (x == 0) {
            angle = (y >= 0) ? 0x4000 : -0x4000;  // ±π/2 in Q7.8
        } else {
            const int16_t abs_y = std::abs(y);
            const int16_t abs_x = std::abs(x);
            const int16_t min_val = std::min(abs_x, abs_y);
            const int16_t max_val = std::max(abs_x, abs_y);
            angle = ((min_val * 256) / max_val) * (max_val == abs_x ? 1 : -1);
            if (x < 0) angle = 0x8000 - angle;  // π - angle
            if (y < 0) angle = -angle;
        }

    // Detect frequency using zero-crossing and convert to fixed-point (Q8.4)
    uint16_t freq_fp = static_cast<uint16_t>(detect_tone(static_cast<float>(angle)) * 16.0f);

        // State machine for SSTV decoding
        switch (state) {
            case STATE_WAIT_SIGNAL:
                if (std::abs(static_cast<int>(freq_fp) - static_cast<int>(VIS_FREQ_1200_FP)) < static_cast<int>(FREQ_TOLERANCE_FP)) {
                    state = STATE_VIS_START;
                    vis_code = 0;
                            status.vis_bit_counter = 0;
                }
                break;

            case STATE_VIS_START:
                if (std::abs(freq_fp - VIS_FREQ_1200_FP) < FREQ_TOLERANCE_FP) {
                    state = STATE_VIS_BITS;
                    sample_count = 0;
                            status.vis_bit_counter++;
                    state = STATE_WAIT_SIGNAL;
                }
                break;

            case STATE_VIS_BITS:
                if (sample_count >= AUDIO_RATE / 33) {  // ~30ms per bit
                    process_vis_bit(static_cast<float>(freq_fp) / 16.0f);  // Convert back to float
                    sample_count = 0;
                    status.vis_bit_counter++;

                    if (status.vis_bit_counter >= 8) {
                        // VIS code complete, check if valid
                        bool found = false;
                        for (size_t m = 0; m < SSTV_MODES_NB; m++) {
                            if (sstv_modes[m].vis_code == vis_code) {
                                line_width = static_cast<uint8_t>(sstv_modes[m].pixels);
                                samples_per_pixel = static_cast<uint8_t>(sstv_modes[m].samples_per_pixel);
                                sync_width = static_cast<uint8_t>(sstv_modes[m].samples_per_sync);
                                found = true;
                                break;
                            }
                        }
                        
                        if (found) {
                            state = STATE_SYNC;
                            sync_count = 0;
                            pixel_count = 0;
                            line_count = 0;
                            // Notify UI of sync
                            shared_memory.application_queue.push(frame_sync_message);
                        } else {
                            state = STATE_WAIT_SIGNAL;
                        }
                    }
                }
                sample_count++;
                break;

            case STATE_SYNC:
                sync_count++;
                if (sync_count >= sync_width) {
                    state = STATE_PIXELS;
                    pixel_count = 0;
                }
                break;

            case STATE_PIXELS:
                // Convert frequency to pixel value (1500-2300Hz -> 0-255)
                // Use freq_fp as the detected frequency (Q8.4 -> convert to float)
                float freq_f = static_cast<float>(freq_fp) / 16.0f;
                uint8_t pixel_value = static_cast<uint8_t>((freq_f - 1500.0f) * 255.0f / 800.0f);
                
                // Bounds check before writing to pixel buffer
                if (pixel_count < line_message.pixel_data.size()) {
                    line_message.pixel_data[pixel_count++] = pixel_value;
                } else {
                    // Buffer overflow would have occurred, reset state
                    reset_rx_state();
                    break;
                }

                if (pixel_count >= line_width) {
                    // Line complete, send to application
                    send_line_data();
                    line_count++;
                    state = STATE_SYNC;
                    sync_count = 0;
                }
                break;
        }
    }
}

float SSTVRXProcessor::detect_tone(float sample) {
    // Fixed-point frequency detection using circular buffer
    baseband_buffer[buffer_pos] = sample > 0.0f ? 1 : -1;
    buffer_pos = (buffer_pos + 1) & (FIFO_SIZE - 1);  // Fast modulo for power of 2
    
    // Count zero crossings in buffer
    int8_t crossings = 0;
    for (uint8_t i = 0; i < FIFO_SIZE - 1; i++) {
        if (baseband_buffer[i] != baseband_buffer[i + 1]) {
            crossings++;
        }
    }
    
    // Convert to fixed-point frequency (local variable)
    const int local_freq = (crossings * (AUDIO_RATE << 4)) / FIFO_SIZE;
    return static_cast<float>(local_freq) / 16.0f; // Convert back to float (Q8.4)
}

void SSTVRXProcessor::process_vis_bit(float freq) {
    // Convert frequency to fixed point
    uint16_t freq_fp = static_cast<uint16_t>(freq * 16.0f);  // Q8.4 format
    
    // Sanity check for bit counter
        if (status.vis_bit_counter >= 8) {
        state = STATE_WAIT_SIGNAL;
        return;
    }

        if (std::abs(static_cast<int>(freq_fp) - static_cast<int>(VIS_FREQ_1100_FP)) < static_cast<int>(FREQ_TOLERANCE_FP)) {
            vis_code |= (1 << status.vis_bit_counter);
        } else if (std::abs(static_cast<int>(freq_fp) - static_cast<int>(VIS_FREQ_1300_FP)) < static_cast<int>(FREQ_TOLERANCE_FP)) {
            vis_code &= ~(1 << status.vis_bit_counter);
    } else {
        state = STATE_WAIT_SIGNAL;  // Invalid frequency
    }
}

void SSTVRXProcessor::send_line_data() {
    // Update line number for current message
    line_message.line_number = line_count;
    
    // Try to send the message
    if (!shared_memory.application_queue.push(line_message)) {
        // Queue is full, reset state to avoid buffer overflow
        reset_rx_state();
    }
    
    // Clear pixel data for next line
    std::fill(line_message.pixel_data.begin(), line_message.pixel_data.end(), 0);
}

void SSTVRXProcessor::reset_rx_state() {
    state = STATE_WAIT_SIGNAL;
    status = {};  // Clear all status bits at once
    vis_code = 0;
    sample_count = 0;
    sync_count = 0;
    pixel_count = 0;
    line_count = 0;
    line_width = 0;
    samples_per_pixel = 0;
    sync_width = 0;
    buffer_pos = 0;
    
    // Clear buffers
    baseband_buffer.fill(0);
    std::fill(line_message.pixel_data.begin(), line_message.pixel_data.end(), 0);
}

void SSTVRXProcessor::on_message(const Message* const msg) {
    if (!msg) return;  // Null check
    
    if (msg->id == Message::ID::SSTVConfigure) {
        const auto message = static_cast<const SSTVConfigureMessage*>(msg);
        if (!message) {
            status.configured = 0;
            return;
        }
        
        if (message->pixel_duration == 0) {
            status.configured = 0;  // Shutdown
            return;
        }
        
        // Reset state before configuring
        reset_rx_state();
        
        // Validate configuration
        if (line_width > MAX_LINE_WIDTH ||
            samples_per_pixel > MAX_SAMPLES_PER_PIXEL ||
            sync_width > MAX_SYNC_WIDTH) {
            status.configured = 0;
            return;
        }
        
        status.configured = 1;
    }
}

// Minimal entry point required by the baseband_sstvrx build target.
// This creates the processor and runs the event dispatcher.
int main() {
    EventDispatcher event_dispatcher{std::make_unique<SSTVRXProcessor>()};
    event_dispatcher.run();
    return 0;
}
