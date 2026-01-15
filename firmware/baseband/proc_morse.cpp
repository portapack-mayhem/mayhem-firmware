/*
 * Copyright (C) 2026 Pezsma
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

#include "proc_morse.hpp"
#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include <cmath>

void MorseProcessor::configure() {
    configured = false;
    baseband_fs = 3072000;
    baseband_thread.set_sampling_rate(baseband_fs);

    // 1. DSP filters
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);
    demod.configure(24000, 5000);
    audio_output.configure(iir_config_passthrough, iir_config_passthrough, 0.0f);

    // 2. Resetting variables
    dc_offset = 0;
    lpf_sample = 0;
    prev_sample = 0;

    // 3. Algorithm reset
    zc_counter = 0;
    last_zc_counter = 0;
    current_freq = 700.0f;
    update_goertzel_coeff(700.0f);

    goertzel_count = 0;
    s_prev_i = 0;
    s_prev2_i = 0;
    duration_samples = 0;
    was_signaling = false;

    noise_floor = 5000;  // learning speed

    startup_delay = 20;

    squelch_is_open = false;
    configured = true;
}

void MorseProcessor::on_message(const Message* const p) {
    if (p->id == Message::ID::MorseRXConfig) {
        configure();
    } else if (p->id == Message::ID::NBFMConfigure) {
        auto nbfm_msg = *reinterpret_cast<const NBFMConfigureMessage*>(p);
        user_squelch_level = nbfm_msg.squelch_level;
        audio_output.configure(iir_config_passthrough, iir_config_passthrough, (float)user_squelch_level / 100.0f);
    }
}

void MorseProcessor::update_goertzel_coeff(float freq) {
    if (freq < 400.0f) freq = 400.0f;
    if (freq > 1500.0f) freq = 1500.0f;  // limit to algo capacity min/max
    float omega = 2.0f * M_PI * freq / 24000.0f;
    coeff_int = (int32_t)(2.0f * cosf(omega) * 16384.0f);
}

void MorseProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    buffer_c16_t dst_buffer_c16(dst_buffer.data(), dst_buffer.size());
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer_c16);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer_c16);
    const auto channel = channel_filter.execute(decim_1_out, dst_buffer_c16);

    feed_channel_stats(channel);

    buffer_s16_t audio_buffer_s16(audio_buffer.data(), audio_buffer.size());
    auto audio_buf = demod.execute(channel, audio_buffer_s16);

    for (size_t i = 0; i < audio_buf.count; i++) {
        int32_t raw_sample = audio_buf.p[i];

        // 1. DC & LPF
        dc_offset += (raw_sample - dc_offset) / 32;
        int32_t sample = raw_sample - dc_offset;
        lpf_sample = (lpf_sample * 3 + sample) / 4;

        // 2. Squelch
        int32_t abs_sample = (sample < 0) ? -sample : sample;
        int32_t audio_threshold = (user_squelch_level * user_squelch_level) * 3;
        int32_t current_audio_threshold = squelch_is_open ? (audio_threshold / 2) : audio_threshold;

        if (abs_sample > current_audio_threshold || user_squelch_level == 0) {
            squelch_is_open = true;
            squelch_hold = 2400;
        } else {
            if (squelch_hold > 0)
                squelch_hold--;
            else
                squelch_is_open = false;
        }

        // 3. Frequency measurement (ZC)
        if (squelch_is_open && !was_signaling) {
            if ((lpf_sample >= 0 && prev_sample < 0) || (lpf_sample < 0 && prev_sample >= 0)) {
                if (zc_counter >= 8 && zc_counter <= 32) {
                    int32_t diff = zc_counter - last_zc_counter;
                    if (diff >= -1 && diff <= 1) {
                        float n_freq = 24000.0f / (zc_counter * 2.0f);

                        // Hybrid tracking

                        if (zc_counter < 15) {
                            // This stabilizes the 1000-1400 Hz range
                            current_freq = (current_freq * 0.6f) + (n_freq * 0.4f);
                        } else {
                            current_freq = n_freq;
                        }

                        update_goertzel_coeff(current_freq);
                    }
                    last_zc_counter = zc_counter;
                } else {
                    last_zc_counter = 0;
                }
                zc_counter = 0;
            } else {
                if (zc_counter < 100) zc_counter++;
            }
        }
        prev_sample = (int16_t)lpf_sample;

        // 4. Goertzel
        int64_t s = (int64_t)sample + (((int64_t)coeff_int * s_prev_i) >> 14) - s_prev2_i;
        s_prev2_i = s_prev_i;
        s_prev_i = (int32_t)s;
        goertzel_count++;

        // 5. Detection
        if (goertzel_count >= 60) {
            if (startup_delay > 0) {
                startup_delay--;
                int64_t pwr = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                              (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);
                noise_floor = (noise_floor * 15 + pwr) / 16;
            } else {
                int64_t power = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                                (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);

                if (!was_signaling) {
                    noise_floor = (noise_floor * 127 + power) / 128;
                }

                int64_t sensitivity = 4 + (user_squelch_level / 10);
                int64_t base_pwr_threshold = noise_floor * sensitivity;
                int64_t current_pwr_threshold = was_signaling ? (base_pwr_threshold / 2) : base_pwr_threshold;

                bool is_tone = squelch_is_open && (power > current_pwr_threshold) && (power > 150000);

                if (is_tone != was_signaling) {
                    int32_t duration_us = (int32_t)((int64_t)duration_samples * 125 / 3);

                    if (duration_us > 10000) {
                        message.times[0] = was_signaling ? duration_us : -duration_us;
                        message.measured_frequency = (uint32_t)current_freq;
                        message.timeptr = 1;
                        shared_memory.application_queue.push(message);
                    }
                    was_signaling = is_tone;
                    duration_samples = 0;
                }

                if (!was_signaling && duration_samples > 28800) {
                    int32_t duration_us = (int32_t)((int64_t)duration_samples * 125 / 3);
                    message.times[0] = -duration_us;
                    message.timeptr = 1;
                    shared_memory.application_queue.push(message);
                    duration_samples = 0;
                }
            }

            duration_samples += 60;
            s_prev_i = 0;
            s_prev2_i = 0;
            goertzel_count = 0;
        }

        audio_buf.p[i] = squelch_is_open ? (int16_t)sample : 0;
    }
    audio_output.write(audio_buf);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<MorseProcessor>()};
    event_dispatcher.run();
    return 0;
}