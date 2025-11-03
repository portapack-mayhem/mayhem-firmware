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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void SSTVRXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        // Just return silently if not configured
        return;
    }
    
    // Decimation chain
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto channel = decim_1.execute(decim_0_out, dst_buffer);
    
    // FM demodulation and audio processing
    // NFM-style demodulation outputs 24kHz audio directly
    auto audio = demod.execute(channel, work_audio_buffer);
    
    // Feed audio samples to output and use for frequency estimation
    audio_output.write(audio);

    // Process each audio sample for SSTV decoding
    // audio is buffer_s16_t, so audio.p[i] is int16_t
    for (size_t i = 0; i < audio.count; i++) {
        // Get int16 audio sample directly (no float conversion needed)
        int32_t audio_sample = audio.p[i];
        
        // Estimate frequency using Goertzel algorithm on the audio tones
        estimate_frequency_goertzel(audio_sample);
        
        // Process based on current state
        switch (state) {
            case STATE_SYNC_SEARCH:
                // Use raw frequency for sync detection (skip offset correction for now)
                detect_sync(current_freq);
                break;
                
            case STATE_VIS_DECODE:
                // VIS code detection not implemented yet
                // Skip directly to image data after sync
                state = STATE_IMAGE_DATA;
                break;
                
            case STATE_IMAGE_DATA:
                // Use raw frequency without offset correction for now
                // TODO: Re-enable offset correction once basic decoding works
                process_pixel_sample(current_freq);
                break;
        }
    }
}

// Estimate frequency from audio samples using Goertzel algorithm
void SSTVRXProcessor::estimate_frequency_goertzel(int32_t audio_sample) {
    // Normalize sample to float [-1.0, 1.0]
    float sample = audio_sample / 32768.0f;
    
    // Update Goertzel filters for each target frequency
    for (int f = 0; f < 4; f++) {
        float Q0 = goertzel_coeff[f] * goertzel_Q1[f] - goertzel_Q2[f] + sample;
        goertzel_Q2[f] = goertzel_Q1[f];
        goertzel_Q1[f] = Q0;
    }
    
    goertzel_count++;
    
    // Calculate magnitudes every N samples
    if (goertzel_count >= GOERTZEL_N) {
        float magnitudes[4];
        
        for (int f = 0; f < 4; f++) {
            // Calculate magnitude^2 (we don't need sqrt for comparison)
            magnitudes[f] = goertzel_Q1[f] * goertzel_Q1[f] + 
                           goertzel_Q2[f] * goertzel_Q2[f] - 
                           goertzel_Q1[f] * goertzel_Q2[f] * goertzel_coeff[f];
            
            // Reset for next block
            goertzel_Q1[f] = 0;
            goertzel_Q2[f] = 0;
        }
        
        // Find which frequency has the strongest response
        int max_idx = 0;
        float max_mag = magnitudes[0];
        for (int f = 1; f < 4; f++) {
            if (magnitudes[f] > max_mag) {
                max_mag = magnitudes[f];
                max_idx = f;
            }
        }
        
        // Map index to frequency
        // 0=1200Hz, 1=1500Hz, 2=1900Hz, 3=2300Hz
        const int freqs[4] = {1200, 1500, 1900, 2300};
        
        // Interpolate between adjacent bins for better accuracy
        if (max_mag > 0.005f) {  // Lower threshold for better sensitivity
            int freq_est = freqs[max_idx];
            
            // Improved linear interpolation between bins
            if (max_idx > 0 && magnitudes[max_idx - 1] > 0.002f) {
                float ratio = magnitudes[max_idx - 1] / max_mag;
                if (ratio > 0.2f) {  // More aggressive interpolation
                    freq_est -= (int)((freqs[max_idx] - freqs[max_idx - 1]) * ratio * 0.5f);
                }
            }
            if (max_idx < 3 && magnitudes[max_idx + 1] > 0.002f) {
                float ratio = magnitudes[max_idx + 1] / max_mag;
                if (ratio > 0.2f) {  // More aggressive interpolation
                    freq_est += (int)((freqs[max_idx + 1] - freqs[max_idx]) * ratio * 0.5f);
                }
            }
            
            // Smooth frequency changes to reduce noise
            current_freq = (current_freq * 3 + freq_est) / 4;
        }
        
        goertzel_count = 0;
    }
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
    const int32_t sync_tolerance = 200;  // Hz - wider tolerance for reliability
    const uint32_t min_sync_samples = samples_per_sync / 4;  // Need at least 1/4 of sync duration
    
    // Check for sync frequency (1200 Hz ± 200 Hz = 1000-1400 Hz)
    if (freq > (FREQ_SYNC - sync_tolerance) && freq < (FREQ_SYNC + sync_tolerance)) {
        sync_sample_count++;
        
        // If we've seen sync frequency long enough, consider it a sync pulse
        if (sync_sample_count > min_sync_samples) {
            in_sync = true;
        }
    } else {
        // Not sync frequency - check if we just finished a valid sync
        if (in_sync && sync_sample_count >= min_sync_samples) {
            // Valid sync pulse detected - start decoding image data
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
            
            // Configure decimation chain using NFM filters (narrower than WFMAM)
            decim_0.configure(taps_11k0_decim_0.taps);  // NFM decim0 filter
            decim_1.configure(taps_11k0_decim_1.taps);  // NFM decim1 filter
            
            // Calculate filter parameters
            const size_t decim_0_input_fs = baseband_fs;
            const size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;
            const size_t decim_1_input_fs = decim_0_output_fs;
            const size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;
            
            // Configure demodulator for SSTV - use moderate NFM deviation
            // SSTV needs wider deviation than voice NFM to capture 1200-2300 Hz tone range
            demod.configure(decim_1_output_fs, 7500);  // 7.5kHz deviation (wider for SSTV tones)
            // No audio filter needed - we want clean SSTV tones without filtering
            // Enable audio output for monitoring with passthrough filters
            audio_output.configure(iir_config_passthrough, iir_config_passthrough, 0.0f);
            
            // Initialize Goertzel coefficients for 24kHz sample rate
            // coeff = 2 * cos(2 * PI * freq / sample_rate)
            const float sample_rate = 24000.0f;  // Changed from 12kHz to 24kHz
            const float target_freqs[4] = {1200.0f, 1500.0f, 1900.0f, 2300.0f};
            for (int f = 0; f < 4; f++) {
                float k = (GOERTZEL_N * target_freqs[f]) / sample_rate;
                float omega = (2.0f * M_PI * k) / GOERTZEL_N;
                goertzel_coeff[f] = 2.0f * cosf(omega);
                goertzel_Q1[f] = 0;
                goertzel_Q2[f] = 0;
            }
            goertzel_count = 0;
            
            // Initialize state variables
            current_freq = 1200;  // Default to sync frequency
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
            // Decimation: 3.072MHz /8 /8 = 48kHz -> demod -> 24kHz
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