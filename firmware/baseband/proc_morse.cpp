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

void MorseProcessor::configure(uint8_t mode) {
    configured = false;

    modulation = static_cast<ModulationMode>((uint8_t)mode);

    if (modulation == ModulationMode::FM) {  // FM
        decim_0.configure(taps_11k0_decim_0.taps);
        decim_1.configure(taps_11k0_decim_1.taps);
        channel_filter.configure(taps_11k0_channel.taps, 2);
        demod_cw_fm.configure(24000, 5000);
    } else {
        decim_0.configure(taps_4k25_decim_0.taps);
        decim_1.configure(taps_4k25_decim_1.taps);
        if (modulation == ModulationMode::AM) {  // AM
            channel_filter.configure(taps_2k0_am_lpf_channel.taps, 4);
        } else {                                    // SSB, DSB
            if (modulation == ModulationMode::DSB)  // DSB
                channel_filter.configure(taps_1k5_dsb_lpf.taps, 4);
            else if (modulation == ModulationMode::USB)  // USB
                channel_filter.configure(taps_1k5_USB_channel.taps, 4);
            else  // LSB
                channel_filter.configure(taps_1k5_LSB_channel.taps, 4);
        }
    }

    if (modulation == ModulationMode::FM)
        audio_output.configure(iir_config_passthrough, iir_config_passthrough, (float)user_squelch_level / 100.0f);
    else
        audio_output.configure(audio_12k_hpf_300hz_config);

    meas_samples_in_period = 0;
    meas_last_period_len = 0;
    meas_consistency_count = 0;
    meas_signal_state_high = false;
    meas_freq_accumulator = 0.0f;
    meas_freq_count = 0;
    ui_update_timer = 0;

    // Decoding reset
    s_prev_i = 0;
    s_prev2_i = 0;
    goertzel_count = 0;
    duration_samples = 0;
    was_signaling = false;

    // Thresholds
    noise_floor = 5000;
    startup_delay = 20;
    squelch_is_open = true;
    squelch_hold = 0;

    dc_average = 0.0f;

    current_freq = 700.0f;
    update_goertzel_coeff(current_freq);

    configured = true;
}

inline buffer_f32_t MorseProcessor::demodulate(const buffer_c16_t& channel) {
    // AM,SSB,DSB always keeps squelch "technically" open for the demodulator
    if (modulation == ModulationMode::AM || modulation == ModulationMode::DSB) {
        squelch_is_open = true;
        return demod_AM.execute(channel, audio_buffer);
    } else {
        if (modulation == ModulationMode::USB || modulation == ModulationMode::LSB) {
            squelch_is_open = true;
            return demod_ssb.execute(channel, audio_buffer);
        }
    }
    return demod_cw_fm.execute(channel, audio_buffer);
}

void MorseProcessor::update_goertzel_coeff(float freq) {
    // limit to algo capacity min/max
    if (freq < 300.0f) freq = 300.0f;
    if (freq > 2300.0f) freq = 2300.0f;

    float sample_rate = (modulation == ModulationMode::FM) ? 24000.0f : 12000.0f;
    float omega = 2.0f * M_PI * freq / sample_rate;
    float omega_sq = omega * omega;
    float cos_approx = 1.0f - (omega_sq * 0.5f);
    // 16384 is the scaling factor for the Goertzel integer math
    coeff_int = (int32_t)(2.0f * cos_approx * 16384.0f);
}

void MorseProcessor::measure_frequency(int32_t sample) {
    // Noise gate threshold
    const int32_t gate_threshold = (modulation == ModulationMode::FM) ? 4000 : 2000;

    if (sample > gate_threshold || sample < -gate_threshold) {
        if (sample > 0 && !meas_signal_state_high) {
            // Rising Edge (End of Period)
            meas_signal_state_high = true;

            if (meas_samples_in_period > 0) {
                bool period_is_stable = false;
                if (meas_last_period_len > 0) {
                    int32_t diff = std::abs((int)meas_samples_in_period - (int)meas_last_period_len);
                    if (diff <= 2) {
                        meas_consistency_count++;
                        period_is_stable = true;
                    } else {
                        meas_consistency_count = 0;
                    }
                }
                meas_last_period_len = meas_samples_in_period;

                // Wait for AT LEAST 3 STABLE CYCLES
                if (period_is_stable && meas_consistency_count > 5) {
                    float base_rate = (modulation == ModulationMode::FM) ? 24000.0f : 12000.0f;
                    float inst_freq = base_rate / (float)meas_samples_in_period;
                    if (modulation == ModulationMode::DSB) {
                        inst_freq /= 2.0f;
                    }
                    // Check for overflows
                    if (inst_freq > 250 && inst_freq < 3000) {
                        meas_freq_accumulator += inst_freq;
                        meas_freq_count++;
                    }
                }
            }
            meas_samples_in_period = 0;

        } else if (sample < 0) {
            meas_signal_state_high = false;
        }
    } else {
        // Silence detection
        if (meas_samples_in_period > 200) {
            meas_last_period_len = 0;
            meas_consistency_count = 0;
        }
    }

    meas_samples_in_period++;

    ui_update_timer++;
    uint32_t update_limit = (modulation == ModulationMode::FM) ? 4800 : 2400;  // ~200ms

    if (ui_update_timer > update_limit) {
        if (meas_freq_count > 0) {
            float avg_freq = meas_freq_accumulator / (float)meas_freq_count;
            current_freq = avg_freq;
            // Round to 5Hz for display
            uint32_t stable_disp = (uint32_t)avg_freq;
            stable_disp = (stable_disp / 5) * 5;

            freq_message.measured_frequency = stable_disp;
            shared_memory.application_queue.push(freq_message);
        }

        meas_freq_accumulator = 0.0f;
        meas_freq_count = 0;
        ui_update_timer = 0;
    }
}

void MorseProcessor::process_decoding(int32_t sample) {
    update_goertzel_coeff(current_freq);

    // 1. Goertzel Algorithm
    int64_t s = (int64_t)sample + (((int64_t)coeff_int * s_prev_i) >> 14) - s_prev2_i;
    s_prev2_i = s_prev_i;
    s_prev_i = (int32_t)s;
    goertzel_count++;

    // 2. Evaluation every 60 samples
    if (goertzel_count >= 60) {
        if (startup_delay > 0) {
            startup_delay--;
            // Fast learning phase
            int64_t pwr = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                          (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);
            noise_floor = (noise_floor * 15 + pwr) / 16;
        } else {
            // Power calculation
            int64_t power = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                            (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);

            // Adaptive Noise Floor
            if (!was_signaling) {
                noise_floor = (noise_floor * 127 + power) / 128;
            }

            // Threshold Calculation
            int64_t sensitivity = 4 + (user_squelch_level / 10);
            int64_t base_pwr_threshold = noise_floor * sensitivity;
            int64_t current_pwr_threshold = was_signaling ? (base_pwr_threshold / 2) : base_pwr_threshold;

            // Tone Detection
            bool is_tone = squelch_is_open && (power > current_pwr_threshold) && (power > 150000);
            int32_t time_base = 250;
            // State Change Logic
            if (is_tone != was_signaling) {
                int32_t duration_us = (int32_t)((int64_t)duration_samples * time_base / 3);

                // Send message if significant
                if (duration_us > 10000 || was_signaling) {
                    message.state_durations[0] = was_signaling ? duration_us : -duration_us;
                    message.state_cnt = 1;  // 1 indicates valid state duration data
                    shared_memory.application_queue.push(message);
                }
                was_signaling = is_tone;
                duration_samples = 0;
            }

            // Timeout Logic
            if (!was_signaling && duration_samples > 28800) {
                int32_t duration_us = (int32_t)((int64_t)duration_samples * time_base / 3);
                message.state_durations[0] = -duration_us;
                message.state_cnt = 1;
                shared_memory.application_queue.push(message);
                duration_samples = 0;
            }
        }

        duration_samples += 60;
        s_prev_i = 0;
        s_prev2_i = 0;
        goertzel_count = 0;
    }
}

void MorseProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);

    auto audio_buf = demodulate(channel_out);

    for (size_t i = 0; i < audio_buf.count; i++) {
        float raw_audio = audio_buf.p[i];

        // Squelch Logic
        int32_t raw_int_abs = (int32_t)(raw_audio * 32768.0f);
        if (raw_int_abs < 0) raw_int_abs = -raw_int_abs;

        int32_t audio_threshold = (user_squelch_level * user_squelch_level) * 3;

        if (raw_int_abs > audio_threshold || user_squelch_level == 0) {
            squelch_is_open = true;
            squelch_hold = (modulation == ModulationMode::FM) ? 2400 : 1200;
        } else {
            if (squelch_hold > 0)
                squelch_hold--;
            else if (modulation == ModulationMode::FM)
                squelch_is_open = false;
        }

        float decode_audio = raw_audio;
        if (modulation != ModulationMode::FM) {
            float gain = 16.0f;
            if (modulation == ModulationMode::USB || modulation == ModulationMode::LSB)
                gain = 5.0f;
            decode_audio *= gain;

            // Hard Limiting / Clipping
            message.clipped = false;
            if (decode_audio > 1.0f) {
                decode_audio = 1.0f;
                message.clipped = true;
            } else if (decode_audio < -1.0f) {
                decode_audio = -1.0f;
            }
            audio_buf.p[i] = decode_audio;
        }

        // DC BLOCKING
        if (modulation != ModulationMode::FM) {
            dc_average = (dc_average * 0.95f) + (decode_audio * 0.05f);
            measure_frequency((int32_t)((decode_audio - dc_average) * 32768.0f));
        } else {
            measure_frequency((int32_t)(raw_audio * 32768.0f));
        }

        process_decoding((int32_t)(decode_audio * 32768.0f));

        if (modulation == ModulationMode::FM && !squelch_is_open) {
            audio_buf.p[i] = 0.0f;  // mute nfm on squelch
        }
    }

    audio_output.write(audio_buf);
}

void MorseProcessor::on_message(const Message* const p) {
    switch (p->id) {
        case Message::ID::MorseRXConfig: {
            auto morse_rx_msg = *reinterpret_cast<const MorseRXConfigureMessage*>(p);
            configure(morse_rx_msg.mode);
            break;
        }

        case Message::ID::NBFMConfigure: {
            auto nbfm_msg = *reinterpret_cast<const NBFMConfigureMessage*>(p);
            user_squelch_level = nbfm_msg.squelch_level;
            audio_output.configure(iir_config_passthrough, iir_config_passthrough, (float)user_squelch_level / 100.0f);
            break;
        }

        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<MorseProcessor>()};
    event_dispatcher.run();
    return 0;
}