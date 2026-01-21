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
        squelch_is_open = false;
    } else {  // USB, LSB
        decim_0.configure(taps_6k0_decim_0.taps);
        decim_1.configure(taps_6k0_decim_1.taps);
        squelch_is_open = true;

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

    current_freq = 0.0f;
    display_freq = 0.0f;
    was_signaling = false;

    // ÚJ VÁLTOZÓK RESETELÉSE
    freq_acc_count = 0;
    freq_avg_accumulator = 0.0f;
    samples_in_period = 0;
    signal_state_high = false;
    freq_hold_timer = 0;
    ui_update_timer = 0;

    configured = true;
}

void MorseProcessor::update_goertzel_coeff(float freq) {
    if (freq < 300.0f) freq = 300.0f;
    if (freq > 2500.0f) freq = 2500.0f;

    float sample_rate = (modulation == 0) ? 24000.0f : 12000.0f;
    float x = 6.283185f * freq / sample_rate;

    // Taylor-soros közelítés cos(x)-re (ez nem foglal extra helyet a flash-ben)
    float x2 = x * x;
    float cos_approx = 1.0f - (x2 * 0.5f) + (x2 * x2 * 0.04166f);

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

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel = channel_filter.execute(decim_1_out, dst_buffer);

    buffer_f32_t audio_buf = demodulate(channel);

    for (size_t i = 0; i < audio_buf.count; i++) {
        int32_t raw_sample = audio_buf.p[i] * 32768;
        dc_offset += (raw_sample - dc_offset) / 32;
        int32_t sample = raw_sample - dc_offset;
        lpf_sample = (lpf_sample * 3 + sample) / 4;
        int32_t abs_sample = (sample < 0) ? -sample : sample;

        // Squelch logika
        int32_t audio_threshold = (user_squelch_level * user_squelch_level) * 3;
        if (abs_sample > audio_threshold || user_squelch_level == 0) {
            squelch_is_open = true;
            squelch_hold = 2400;
        } else {
            if (squelch_hold > 0)
                squelch_hold--;
            else if (modulation == 0)
                squelch_is_open = false;
        }

        // --- STABILIZÁLT FREKVENCIA MÉRÉS ---
        if (squelch_is_open) {
            if ((lpf_sample >= 0 && prev_sample < 0) || (lpf_sample < 0 && prev_sample >= 0)) {
                // Csak akkor frissítünk frekvenciát, ha NINCS épp jelvétel (szünetben hangolunk)
                // Vagy ha a jel nagyon instabil. Ez megakadályozza az ugrálást vétel közben.
                if (!was_signaling && zc_counter >= 4 && zc_counter <= 64) {
                    float base_sr = (modulation == 0) ? 24000.0f : 12000.0f;
                    float n_freq = base_sr / (zc_counter * 2.0f);

                    if (n_freq > 350.0f && n_freq < 2200.0f) {
                        // Erősebb simítás (0.95), hogy ne ugráljon
                        current_freq = (current_freq * 0.95f) + (n_freq * 0.05f);
                        update_goertzel_coeff(current_freq);
                        freq_hold_timer = (int32_t)base_sr;
                    }
                }
                zc_counter = 0;
            } else {
                if (zc_counter < 100) zc_counter++;
            }
        }
        prev_sample = (int16_t)lpf_sample;

        // Goertzel
        int64_t s = (int64_t)sample + (((int64_t)coeff_int * s_prev_i) >> 14) - s_prev2_i;
        s_prev2_i = s_prev_i;
        s_prev_i = (int32_t)s;
        goertzel_count++;

        if (goertzel_count >= 60) {
            int64_t power = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                            (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);

            if (startup_delay > 0) {
                startup_delay--;
                noise_floor = (noise_floor * 15 + power) / 16;
            } else {
                if (!was_signaling) noise_floor = (noise_floor * 127 + power) / 128;

                int64_t sensitivity = 4 + (user_squelch_level / 10);
                int64_t pwr_threshold = was_signaling ? (noise_floor * sensitivity / 2) : (noise_floor * sensitivity);

                int64_t min_pwr = (modulation == 0) ? 150000 : 80000;
                bool is_tone = squelch_is_open && (power > pwr_threshold) && (power > min_pwr);

                if (is_tone != was_signaling) {
                    int32_t time_base = (modulation == 0) ? 41 : 83;
                    int32_t duration_us = (int32_t)((int64_t)duration_samples * time_base);

                    if (duration_us > 15000) {  // 15ms alatti tüskéket elvetünk (zajszűrés)
                        message.state_durations[0] = was_signaling ? duration_us : -duration_us;
                        message.measured_frequency = (uint32_t)current_freq;
                        message.state_cnt = 1;
                        shared_memory.application_queue.push(message);
                    }
                    was_signaling = is_tone;
                    duration_samples = 0;
                }
            }
            duration_samples += 60;
            s_prev_i = s_prev2_i = goertzel_count = 0;
        }

        // UI frissítés
        if (freq_hold_timer > 0) {
            if (++ui_update_timer > 4800) {  // Ritkább UI frissítés (200ms), kevesebb ugrálás
                message.measured_frequency = (uint32_t)current_freq;
                message.state_cnt = 0;
                shared_memory.application_queue.push(message);
                ui_update_timer = 0;
            }
            freq_hold_timer--;
        }
        if (!squelch_is_open && modulation == 0) audio_buf.p[i] = 0;
    }

    // SSB Hangerő korrekció
    if (modulation > 0) {
        for (size_t i = 0; i < audio_buf.count; i++) {
            audio_buf.p[i] *= 6.0f;  // 8x helyett 6x, hogy ne torzítson annyira
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