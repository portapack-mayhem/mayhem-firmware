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
#include <cstdlib>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
constexpr size_t sstv_shared_buffer_bytes = sizeof(shared_memory.bb_data.data);
constexpr size_t sstv_chunk_flag_index = sstv_shared_buffer_bytes - 1;  // Reserve last byte as ownership flag
constexpr size_t sstv_chunk_header_bytes = 2;
constexpr size_t sstv_chunk_copy_bytes = sstv_shared_buffer_bytes - 1;  // Bytes copied to M0 (excludes flag)
constexpr uint16_t sstv_max_chunk_pixels = (sstv_chunk_copy_bytes - sstv_chunk_header_bytes) / 3;

inline volatile uint8_t& chunk_flag() {
    return *reinterpret_cast<volatile uint8_t*>(&shared_memory.bb_data.data[sstv_chunk_flag_index]);
}

inline void mark_chunk_ready() {
    // Make sure the chunk payload is visible to the M0 before the flag is.
    __asm__ volatile("dmb" ::: "memory");
    chunk_flag() = 1;
}

inline const sstv_mode* find_mode_by_vis_code(const uint8_t vis_code) {
    for (const auto& mode : sstv_modes) {
        if (mode.vis_code == vis_code) {
            return &mode;
        }
    }
    return nullptr;
}

inline std::array<uint8_t, 3> color_order_for_mode(const sstv_mode& mode) {
    switch (mode.color_sequence) {
        case SSTV_COLOR_RGB:
            return {0, 1, 2};
        case SSTV_COLOR_GBR:
            return {1, 2, 0};
        default:
            return {0, 1, 2};
    }
}
}  // namespace

void SSTVRXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        // Just return silently if not configured
        return;
    }

    send_pending_chunk();

    // Decimation chain (same as NFM)
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel = channel_filter.execute(decim_1_out, dst_buffer);
    feed_channel_stats(channel);

    if (modulation == SSTV_MOD_FM) {
        // FM demodulation, 48 kHz audio
        auto audio = demod.execute(channel, work_audio_buffer);
        audio_output.write(audio);
        for (size_t i = 0; i < audio.count; i++) {
            decode_sample(audio.p[i] / 32768.0f, 0.0f);
        }
        return;
    }

    // SSB: the tone is already in the channel, at +f (USB) or -f (LSB): no demodulation,
    // the frequency comes straight from the IQ. The real part is the audio to listen to.
    const float sign = (modulation == SSTV_MOD_LSB) ? -1.0f : 1.0f;
    const size_t count = std::min(channel.count, ssb_audio.size());
    for (size_t i = 0; i < count; i++) {
        ssb_audio[i] = channel.p[i].real();
        decode_sample(channel.p[i].real() / 32768.0f, sign * channel.p[i].imag() / 32768.0f);
    }
    audio_output.write(buffer_s16_t{ssb_audio.data(), count, channel.sampling_rate});
}

// One audio sample (FM) or IQ sample (SSB) through the decoder
void SSTVRXProcessor::decode_sample(const float re, const float im) {
    // Increment global sample counter for calibration
    global_sample_count++;

    // Estimate the tone frequency
    estimate_frequency(re, im);
    detect_vis();

    // Process based on current state
    switch (state) {
        case STATE_SYNC_SEARCH:
            // Before Line 0: wait for initial sync pulses to establish timing
            if (current_line == 0) {
                detect_sync(sync_freq);
            }
            // After Line 0 started: we're at end of a line, waiting for next sync
            // Just wait - the sync will be detected and we'll transition to separator
            else {
                detect_sync(sync_freq);
            }
            break;

        case STATE_VIS_DECODE:
            // VIS code detection not implemented yet
            // Skip directly to separator wait
            state = STATE_SEPARATOR;
            sample_count = 0;
            break;

        case STATE_SEPARATOR:
            // Wait for separator/porch tone to finish before resuming pixels
            sample_count++;
            if (robot36 && channel_index == 1) {
                measure_robot36_separator();
            }
            if (separator_target == 0 || sample_count >= separator_target) {
                sample_count = 0;
                state = STATE_IMAGE_DATA;
            }
            break;

        case STATE_IMAGE_DATA:
            // Process pixels continuously
            process_pixel_sample(current_freq - freq_offset);
            break;
    }
}

void SSTVRXProcessor::Biquad::low_pass(const float cutoff, const float q, const float sample_rate) {
    const float w0 = 2.0f * static_cast<float>(M_PI) * cutoff / sample_rate;
    const float alpha = sinf(w0) / (2.0f * q);
    const float c = cosf(w0);
    const float a0 = 1.0f + alpha;
    b0 = (1.0f - c) / 2.0f / a0;
    b1 = (1.0f - c) / a0;
    b2 = b0;
    a1 = -2.0f * c / a0;
    a2 = (1.0f - alpha) / a0;
    z1 = z2 = 0;
}

// Estimate the tone frequency for every audio sample
void SSTVRXProcessor::estimate_frequency(const float re_in, const float im_in) {
    // Mix down: (re + j*im) * e^(-j*phase); FM audio is real (im = 0)
    float i = re_in * mix_re + im_in * mix_im;
    float q = im_in * mix_re - re_in * mix_im;
    const float re = mix_re * mix_step_re - mix_im * mix_step_im;
    mix_im = mix_re * mix_step_im + mix_im * mix_step_re;
    mix_re = re;
    if (++mix_count >= 1024) {  // keep the oscillator on the unit circle
        const float norm = 1.0f / sqrtf(mix_re * mix_re + mix_im * mix_im);
        mix_re *= norm;
        mix_im *= norm;
        mix_count = 0;
    }

    for (size_t s = 0; s < 2; s++) {
        i = lpf_i[s].execute(i);
        q = lpf_q[s].execute(q);
    }

    // Phase step from the previous sample: arg(x[n] * conj(x[n-1]))
    const float step = atan2f(q * prev_i - i * prev_q, i * prev_i + q * prev_q);
    prev_i = i;
    prev_q = q;

    current_freq = static_cast<int32_t>(MIX_FREQ + step * hz_per_radian);
    sync_freq_avg += (current_freq - sync_freq_avg) * SYNC_SMOOTHING;
    sync_freq = static_cast<int32_t>(sync_freq_avg);
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
        // For the tuning offset: the sync tone itself, not the edges the average still has
        if (current_freq > (FREQ_SYNC - sync_tolerance) && current_freq < (FREQ_SYNC + sync_tolerance)) {
            sync_freq_sum += current_freq;
            sync_freq_samples++;
        }
    } else {
        // Not sync frequency - check if we just finished a valid sync
        // Require at least 1/3 of expected sync duration (more lenient for weak signals)
        if (in_sync && sync_sample_count >= (samples_per_sync / 3)) {
            update_freq_offset();
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
                    const uint32_t nominal_interval = compute_nominal_line_interval();
                    if (nominal_interval == 0) {
                        use_for_calibration = false;
                    } else {
                        const uint32_t tolerance = nominal_interval / 4;
                        const uint32_t min_interval = (nominal_interval > tolerance) ? (nominal_interval - tolerance) : 0;
                        const uint32_t max_interval = nominal_interval + tolerance;
                        if (interval < min_interval || interval > max_interval) {
                            use_for_calibration = false;  // Don't use this sync for calibration
                            // Debug: Send outlier rejection message (use 0xFFF8 for interval value)
                            SSTVRXProgressMessage outlier_msg{0xFFF8, (uint16_t)(interval & 0xFFFF)};
                            shared_memory.application_queue.push(outlier_msg);
                        }
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

            bool ready_for_line = false;
            if (waiting_for_first_line) {
                if (sync_history_count >= 2) {
                    waiting_for_first_line = false;
                    ready_for_line = true;
                    SSTVRXProgressMessage start_msg{0xFFF4, static_cast<uint16_t>(sync_sample_count)};
                    shared_memory.application_queue.push(start_msg);
                }
            } else if (state == STATE_SYNC_SEARCH) {
                ready_for_line = true;
            }

            if (ready_for_line) {
                begin_line_after_sync();
            }
            // else: Line 0 without enough syncs, or mid-image but not in SYNC_SEARCH - just track the sync
        }
        in_sync = false;
        sync_sample_count = 0;
        sync_freq_sum = 0;
        sync_freq_samples = 0;
    }
}

// Mistuning (SSB on HF) moves every tone: the syncs say by how much, 1200 Hz is known
void SSTVRXProcessor::update_freq_offset() {
    if (sync_freq_samples < samples_per_sync / 4) return;
    const int32_t measured = sync_freq_sum / static_cast<int32_t>(sync_freq_samples) - FREQ_SYNC;
    if (!freq_offset_calibrated) {
        freq_offset = measured;
        freq_offset_calibrated = true;
    } else {
        freq_offset += (measured - freq_offset) / 4;
    }
}

// Calculate phase and slant calibration from sync timing
void SSTVRXProcessor::calculate_calibration() {
    if (sync_history_count < 2 || pixel_time_frac == 0.0f) return;

    expected_sync_interval = compute_nominal_line_interval();
    if (expected_sync_interval == 0) {
        return;
    }

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

uint32_t SSTVRXProcessor::compute_nominal_line_interval() const {
    if (robot36) {
        // sync, porch, Y, separator and porch, chroma at half the pixel time
        const float y_samples = pixel_time_frac * static_cast<float>(pixels_per_line);
        return samples_per_sync + samples_per_gap + channel_gap_samples + static_cast<uint32_t>(std::round(y_samples * 1.5f));
    }

    const uint32_t channel_sections = (channel_count > 0) ? channel_count : 1U;
    const uint32_t gap_sections = (samples_per_gap == 0)
                                      ? 0U
                                      : ((active_mode && active_mode->gaps) ? channel_sections : 1U);
    const float samples_per_channel_f = pixel_time_frac * static_cast<float>(pixels_per_line);
    const float rounded_channel = std::round(samples_per_channel_f);
    const uint32_t samples_per_channel = static_cast<uint32_t>(std::max(1.0f, rounded_channel));
    const uint32_t total_channel_samples = samples_per_channel * channel_sections;
    const uint32_t total_gap_samples = samples_per_gap * gap_sections;
    return samples_per_sync + total_gap_samples + total_channel_samples;
}

// Process pixel samples during image data state
void SSTVRXProcessor::process_pixel_sample(int32_t freq) {
    // Accumulate frequency samples for averaging
    pixel_accumulator += freq;
    pixel_sample_count++;

    // Advance pixel phase (1.0 per sample, adjusted by slant)
    pixel_phase += slant_factor;

    // Robot 36 sends the chroma at half the pixel time
    const float channel_pixel_time = (robot36 && channel_index == 1) ? pixel_time_frac / 2.0f : pixel_time_frac;

    // Check if we've accumulated enough samples for one or more pixels
    // channel_pixel_time is the number of audio samples per pixel for the current mode and channel
    // Use a loop to handle cases where pixel_phase exceeds it by more than one pixel
    while (pixel_phase >= channel_pixel_time && pixel_index < pixels_per_line) {
        // Pixel complete - calculate average frequency
        // Prevent division by zero
        int32_t avg_freq;
        if (pixel_sample_count > 0) {
            avg_freq = pixel_accumulator / pixel_sample_count;
        } else {
            avg_freq = freq;  // Use current frequency if no samples accumulated
        }

        // Convert to pixel value
        uint8_t pixel_value = freq_to_pixel(avg_freq);

        // Apply phase offset (horizontal shift) and clamp to prevent out-of-bounds writes
        // Clamping prevents pixels from wrapping around and causing duplication
        int32_t adjusted_pixel_index = (int32_t)pixel_index + phase_offset;
        if (adjusted_pixel_index < 0) {
            adjusted_pixel_index = 0;
        } else if (adjusted_pixel_index >= pixels_per_line) {
            adjusted_pixel_index = pixels_per_line - 1;
        }

        store_pixel_value(channel_index, static_cast<uint16_t>(adjusted_pixel_index), pixel_value);

        pixel_index++;
        // Reset accumulator for next pixel
        // If this is not the last pixel in the loop, subsequent pixels will use current sample
        pixel_accumulator = freq;
        pixel_sample_count = 1;
        pixel_phase -= channel_pixel_time;  // Keep fractional part for next pixel

        // Check if we finished a color channel
        if (pixel_index >= pixels_per_line) {
            pixel_index = 0;

            const bool last_channel = ((channel_index + 1) >= channel_count);
            if (last_channel) {
                process_line();
                channel_index = 0;
                state = STATE_SYNC_SEARCH;
                sync_sample_count = 0;
                sync_freq_sum = 0;
                sync_freq_samples = 0;
                in_sync = false;
                reset_pixel_state();
                break;
            } else {
                channel_index++;
                reset_pixel_state();
                if (channel_gap_samples > 0) {
                    start_gap(channel_gap_samples);
                } else {
                    state = STATE_IMAGE_DATA;
                }
                break;
            }
        }
    }
}

void SSTVRXProcessor::process_line() {
    if (mode_total_lines == 0) return;                       // not set
    if (current_line >= mode_total_lines) current_line = 0;  // reset, maybe a new image

    // What the M0 hasn't taken of the lines before is dropped: it is too far behind
    out_count = 0;
    out_next_chunk = 0;

    if (active_mode && active_mode->format == SSTV_FORMAT_PD) {
        // First output line
        ycbcr_to_line(pd_y0, pd_ry, pd_by);
        queue_line(current_line);

        // Second output line
        if ((current_line + 1) < mode_total_lines) {
            ycbcr_to_line(pd_y1, pd_ry, pd_by);
            queue_line(static_cast<uint16_t>(current_line + 1));
        }

        current_line = static_cast<uint16_t>(current_line + 2);
    } else if (robot36) {
        ycbcr_to_line(pd_y0, pd_ry, pd_by);
        queue_line(current_line);
        current_line++;
    } else {
        queue_line(current_line);
        current_line++;
    }
}

// Everything that depends on the mode; false when it can't be received
bool SSTVRXProcessor::set_mode(const sstv_mode* const mode) {
    if (!mode || mode->pixels == 0 || mode->pixels > MAX_PIXELS_PER_LINE) return false;

    active_mode = mode;
    vis_code = mode->vis_code;
    mode_total_lines = mode->lines ? mode->lines : 1;
    pixels_per_line = mode->pixels;
    robot36 = (mode->format == SSTV_FORMAT_ROBOT36);
    if (mode->format == SSTV_FORMAT_PD) {
        channel_count = 4;  // Y, (R-Y), (B-Y), Y
    } else if (robot36) {
        channel_count = 2;  // Y, then (R-Y) or (B-Y)
    } else {
        channel_count = mode->color ? 3 : 1;
    }
    color_order = color_order_for_mode(*mode);
    // The line is read from the sync on: with the sync before the last colour
    // (Scottie: G, B, sync, R) the colours come in from there
    if (mode->sync_index > 0 && mode->sync_index < color_order.size()) {
        std::rotate(color_order.begin(), color_order.begin() + mode->sync_index, color_order.end());
    }

    // Translate SSTV timing constants (expressed for 3.072MHz TX) to the audio rate
    const float conversion = audio_rate / static_cast<float>(SSTV_SAMPLERATE);
    pixel_time_frac = std::max(1.0f, static_cast<float>(mode->samples_per_pixel) * conversion);
    samples_per_pixel = static_cast<uint32_t>(pixel_time_frac + 0.5f);
    const auto convert_interval = [conversion](uint32_t value) -> uint32_t {
        return static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(value) * conversion)));
    };
    samples_per_sync = convert_interval(mode->samples_per_sync);
    samples_per_gap = convert_interval(mode->samples_per_gap);
    channel_gap_samples = mode->gaps ? samples_per_gap : 0;
    if (robot36) {
        channel_gap_samples = convert_interval(SSTV_ROBOT36_SEPARATOR + SSTV_ROBOT36_PORCH);
        // Middle of the separator, clear of the Y before and the porch after
        robot36_separator_from = convert_interval(SSTV_MS2S(1.0));
        robot36_separator_to = convert_interval(SSTV_MS2S(4.0));
    }
    return true;
}

// Back to the start of a picture: line 0 begins at the next sync, or (wait_for_two_syncs)
// at the second of two, when there was no VIS to say a picture starts
void SSTVRXProcessor::restart_image(const bool wait_for_two_syncs) {
    current_line = 0;
    waiting_for_first_line = wait_for_two_syncs;
    state = STATE_SYNC_SEARCH;
    sample_count = 0;
    pixel_index = 0;
    channel_index = 0;
    sync_sample_count = 0;
    sync_freq_sum = 0;
    sync_freq_samples = 0;
    in_sync = false;
    separator_target = 0;
    clear_line_buffers();
    reset_pixel_state();

    freq_offset = 0;
    freq_offset_calibrated = false;
    sync_history_count = 0;
    memset(sync_positions, 0, sizeof(sync_positions));

    out_count = 0;
    out_next_chunk = 0;
}

// The VIS header: 1900 Hz leader, then a start bit, 8 bits (LSB first, 1100 Hz is a 1,
// 1300 Hz a 0, the last one parity) and a stop bit, 30 ms each at 1200 Hz. Searched in
// 5 ms bins of the tone frequency, so it is found whatever state the decoder is in.
void SSTVRXProcessor::detect_vis() {
    vis_sum += current_freq;
    if (++vis_count < vis_bin_samples) return;

    vis_bins[vis_bin_pos] = static_cast<int16_t>(vis_sum / static_cast<int32_t>(vis_count));
    vis_bin_pos = (vis_bin_pos + 1) % VIS_BINS;
    vis_sum = 0;
    vis_count = 0;

    if (vis_holdoff) {
        vis_holdoff--;
        return;
    }

    // Oldest bin first: leader, start bit, 8 bits, stop bit
    const auto bin = [this](size_t i) -> int32_t { return vis_bins[(vis_bin_pos + i) % VIS_BINS]; };
    // A bit by the bins clear of its edges
    const auto bit_freq = [&bin](size_t first) -> int32_t {
        return (bin(first + 1) + bin(first + 2) + bin(first + 3) + bin(first + 4)) / 4;
    };

    size_t leader = 0;
    for (size_t i = 0; i < VIS_LEADER_BINS; i++)
        if (std::abs(bin(i) - 1900) < 150) leader++;
    if (leader < VIS_LEADER_BINS * 9 / 10) return;

    const int32_t start = bit_freq(VIS_LEADER_BINS);
    const int32_t stop = bit_freq(VIS_BINS - VIS_BINS_PER_BIT);
    if (std::abs(start - FREQ_SYNC) >= 150 || std::abs(stop - FREQ_SYNC) >= 150 || std::abs(start - stop) >= 60) return;
    const int32_t reference = (start + stop) / 2;  // 1200 Hz as received

    uint8_t code = 0;
    for (size_t b = 0; b < 8; b++) {
        const int32_t d = bit_freq(VIS_LEADER_BINS + (b + 1) * VIS_BINS_PER_BIT) - reference;
        if (d < -40 && d > -180) {
            code |= 1 << b;  // 1100 Hz
        } else if (!(d > 40 && d < 180)) {
            return;  // neither 1100 nor 1300 Hz
        }
    }

    const sstv_mode* const mode = find_mode_by_vis_code(code);
    if (!mode || !set_mode(mode)) return;
    vis_holdoff = 1000 / VIS_BIN_MS;  // a second

    // The stop bit runs into line 0's sync
    restart_image(false);
    freq_offset = reference - FREQ_SYNC;
    freq_offset_calibrated = true;

    SSTVRXProgressMessage vis_message{0xFFF3, code};
    shared_memory.application_queue.push(vis_message);
}

// A finished line, kept until the M0 has taken all its chunks
void SSTVRXProcessor::queue_line(const uint16_t line) {
    if (out_count >= MAX_OUT_LINES) return;
    uint8_t* const rgb = out_rgb[out_count];
    for (uint16_t x = 0; x < pixels_per_line; x++) {
        rgb[x * 3 + 0] = line_buffer_r[x];
        rgb[x * 3 + 1] = line_buffer_g[x];
        rgb[x * 3 + 2] = line_buffer_b[x];
    }
    out_line[out_count] = line;
    out_count++;
}

// One chunk to the M0 when it has taken the one before. Never waits: the samples
// have to keep being processed, and messages are handled between two execute()s.
void SSTVRXProcessor::send_pending_chunk() {
    if (out_count == 0 || chunk_flag() != 0) return;

    const uint16_t chunks_per_line = std::max<uint16_t>(1, (pixels_per_line + sstv_max_chunk_pixels - 1) / sstv_max_chunk_pixels);
    const uint16_t index = out_next_chunk / chunks_per_line;
    const uint16_t chunk = out_next_chunk % chunks_per_line;
    const uint16_t start = chunk * sstv_max_chunk_pixels;
    const uint16_t count = std::min<uint16_t>(sstv_max_chunk_pixels, pixels_per_line - start);
    const uint16_t encoded = out_line[index] * chunks_per_line + chunk;

    uint8_t* const data = shared_memory.bb_data.data;
    data[0] = encoded & 0xFF;
    data[1] = (encoded >> 8) & 0xFF;
    memcpy(data + sstv_chunk_header_bytes, out_rgb[index] + start * 3, count * 3);
    mark_chunk_ready();

    SSTVRXProgressMessage progress_message{encoded, mode_total_lines};
    if (!shared_memory.application_queue.push(progress_message)) {
        // Queue full: the M0 won't hear of it, so free the slot and send it again next time
        chunk_flag() = 0;
        return;
    }

    out_next_chunk++;
    if (out_next_chunk >= out_count * chunks_per_line) {
        out_count = 0;
        out_next_chunk = 0;
    }
}

// PD and Robot send BT.601 YCbCr (Y 16..235, chroma around 128)
void SSTVRXProcessor::ycbcr_to_line(const uint8_t* y, const uint8_t* cr, const uint8_t* cb) {
    for (uint16_t x = 0; x < pixels_per_line; x++) {
        const int32_t luma = 298 * (static_cast<int32_t>(y[x]) - 16);
        const int32_t red_diff = static_cast<int32_t>(cr[x]) - 128;
        const int32_t blue_diff = static_cast<int32_t>(cb[x]) - 128;
        line_buffer_r[x] = std::clamp<int32_t>((luma + 409 * red_diff + 128) >> 8, 0, 255);
        line_buffer_g[x] = std::clamp<int32_t>((luma - 208 * red_diff - 100 * blue_diff + 128) >> 8, 0, 255);
        line_buffer_b[x] = std::clamp<int32_t>((luma + 516 * blue_diff + 128) >> 8, 0, 255);
    }
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
            modulation = (message.modulation <= SSTV_MOD_LSB) ? static_cast<sstv_modulation>(message.modulation) : SSTV_MOD_FM;

            // Configure decimation chain using NFM filters (narrower than WFMAM)
            decim_0.configure(taps_11k0_decim_0.taps);            // NFM decim0 filter
            decim_1.configure(taps_11k0_decim_1.taps);            // NFM decim1 filter
            channel_filter.configure(taps_11k0_channel.taps, 1);  // Keep 48kHz audio for better pixel resolution

            // Calculate filter parameters
            const size_t decim_0_input_fs = baseband_fs;
            const size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;
            const size_t decim_1_input_fs = decim_0_output_fs;
            const size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;
            const size_t channel_filter_output_fs = decim_1_output_fs;  // Final rate: 48kHz
            audio_rate = static_cast<float>(channel_filter_output_fs);

            if (!set_mode(find_mode_by_vis_code(message.code))) {
                configured = false;
                SSTVRXProgressMessage error_msg{0xFFFF, 0};
                shared_memory.application_queue.push(error_msg);
                break;
            }

            // Configure demodulator for SSTV - use moderate NFM deviation
            // SSTV needs wider deviation than voice NFM to capture 1200-2300 Hz tone range
            demod.configure(channel_filter_output_fs, 7500);  // 7.5kHz deviation (wider for SSTV tones)
            // No audio filter needed - we want clean SSTV tones without filtering
            // Enable audio output for monitoring with passthrough filters
            audio_output.configure(iir_config_passthrough, iir_config_passthrough, 0.0f);

            // Tone frequency estimator for the audio rate
            const float mix_w = 2.0f * static_cast<float>(M_PI) * MIX_FREQ / audio_rate;
            mix_step_re = cosf(mix_w);
            mix_step_im = sinf(mix_w);
            mix_re = 1.0f;
            mix_im = 0.0f;
            mix_count = 0;
            const float butterworth_q[2] = {0.5412f, 1.3066f};
            for (size_t s = 0; s < 2; s++) {
                lpf_i[s].low_pass(TONE_LOW_PASS, butterworth_q[s], audio_rate);
                lpf_q[s].low_pass(TONE_LOW_PASS, butterworth_q[s], audio_rate);
            }
            prev_i = prev_q = 0.0f;
            hz_per_radian = audio_rate / (2.0f * static_cast<float>(M_PI));
            sync_freq_avg = 1200.0f;
            // From 1200 to the 1500 Hz porch, the average leaves the sync range (+150 Hz) half way
            sync_detect_lag = static_cast<uint32_t>(std::round(logf(2.0f) / SYNC_SMOOTHING));
            current_freq = 1200;  // Default to sync frequency
            sync_freq = 1200;

            vis_bin_samples = static_cast<uint32_t>(audio_rate) * VIS_BIN_MS / 1000;
            vis_sum = 0;
            vis_count = 0;
            vis_holdoff = 0;
            std::fill(std::begin(vis_bins), std::end(vis_bins), int16_t{0});

            // Without a VIS seen, two syncs in a row start the picture
            restart_image(true);
            shared_memory.bb_data.data[sstv_chunk_flag_index] = 0;
            configured = true;
            break;
        }

        default:
            break;
    }
}

void SSTVRXProcessor::reset_pixel_state() {
    pixel_accumulator = 0;
    pixel_sample_count = 0;
    pixel_phase = 0.0f;
}

void SSTVRXProcessor::start_gap(const uint32_t duration) {
    reset_pixel_state();
    separator_target = duration;
    sample_count = 0;
    if (duration == 0) {
        state = STATE_IMAGE_DATA;
    } else {
        state = STATE_SEPARATOR;
    }
}

void SSTVRXProcessor::clear_line_buffers(const bool keep_chroma) {
    std::fill_n(line_buffer_r, pixels_per_line, uint8_t{0});
    std::fill_n(line_buffer_g, pixels_per_line, uint8_t{0});
    std::fill_n(line_buffer_b, pixels_per_line, uint8_t{0});
    std::fill_n(pd_y0, pixels_per_line, uint8_t{0});
    if (!keep_chroma) {
        std::fill_n(pd_ry, pixels_per_line, uint8_t{128});
        std::fill_n(pd_by, pixels_per_line, uint8_t{128});
    }
    std::fill_n(pd_y1, pixels_per_line, uint8_t{0});
}

void SSTVRXProcessor::measure_robot36_separator() {
    if (sample_count == 1) {
        robot36_separator_sum = 0;
        robot36_separator_count = 0;
    }
    if (sample_count >= robot36_separator_from && sample_count < robot36_separator_to) {
        robot36_separator_sum += current_freq - freq_offset;
        robot36_separator_count++;
    }
    if (sample_count == robot36_separator_to && robot36_separator_count) {
        const int32_t middle = (SSTV_ROBOT36_FREQ_RY + SSTV_ROBOT36_FREQ_BY) / 2;
        robot36_chroma_by = (robot36_separator_sum / static_cast<int32_t>(robot36_separator_count)) > middle;
    }
}

void SSTVRXProcessor::begin_line_after_sync() {
    pixel_index = 0;
    channel_index = 0;
    // Robot 36 keeps the chroma of the line before, it only sends one of the two per line
    clear_line_buffers(robot36);
    // The end of the sync was seen sync_detect_lag samples late, make up for it
    if (samples_per_gap > sync_detect_lag) {
        start_gap(samples_per_gap - sync_detect_lag);
    } else {
        start_gap(0);
        pixel_phase = static_cast<float>(sync_detect_lag - samples_per_gap);
    }
}

void SSTVRXProcessor::store_pixel_value(const uint32_t channel, const uint16_t pixel, const uint8_t value) {
    if (!active_mode) {
        return;
    }

    if (pixel >= pixels_per_line) {
        return;
    }

    if (active_mode->format == SSTV_FORMAT_PD) {
        switch (channel) {
            case 0:
                pd_y0[pixel] = value;
                break;
            case 1:
                pd_ry[pixel] = value;
                break;
            case 2:
                pd_by[pixel] = value;
                break;
            case 3:
                pd_y1[pixel] = value;
                break;
            default:
                break;
        }
        return;
    }

    if (robot36) {
        if (channel == 0) {
            pd_y0[pixel] = value;
        } else {
            (robot36_chroma_by ? pd_by : pd_ry)[pixel] = value;
        }
        return;
    }

    if (!active_mode->color) {
        line_buffer_r[pixel] = value;
        line_buffer_g[pixel] = value;
        line_buffer_b[pixel] = value;
        return;
    }

    if (channel >= channel_count || channel >= color_order.size()) {
        return;
    }

    switch (color_order[channel]) {
        case 0:
            line_buffer_r[pixel] = value;
            break;
        case 1:
            line_buffer_g[pixel] = value;
            break;
        case 2:
            line_buffer_b[pixel] = value;
            break;
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
