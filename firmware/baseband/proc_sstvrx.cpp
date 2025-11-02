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
#include "audio_dma.hpp"
#include "sine_table_int8.hpp"
#include "fxpt_atan2.hpp"

#include <cstdint>
#include <cmath>
#include <cstddef>
#include <cstring>

void SSTVRXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        // Just return silently if not configured
        return;
    }
    
    // Decimation chain
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto channel = decim_1.execute(decim_0_out, dst_buffer);
    
    // FM demodulation and audio processing
    auto audio_oversampled = demod.execute(channel, work_audio_buffer);
    auto audio_4fs = audio_dec_1.execute(audio_oversampled, work_audio_buffer);
    auto audio_2fs = audio_dec_2.execute(audio_4fs, work_audio_buffer);
    auto audio = audio_filter.execute(audio_2fs, work_audio_buffer);
    
    // Feed audio samples to output and use for frequency estimation
    audio_output.write(audio);

    // Process each audio sample for SSTV decoding
    for (size_t i = 0; i < audio.count; i++) {
        // Convert float audio sample to int16 for processing
        const int32_t sample_int = audio.p[i] * 32768.0f;
        int32_t audio_sample = __SSAT(sample_int, 16);
        
        // Estimate frequency using zero-crossing detection on the audio tones
        estimate_frequency_from_audio(audio_sample);
        
        // Apply frequency offset compensation (calibrated from sync pulse)
        int32_t corrected_freq = current_freq - freq_offset;
        
        // Process based on current state
        switch (state) {
            case STATE_SYNC_SEARCH:
                // Use uncorrected frequency for sync detection and calibration
                detect_sync(current_freq);
                break;
                
            case STATE_VIS_DECODE:
                // TODO: Implement VIS code detection
                // For now, skip directly to image data
                state = STATE_IMAGE_DATA;
                break;
                
            case STATE_IMAGE_DATA:
                // Use corrected frequency for pixel decoding
                process_pixel_sample(corrected_freq);
                break;
        }
    }
}

// Estimate frequency from audio samples using zero-crossing detection
void SSTVRXProcessor::estimate_frequency_from_audio(int32_t audio_sample) {
    // Simple zero-crossing detection
    // Count zero-crossings to estimate frequency of the audio tone
    
    // Detect zero crossing (sign change)
    if ((prev_audio_sample < 0 && audio_sample >= 0) || 
        (prev_audio_sample >= 0 && audio_sample < 0)) {
        
        if (zero_cross_timer > 2 && zero_cross_timer < 200) {  // Reasonable range, skip very fast crossings
            // Frequency = sample_rate / (2 * half_period)
            // At 24kHz: 1200Hz = 10 samples per half-period, 2300Hz = 5.2 samples
            uint32_t measured_freq = 12000 / zero_cross_timer;  // 24000 / (2 * timer)
            
            // Validate range
            if (measured_freq >= 800 && measured_freq <= 3000) {
                // Light smoothing to reduce noise but still track changes
                freq_smooth = (measured_freq * 3 + freq_smooth * 5) / 8;
                current_freq = freq_smooth;
            }
        }
        zero_cross_timer = 0;
    } else {
        zero_cross_timer++;
        if (zero_cross_timer > 300) {
            // No crossings for a long time - signal might be DC or very low freq
            zero_cross_timer = 300;
        }
    }
    
    prev_audio_sample = audio_sample;
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
    const int32_t sync_tolerance = 150;  // Hz - wider tolerance for reliability
    const uint32_t min_sync_samples = samples_per_sync / 4;  // Reduced requirement for better detection
    
    // Check for sync frequency (1200 Hz)
    if (freq > (FREQ_SYNC - sync_tolerance) && freq < (FREQ_SYNC + sync_tolerance)) {
        sync_sample_count++;
        
        // Accumulate frequency during sync for offset calibration
        if (!freq_offset_calibrated && sync_sample_count < 100) {
            sync_freq_accumulator += freq;
            sync_freq_count++;
        }
        
        // If we've seen sync frequency long enough, consider it a sync pulse
        if (sync_sample_count > min_sync_samples) {
            in_sync = true;
        }
    } else {
        // Not sync frequency - check if we just finished a valid sync
        if (in_sync && sync_sample_count >= min_sync_samples) {
            // Valid sync pulse detected
            
            // Calibrate frequency offset from first sync pulse
            if (!freq_offset_calibrated && sync_freq_count > 20) {
                int32_t avg_sync_freq = sync_freq_accumulator / sync_freq_count;
                freq_offset = avg_sync_freq - FREQ_SYNC;  // Should be 0 if perfectly tuned
                freq_offset_calibrated = true;
            }
            
            // Start decoding image data
            state = STATE_IMAGE_DATA;
            sample_count = 0;
            pixel_index = 0;
            channel_index = 0;  // Scottie starts with Green
            
            // Reset line buffers
            memset(line_buffer_r, 0, PIXELS_PER_LINE);
            memset(line_buffer_g, 0, PIXELS_PER_LINE);
            memset(line_buffer_b, 0, PIXELS_PER_LINE);
        }
        in_sync = false;
        sync_sample_count = 0;
        sync_freq_accumulator = 0;  // Reset for next sync
        sync_freq_count = 0;
    }
}

// Process pixel samples during image data state
void SSTVRXProcessor::process_pixel_sample(int32_t freq) {
    sample_count++;
    
    // Accumulate frequency samples over pixel duration
    // Clamp to reasonable range if invalid (helps with noise)
    if (freq < 800) {
        freq = 800;
    } else if (freq > 3000) {
        freq = 3000;
    }
    
    pixel_accumulator += freq;
    pixel_sample_count++;
    
    // When we've accumulated enough samples for one pixel
    if (pixel_sample_count >= samples_per_pixel) {
        // Calculate average frequency (protect against division by zero)
        int32_t avg_freq = (pixel_sample_count > 0) ? (pixel_accumulator / pixel_sample_count) : FREQ_BLACK;
        
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
        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(msg));
            break;
            
        case Message::ID::SSTVRXConfigure: {
            const auto message = *reinterpret_cast<const SSTVRXConfigureMessage*>(msg);
            vis_code = message.code;
            
            // Configure decimation and filtering chain
            decim_0.configure(taps_16k0_decim_0.taps);
            decim_1.configure(taps_38k_wfmam_decim_1.taps);
            
            // Calculate filter parameters
            const size_t decim_0_input_fs = baseband_fs;
            const size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;
            const size_t decim_1_input_fs = decim_0_output_fs;
            const size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;
            
            // Configure demodulator and audio chain for SSTV
            // SSTV is transmitted as narrowband FM (same as voice), typically ±5kHz deviation
            demod.configure(decim_1_output_fs, 5000);
            // Use low-pass filter to pass audio frequencies up to ~3kHz (covers SSTV range 1200-2300 Hz)
            audio_filter.configure(taps_64_lp_025_025.taps);  // Low-pass filter
            // Enable audio output for monitoring with passthrough filters (no HPF or deemph needed for SSTV)
            audio_output.configure(iir_config_passthrough, iir_config_passthrough, 0.0f);
            
            // Initialize state variables
            current_freq = 1200;  // Default to sync frequency
            prev_audio_sample = 0;
            zero_cross_timer = 0;
            freq_smooth = 1200;
            configured = true;
            current_line = 0;
            sample_count = 0;
            pixel_index = 0;
            channel_index = 0;
            pixel_accumulator = 0;
            pixel_sample_count = 0;
            sync_sample_count = 0;
            in_sync = false;
            state = STATE_SYNC_SEARCH;
            
            // Reset frequency offset calibration
            freq_offset = 0;
            freq_offset_calibrated = false;
            sync_freq_accumulator = 0;
            sync_freq_count = 0;
            
            // Set timing for Scottie 2 at 24kHz audio sample rate
            // Scottie 2: 0.2752ms/pixel, 9ms sync, 1.5ms gap
            samples_per_pixel = 7;      // 0.2752ms × 24000 Hz = 6.6 samples (round to 7)
            samples_per_sync = 216;     // 9ms × 24000 Hz = 216 samples
            samples_per_gap = 36;       // 1.5ms × 24000 Hz = 36 samples
            
            break;
        }
        
        default:
            break;
    }
}

void SSTVRXProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config) {
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } else {
        audio_output.set_stream(nullptr);
    }
}

int main() {
    // Initialize audio DMA
    audio::dma::init_audio_out();
    
    EventDispatcher event_dispatcher{std::make_unique<SSTVRXProcessor>()};
    event_dispatcher.run();
    return 0;
}