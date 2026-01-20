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

void MorseProcessor::configure(uint8_t mode) {
    configured = false;

    if (mode == 0) {  // CW/FM
        decim_0.configure(taps_11k0_decim_0.taps);
        decim_1.configure(taps_11k0_decim_1.taps);
        channel_filter.configure(taps_11k0_channel.taps, 2);
        demod_cw_fm.configure(24000, 5000);
    } else {  // USB, LSB
        decim_0.configure(taps_6k0_decim_0.taps);
        decim_1.configure(taps_6k0_decim_1.taps);

        if (mode == 1)  // USB
            channel_filter.configure(taps_2k8_usb_channel.taps, 4);
        else  // LSB
            channel_filter.configure(taps_2k8_lsb_channel.taps, 4);
    }

    modulation = mode;
    if (mode > 0)
        audio_output.configure(audio_12k_hpf_300hz_config);
    else
        audio_output.configure(iir_config_passthrough, iir_config_passthrough, (float)user_squelch_level / 100.0f);

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
    // frequency measurement
    ui_update_timer = 0;
    freq_hold_timer = 0;
    display_freq = 0.0f;
    current_freq = 0.0f;

    samples_in_period = 0;
    signal_state_high = false;
    freq_avg_accumulator = 0.0f;
    freq_avg_count = 0;

    squelch_is_open = true;
    configured = true;
}

void MorseProcessor::update_goertzel_coeff(float freq) {
    if (freq < 100.0f) freq = 100.0f;
    if (freq > 4000.0f) freq = 4000.0f;  // limit to algo capacity min/max

    // 0 = FM (24k), 1 = USB (12k), 2 = LSB (12k)
    float sample_rate = (modulation == 0) ? 24000.0f : 12000.0f;

    float omega = 2.0f * M_PI * freq / sample_rate;
    float omega_sq = omega * omega;
    float cos_approx = 1.0f - (omega_sq * 0.5f);

    coeff_int = (int32_t)(2.0f * cos_approx * 16384.0f);
}

inline buffer_f32_t MorseProcessor::demodulate(const buffer_c16_t& channel) {
    if (modulation > 0) {
        squelch_is_open = true;
        return demod_ssb.execute(channel, audio_buffer);
    }
    return demod_cw_fm.execute(channel, audio_buffer);
}

void MorseProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // --- 1. Filter & Demod & Stats ---
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    // RSSI és Waterfall működéséhez
    feed_channel_stats(decim_1_out);

    const auto channel = channel_filter.execute(decim_1_out, dst_buffer);
    buffer_f32_t audio_buf = demodulate(channel);

    // --- 2. Gain ---
    if (modulation > 0) {
        const float gain = 6.0f;
        for (size_t i = 0; i < audio_buf.count; i++) {
            float sample = audio_buf.p[i] * gain;
            if (sample > 1.0f)
                sample = 1.0f;
            else if (sample < -1.0f)
                sample = -1.0f;
            audio_buf.p[i] = sample;
        }
    }

    float sample_rate = (modulation == 0) ? 24000.0f : 12000.0f;
    int32_t trigger_level = 150;

    for (size_t i = 0; i < audio_buf.count; i++) {
        int32_t raw_sample = audio_buf.p[i] * 32768;

        // --- 3. DC & LPF ---
        dc_offset += (raw_sample - dc_offset) / 256;
        int32_t sample = raw_sample - dc_offset;
        lpf_sample = (lpf_sample + sample) / 2;

        int32_t abs_sample = (sample < 0) ? -sample : sample;
        int32_t audio_threshold = (user_squelch_level * user_squelch_level) * 3;

        int32_t sq_thresh = squelch_is_open ? (audio_threshold / 2) : audio_threshold;
        if (abs_sample > sq_thresh || user_squelch_level == 0) {
            squelch_is_open = true;
            squelch_hold = (modulation == 0) ? 2400 : 1200;
        } else {
            if (squelch_hold > 0)
                squelch_hold--;
            else if (modulation == 0)
                squelch_is_open = false;
        }

        // Szigorúbb "erős jel" definíció a méréshez (kevesebb ugrálás zajban)
        bool strong_signal = (abs_sample > (audio_threshold * 0.8f));

        // --- 4. PRECÍZIÓS FREKVENCIA MÉRÉS ---
        if (squelch_is_open) {
            samples_in_period++;

            // Schmitt Trigger
            if (!signal_state_high) {
                if (lpf_sample > trigger_level) {
                    signal_state_high = true;

                    // Zajszűrés: > 13 minta (~1850Hz alatt)
                    if (samples_in_period > 13 && samples_in_period < 80) {
                        float raw_freq = sample_rate / (float)samples_in_period;

                        freq_avg_accumulator += raw_freq;
                        freq_avg_count++;

                        // Átlagolás (4 periódus)
                        if (freq_avg_count >= 4) {
                            float avg_freq = freq_avg_accumulator / 4.0f;

                            // A) Morse dekódernek: Gyors, pontos érték kell
                            current_freq = (current_freq * 0.5f) + (avg_freq * 0.5f);

                            // B) Kijelzőnek: "Ragadós" (Sticky) logika kell
                            if (strong_signal && avg_freq > 300.0f && avg_freq < 1850.0f) {
                                float diff = avg_freq - display_freq;
                                if (diff < 0) diff = -diff;  // Abszolút érték

                                // LOGIKA:
                                // Ha a változás kicsi (< 20Hz), akkor NEM nyúlunk a kijelzőhöz!
                                // Ezzel megszűnik az ugrálás (remegés).
                                // Csak akkor váltunk, ha tényleg új hang van (> 20Hz eltérés).
                                if (diff > 20.0f) {
                                    display_freq = avg_freq;  // Azonnali ugrás új hangra
                                } else {
                                    // Nagyon finom csúszás a pontosság kedvéért, de alig látható
                                    display_freq = (display_freq * 0.98f) + (avg_freq * 0.02f);
                                }

                                // Tartás időzítő
                                freq_hold_timer = (modulation == 0) ? 36000 : 18000;
                            }

                            freq_avg_accumulator = 0.0f;
                            freq_avg_count = 0;
                        }
                    }
                    samples_in_period = 0;
                }
            } else {
                if (lpf_sample < -trigger_level) {
                    signal_state_high = false;
                }
            }
            if (samples_in_period > 100) samples_in_period = 100;
        } else {
            samples_in_period = 0;
            signal_state_high = false;
        }
        prev_sample = (int16_t)lpf_sample;

        // HOLD Timer
        if (freq_hold_timer > 0)
            freq_hold_timer--;
        else if (display_freq > 0.0f)
            display_freq *= 0.95f;

        // --- 5. Goertzel ---
        bool freq_in_range = (current_freq >= 400.0f && current_freq <= 1400.0f);
        if (freq_in_range) {
            update_goertzel_coeff(current_freq);

            int64_t s = (int64_t)sample + (((int64_t)coeff_int * s_prev_i) >> 14) - s_prev2_i;
            s_prev2_i = s_prev_i;
            s_prev_i = (int32_t)s;
            goertzel_count++;

            if (goertzel_count >= 60) {
                if (startup_delay > 0) startup_delay--;

                int64_t pwr = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                              (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);

                if (!was_signaling) noise_floor = (noise_floor * 127 + pwr) / 128;
                int64_t threshold = noise_floor * (4 + (user_squelch_level / 10));
                if (was_signaling) threshold /= 2;

                bool is_tone = squelch_is_open && (pwr > threshold) && (pwr > 150000);

                if (is_tone != was_signaling) {
                    int32_t time_base = (modulation == 0) ? 125 : 250;
                    int32_t duration_us = (int32_t)((int64_t)duration_samples * time_base / 3);

                    message.state_durations[0] = was_signaling ? duration_us : -duration_us;

                    // KEREKÍTÉS (SNAP): 5Hz-es rácsra kerekítünk a kijelzéshez
                    uint32_t stable_disp = (uint32_t)(display_freq);
                    stable_disp = (stable_disp / 5) * 5;

                    message.measured_frequency = stable_disp;
                    message.state_cnt = 1;
                    shared_memory.application_queue.push(message);

                    was_signaling = is_tone;
                    duration_samples = 0;
                    ui_update_timer = 0;
                }

                if (!was_signaling && duration_samples > 28800) duration_samples = 28800;
                duration_samples += 60;
                s_prev_i = 0;
                s_prev2_i = 0;
                goertzel_count = 0;
            }
        } else {
            s_prev_i = 0;
            s_prev2_i = 0;
            goertzel_count = 0;
            if (was_signaling) {
                int32_t time_base = (modulation == 0) ? 125 : 250;
                int32_t duration_us = (int32_t)((int64_t)duration_samples * time_base / 3);

                uint32_t stable_disp = (uint32_t)(display_freq);
                stable_disp = (stable_disp / 5) * 5;

                message.state_durations[0] = duration_us;
                message.measured_frequency = stable_disp;
                message.state_cnt = 1;
                shared_memory.application_queue.push(message);
                was_signaling = false;
                duration_samples = 0;
            }
        }

        // --- 6. UI Update (Stabilizált) ---
        if (squelch_is_open) {
            ui_update_timer++;
            int32_t limit = (modulation == 0) ? 2400 : 1200;

            if (ui_update_timer > limit) {
                if (freq_hold_timer > 0) {
                    // Itt is alkalmazzuk a kerekítést
                    uint32_t stable_disp = (uint32_t)(display_freq);
                    stable_disp = (stable_disp / 5) * 5;  // Kerekítés 5Hz-re

                    message.measured_frequency = stable_disp;
                    message.state_cnt = 0;
                    message.state_durations[0] = 0;
                    shared_memory.application_queue.push(message);
                }
                ui_update_timer = 0;
            }
        } else {
            ui_update_timer = 0;
        }

        if (!squelch_is_open) audio_buf.p[i] = 0;
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