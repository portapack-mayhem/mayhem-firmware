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
#include "message.hpp"

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
    
    // Decimation chain (same as NFM)
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel = channel_filter.execute(decim_1_out, dst_buffer);
    
    // FM demodulation and audio processing
    // Demodulator outputs 24kHz audio after channel filter decimation
    auto audio = demod.execute(channel, work_audio_buffer);
    
    // Feed audio samples to output and use for frequency estimation
    audio_output.write(audio);

    // Process each audio sample for SSTV decoding
    // audio is buffer_s16_t, so audio.p[i] is int16_t
    for (size_t i = 0; i < audio.count; i++) {
        // Get int16 audio sample directly (no float conversion needed)
        int32_t audio_sample = audio.p[i];
        
        // Increment global sample counter for calibration
        global_sample_count++;
        
        // Estimate frequency using Goertzel algorithm on the audio tones
        estimate_frequency_goertzel(audio_sample);
        
        // Process based on current state
        switch (state) {
            case STATE_SYNC_SEARCH:
                detect_sync(current_freq);
                
                // On Line 0, we MUST wait indefinitely for the first valid sync
                // This establishes proper timing and channel alignment
                // Do NOT increment sample_count or check timeout on Line 0
                if (current_line > 0) {
                    sample_count++;
                    
                    // Timeout if sync not detected within reasonable time on mid-image lines
                    // Expected sync duration is ~9ms (216 samples), but with slant/drift,
                    // allow up to 500ms (~12000 samples) before giving up and moving on
                    if (sample_count > 12000) {
                        // No sync detected for mid-image line - skip to separator and continue
                        // This prevents getting stuck waiting for a weak/missing sync
                        // Send debug message to indicate timeout
                        SSTVRXProgressMessage timeout_msg{0xFFFA, (uint16_t)sample_count};
                        shared_memory.application_queue.push(timeout_msg);
                        
                        state = STATE_SEPARATOR;
                        sample_count = 0;
                        pixel_index = 0;
                        channel_index = 0;
                        pixel_accumulator = 0;
                        pixel_sample_count = 0;
                        pixel_phase = 0.0f;
                    }
                }
                // Line 0: wait indefinitely for first sync - no timeout
                break;
                
            case STATE_VIS_DECODE:
                // VIS code detection not implemented yet
                // Skip directly to separator wait
                state = STATE_SEPARATOR;
                sample_count = 0;
                break;
                
            case STATE_SEPARATOR:
                // Wait for separator pulse to complete (1500Hz, 1.5ms)
                // We don't need to verify frequency, just count samples
                sample_count++;
                
                if (sample_count >= samples_per_gap) {
                    // Separator complete, move to image data
                    sample_count = 0;
                    pixel_accumulator = 0;
                    pixel_sample_count = 0;
                    pixel_phase = 0.0f;  // Reset pixel timing
                    
                    // Always move to image data after separator
                    // SSTV is continuous - no need to hunt for syncs after initial lock
                    state = STATE_IMAGE_DATA;
                }
                break;
                
            case STATE_IMAGE_DATA:
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
        
        // Check if we have a strong enough signal
        // Lowered threshold for weak signals (SSTV often has low audio levels)
        if (max_mag > 0.001f) {  // Very low threshold - accept weak signals
            int freq_est = freqs[max_idx];
            
            // Improved linear interpolation between bins
            if (max_idx > 0 && magnitudes[max_idx - 1] > 0.0005f) {
                float ratio = magnitudes[max_idx - 1] / max_mag;
                if (ratio > 0.2f) {
                    freq_est -= (int)((freqs[max_idx] - freqs[max_idx - 1]) * ratio * 0.5f);
                }
            }
            if (max_idx < 3 && magnitudes[max_idx + 1] > 0.0005f) {
                float ratio = magnitudes[max_idx + 1] / max_mag;
                if (ratio > 0.2f) {
                    freq_est += (int)((freqs[max_idx + 1] - freqs[max_idx]) * ratio * 0.5f);
                }
            }
            
            // Light smoothing to reduce noise while maintaining responsiveness
            current_freq = (current_freq + freq_est) / 2;
        } else {
            // Signal too weak - don't update frequency (keeps last valid estimate)
            // This prevents spurious detections from noise
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
    const int32_t sync_tolerance = 150;  // Hz - tolerance for sync detection
    
    // Check for sync frequency (1200 Hz ± 150 Hz)
    if (freq > (FREQ_SYNC - sync_tolerance) && freq < (FREQ_SYNC + sync_tolerance)) {
        sync_sample_count++;
        in_sync = true;
    } else {
        // Not sync frequency - check if we just finished a valid sync
        // Require at least 1/3 of expected sync duration (more lenient for weak signals)
        if (in_sync && sync_sample_count >= (samples_per_sync / 3)) {
            // Valid sync pulse detected - always record it for timing tracking
            // Debug: log current history count before recording
            SSTVRXProgressMessage pre_count_msg{0xFFF7, sync_history_count};
            shared_memory.application_queue.push(pre_count_msg);
            
            if (sync_history_count < MAX_SYNC_HISTORY) {
                sync_positions[sync_history_count] = global_sample_count;
                sync_history_count++;
                
                // Send debug message with sync count
                SSTVRXProgressMessage sync_debug{0xFFFD, sync_history_count};
                shared_memory.application_queue.push(sync_debug);
                
                // Check if this sync should be used for calibration (reject outliers)
                bool use_for_calibration = true;
                if (sync_history_count > 1) {
                    uint32_t interval = sync_positions[sync_history_count - 1] - sync_positions[sync_history_count - 2];
                    // Reject intervals >50% off expected LINE interval (not sync pulse duration!)
                    // Expected line interval is ~6663 samples (sync+sep+G+sep+B+sep+R)
                    // So accept 3331-13326 samples (50%-100% tolerance for clock drift/noise)
                    const uint32_t expected_line_interval = 6663;
                    const uint32_t min_interval = expected_line_interval / 2;       // 3331 samples
                    const uint32_t max_interval = expected_line_interval * 2;       // 13326 samples
                    if (interval < min_interval || interval > max_interval) {
                        use_for_calibration = false;  // Don't use this sync for calibration
                        // Debug: Send outlier rejection message (use 0xFFF8 for interval value)
                        SSTVRXProgressMessage outlier_msg{0xFFF8, (uint16_t)(interval & 0xFFFF)};
                        shared_memory.application_queue.push(outlier_msg);
                    }
                }
                
                // Calculate calibration after collecting enough syncs for accuracy
                // Wait for 8 syncs to get better statistics, then update every 8 syncs
                if (use_for_calibration && sync_history_count >= 8 && pixel_time_frac != 0.0f && sync_history_count % 8 == 0) {
                    calculate_calibration();
                }
            } else {
                // Debug: MAX_SYNC_HISTORY exceeded
                SSTVRXProgressMessage max_reached_msg{0xFFF6, sync_history_count};
                shared_memory.application_queue.push(max_reached_msg);
            }
            
            // Debug: Send sync detection info with timing data
            // Also send current frequency estimate for debugging
            SSTVRXProgressMessage debug_msg{0xFFFE, (uint16_t)sync_sample_count};
            shared_memory.application_queue.push(debug_msg);
            
            // Send frequency estimate for debugging (use 0xFFF9)
            SSTVRXProgressMessage freq_msg{0xFFF9, (uint16_t)current_freq};
            shared_memory.application_queue.push(freq_msg);
            
            state = STATE_SEPARATOR;
            sample_count = 0;
            pixel_index = 0;
            channel_index = 0;  // Will start with Green after separator
            pixel_accumulator = 0;
            pixel_sample_count = 0;
            pixel_phase = 0.0f;  // Reset fractional timing
            
            // Reset line buffers
            memset(line_buffer_r, 0, PIXELS_PER_LINE);
            memset(line_buffer_g, 0, PIXELS_PER_LINE);
            memset(line_buffer_b, 0, PIXELS_PER_LINE);
        }
        in_sync = false;
        sync_sample_count = 0;
    }
}

// Calculate phase and slant calibration from sync timing
void SSTVRXProcessor::calculate_calibration() {
    if (sync_history_count < 2 || pixel_time_frac == 0.0f) return;
    
    // Calculate expected interval (one full line for Scottie 2)
    // Scottie 2: 9ms sync + 1.5ms sep + 146.432ms G + 1.5ms sep + 146.432ms B + 1.5ms sep + 146.432ms R
    expected_sync_interval = samples_per_sync + samples_per_gap + 
                            (uint32_t)(pixel_time_frac * PIXELS_PER_LINE) + samples_per_gap +
                            (uint32_t)(pixel_time_frac * PIXELS_PER_LINE) + samples_per_gap +
                            (uint32_t)(pixel_time_frac * PIXELS_PER_LINE);
    
    // Send debug info about expected interval
    SSTVRXProgressMessage debug_interval{0xFFFC, (uint16_t)(expected_sync_interval & 0xFFFF)};
    shared_memory.application_queue.push(debug_interval);
    
    // Calculate average timing error (slant) from recent intervals
    // Use last 8 intervals for more responsive calibration, but filter outliers
    int32_t total_timing_error = 0;
    uint32_t last_interval = 0;
    uint16_t start_idx = (sync_history_count > 8) ? (sync_history_count - 8) : 1;
    uint16_t interval_count = 0;
    
    for (uint16_t i = start_idx; i < sync_history_count; i++) {
        uint32_t actual_interval = sync_positions[i] - sync_positions[i - 1];
        last_interval = actual_interval;
        
        // Filter out outliers: reject intervals >20% off expected value
        // These are likely missed syncs, not actual timing drift
        int32_t timing_error = (int32_t)actual_interval - (int32_t)expected_sync_interval;
        int32_t max_deviation = (int32_t)expected_sync_interval / 5;  // 20% threshold
        
        // Only include intervals within ±20% of expected
        if (timing_error >= -max_deviation && timing_error <= max_deviation) {
            total_timing_error += timing_error;
            interval_count++;
        }
    }
    
    // Send debug info about last actual interval
    SSTVRXProgressMessage debug_actual{0xFFFB, (uint16_t)(last_interval & 0xFFFF)};
    shared_memory.application_queue.push(debug_actual);
    
    if (interval_count == 0) return;  // Safety check - no valid intervals
    
    // Average error per line
    int32_t avg_error = total_timing_error / interval_count;
    
    // Convert to slant adjustment (0.1% units)
    // Error in samples / expected_sync_interval = fractional error
    // Multiply by 1000 to get 0.1% units
    int16_t suggested_slant = (int16_t)(((int64_t)avg_error * 1000) / expected_sync_interval);
    
    // Clamp to reasonable range (±10% = ±100 in 0.1% units)
    if (suggested_slant > 100) suggested_slant = 100;
    if (suggested_slant < -100) suggested_slant = -100;
    
    // Phase is harder to detect automatically without knowing absolute position
    // For now, we only suggest slant correction
    int16_t suggested_phase = 0;
    
    // Send calibration suggestion
    SSTVRXCalibrationMessage cal_msg{suggested_phase, suggested_slant, sync_history_count};
    shared_memory.application_queue.push(cal_msg);
}

// Process pixel samples during image data state
void SSTVRXProcessor::process_pixel_sample(int32_t freq) {
    // Accumulate frequency samples for averaging
    pixel_accumulator += freq;
    pixel_sample_count++;
    
    // Advance pixel phase with slant adjustment
    pixel_phase += slant_factor;
    
    // Check if we've accumulated enough for one pixel using fractional timing
    if (pixel_phase >= pixel_time_frac) {
        // Pixel complete - calculate average frequency
        int32_t avg_freq = pixel_accumulator / pixel_sample_count;
        
        // Convert to pixel value
        uint8_t pixel_value = freq_to_pixel(avg_freq);
        
        // Apply phase offset (horizontal shift)
        int32_t adjusted_pixel_index = (int32_t)pixel_index + phase_offset;
        
        // Store in appropriate channel buffer if within bounds
        if (adjusted_pixel_index >= 0 && adjusted_pixel_index < PIXELS_PER_LINE) {
            switch (channel_index) {
                case 0:  // Green (Scottie sequence: GBR)
                    line_buffer_g[adjusted_pixel_index] = pixel_value;
                    break;
                case 1:  // Blue
                    line_buffer_b[adjusted_pixel_index] = pixel_value;
                    break;
                case 2:  // Red
                    line_buffer_r[adjusted_pixel_index] = pixel_value;
                    break;
            }
        }
        
        pixel_index++;
        pixel_accumulator = 0;
        pixel_sample_count = 0;
        pixel_phase -= pixel_time_frac;  // Keep fractional part for next pixel
        
        // Check if we finished a color channel
        if (pixel_index >= PIXELS_PER_LINE) {
            pixel_index = 0;
            
            // Check if we finished all three channels (complete line)
            if (channel_index >= 2) {
                // Finished Red channel (last channel)
                process_line();
                channel_index = 0;
                
                // In Scottie 2, there's NO separator after red channel
                // The sync pulse comes immediately, followed by separator before green
                // So we transition directly to sync search for the next line
                state = STATE_SYNC_SEARCH;
                sync_sample_count = 0;
                in_sync = false;
            } else {
                // Finished Green or Blue, move to next channel
                channel_index++;
                
                // Enter separator state between channels
                state = STATE_SEPARATOR;
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
            
        case Message::ID::SSTVRXPhaseSlant: {
            const auto message = *reinterpret_cast<const SSTVRXPhaseSlantMessage*>(msg);
            phase_offset = message.phase;
            slant_rate = message.slant;
            // Convert slant from 0.1% units to a multiplier
            // slant_rate of +10 = +1% faster = multiply by 1.01
            slant_factor = 1.0f + (slant_rate / 1000.0f);
            break;
        }
            
        case Message::ID::SSTVRXConfigure: {
            const auto message = *reinterpret_cast<const SSTVRXConfigureMessage*>(msg);
            vis_code = message.code;
            
            // Configure decimation chain using NFM filters (narrower than WFMAM)
            decim_0.configure(taps_11k0_decim_0.taps);  // NFM decim0 filter
            decim_1.configure(taps_11k0_decim_1.taps);  // NFM decim1 filter
            channel_filter.configure(taps_11k0_channel.taps, 2);  // Decimate by 2 to get 24kHz
            
            // Calculate filter parameters
            const size_t decim_0_input_fs = baseband_fs;
            const size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;
            const size_t decim_1_input_fs = decim_0_output_fs;
            const size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;
            const size_t channel_filter_output_fs = decim_1_output_fs / 2;  // Final rate: 24kHz
            
            // Configure demodulator for SSTV - use moderate NFM deviation
            // SSTV needs wider deviation than voice NFM to capture 1200-2300 Hz tone range
            demod.configure(channel_filter_output_fs, 7500);  // 7.5kHz deviation (wider for SSTV tones)
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
            
            // Reset sync history for calibration
            sync_history_count = 0;
            memset(sync_positions, 0, sizeof(sync_positions));
            
            // Set timing for Scottie 2 at 24kHz audio sample rate
            // Decimation: 3.072MHz /8 /8 /2 = 24kHz (same as NFM)
            // Scottie 2: 0.2752ms/pixel, 9ms sync, 1.5ms separator
            pixel_time_frac = 0.2752f * 24.0f;  // 6.6048 samples/pixel (exact)
            samples_per_pixel = 7;               // Integer approximation for quick checks
            samples_per_sync = 216;              // 9ms × 24000 Hz = 216 samples
            samples_per_gap = 36;                // 1.5ms × 24000 Hz = 36 samples
            pixel_phase = 0.0f;
            
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