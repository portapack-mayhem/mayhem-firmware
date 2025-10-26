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
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <cmath>
#include <cstring>

void SSTVRXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Process I/Q samples
    for (size_t i = 0; i < buffer.count; i++) {
        auto s = buffer.p[i];
        
        // FM demodulation using phase difference
        int32_t i_val = s.real();
        int32_t q_val = s.imag();
        
        // Calculate phase difference (simplified FM demod)
        int32_t delta_phi = (i_val * prev_q - q_val * prev_i);
        prev_i = i_val;
        prev_q = q_val;
        
        // Scale to approximate audio sample (-128 to 127 range)
        int32_t demod_sample = delta_phi >> 16;
        
        // Estimate frequency from demodulated signal
        estimate_frequency(demod_sample);
        
        // Process based on current state
        switch (state) {
            case STATE_SYNC_SEARCH:
                detect_sync(current_freq);
                break;
                
            case STATE_VIS_DECODE:
                // TODO: Implement VIS code detection
                // For now, skip directly to image data
                state = STATE_IMAGE_DATA;
                break;
                
            case STATE_IMAGE_DATA:
                process_pixel_sample(current_freq);
                break;
        }
    }
}

// Estimate frequency using zero-crossing detection
void SSTVRXProcessor::estimate_frequency(int32_t demod_sample) {
    // Detect zero crossings
    if ((prev_sample < 0 && demod_sample >= 0) || (prev_sample >= 0 && demod_sample < 0)) {
        // Zero crossing detected
        if (zero_cross_timer > 0) {
            // Calculate frequency: f = sampling_rate / (2 * period_in_samples)
            // Sampling rate = 3,072,000 Hz
            // Period = 2 * zero_cross_timer (time between crossings)
            current_freq = 1536000 / (zero_cross_timer + 1);  // Avoid divide by zero
            zero_cross_count++;
        }
        zero_cross_timer = 0;
    } else {
        zero_cross_timer++;
        // Prevent overflow and stuck frequency
        if (zero_cross_timer > 6144) {  // ~2ms timeout
            zero_cross_timer = 6144;
            current_freq = 0;
        }
    }
    
    prev_sample = demod_sample;
}

// Convert frequency to pixel value (0-255)
int32_t SSTVRXProcessor::freq_to_pixel(int32_t freq) {
    // SSTV standard: 1500 Hz = black (0), 2300 Hz = white (255)
    if (freq < FREQ_BLACK) freq = FREQ_BLACK;
    if (freq > FREQ_WHITE) freq = FREQ_WHITE;
    
    // Linear mapping
    int32_t pixel = ((freq - FREQ_BLACK) * 255) / (FREQ_WHITE - FREQ_BLACK);
    
    if (pixel < 0) pixel = 0;
    if (pixel > 255) pixel = 255;
    
    return pixel;
}

// Detect horizontal sync pulses
void SSTVRXProcessor::detect_sync(int32_t freq) {
    // Sync pulse is 1200 Hz for ~9ms
    const int32_t sync_tolerance = 50;  // Hz
    
    if (freq > (FREQ_SYNC - sync_tolerance) && freq < (FREQ_SYNC + sync_tolerance)) {
        sync_sample_count++;
        
        // If we've seen sync frequency long enough, consider it a sync pulse
        if (sync_sample_count > samples_per_sync / 2) {  // At least half of sync duration
            in_sync = true;
        }
    } else {
        // Not sync frequency
        if (in_sync && sync_sample_count > 0) {
            // End of sync pulse detected - start of image data
            state = STATE_IMAGE_DATA;
            sample_count = 0;
            pixel_index = 0;
            channel_index = 0;  // Scottie starts with Green
            in_sync = false;
        }
        sync_sample_count = 0;
    }
}

// Process pixel samples during image data state
void SSTVRXProcessor::process_pixel_sample(int32_t freq) {
    sample_count++;
    
    // Average samples over pixel duration
    static int32_t pixel_accumulator = 0;
    static uint32_t pixel_sample_count = 0;
    
    pixel_accumulator += freq;
    pixel_sample_count++;
    
    // When we've accumulated enough samples for one pixel
    if (pixel_sample_count >= samples_per_pixel) {
        // Calculate average frequency
        int32_t avg_freq = pixel_accumulator / pixel_sample_count;
        
        // Convert to pixel value
        uint8_t pixel_value = freq_to_pixel(avg_freq);
        
        // Store in appropriate channel buffer
        if (pixel_index < PIXELS_PER_LINE) {
            switch (channel_index) {
                case 0:  // Green (Scottie sequence: GBR)
                    line_buffer_g[pixel_index] = pixel_value;
                    break;
                case 1:  // Blue
                    line_buffer_b[pixel_index] = pixel_value;
                    break;
                case 2:  // Red
                    line_buffer_r[pixel_index] = pixel_value;
                    break;
            }
        }
        
        pixel_index++;
        pixel_accumulator = 0;
        pixel_sample_count = 0;
        
        // Check if we finished a color channel
        if (pixel_index >= PIXELS_PER_LINE) {
            pixel_index = 0;
            channel_index++;
            
            // Skip gap between channels (1.5ms for Scottie)
            sample_count += samples_per_gap;
            
            // Check if we finished all three channels (complete line)
            if (channel_index >= 3) {
                process_line();
                channel_index = 0;
                
                // Look for next sync pulse
                state = STATE_SYNC_SEARCH;
                sync_sample_count = 0;
                sample_count = 0;
            }
        }
    }
}

void SSTVRXProcessor::process_line() {
    if (current_line >= 256) return;  // Scottie 2 has 256 lines
    
    // Copy line data to shared memory for M0 to read
    // Use bb_data to transfer RGB data
    uint8_t* data_ptr = shared_memory.bb_data.data;
    
    // Pack RGB data: [line_number(2 bytes)][R(320)][G(320)][B(320)]
    data_ptr[0] = current_line & 0xFF;
    data_ptr[1] = (current_line >> 8) & 0xFF;
    
    memcpy(data_ptr + 2, line_buffer_r, PIXELS_PER_LINE);
    memcpy(data_ptr + 2 + PIXELS_PER_LINE, line_buffer_g, PIXELS_PER_LINE);
    memcpy(data_ptr + 2 + PIXELS_PER_LINE * 2, line_buffer_b, PIXELS_PER_LINE);
    
    // Send progress message
    SSTVRXProgressMessage progress_message{current_line, 256};
    shared_memory.application_queue.push(progress_message);
    
    current_line++;
}

void SSTVRXProcessor::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::SSTVRXConfigure: {
            const auto message = *reinterpret_cast<const SSTVRXConfigureMessage*>(msg);
            vis_code = message.code;
            configured = true;
            current_line = 0;
            sample_count = 0;
            pixel_index = 0;
            channel_index = 0;
            state = STATE_SYNC_SEARCH;
            
            // Set timing for Scottie 2 (can be extended for other modes)
            // Scottie 2: 0.2752ms per pixel at 3.072MHz = 845.5 samples
            samples_per_pixel = 846;
            samples_per_sync = 27648;   // 9ms
            samples_per_gap = 4608;     // 1.5ms
            
            break;
        }
        
        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SSTVRXProcessor>()};
    event_dispatcher.run();
    return 0;
}