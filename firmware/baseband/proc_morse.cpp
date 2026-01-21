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
/*
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
*/

void MorseProcessor::update_goertzel_coeff(float freq) {
    if (freq < 100.0f) freq = 100.0f;
    if (freq > 4000.0f) freq = 4000.0f;

    float sample_rate = (modulation == 0) ? 24000.0f : 12000.0f;

    // Taylor-soros közelítés cos(x)-re: 1 - x^2/2 + x^4/24
    // Ez sokkal kisebb, mint a teljes math könyvtár cosf() függvénye.
    float x = 2.0f * M_PI * freq / sample_rate;
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

    const auto channel = channel_filter.execute(
        decim_1.execute(decim_0.execute(buffer, dst_buffer), dst_buffer),
        dst_buffer);
    buffer_f32_t audio_buf = demodulate(channel);

    // Kiszámoljuk a mód-függő értékeket egyszer, fixpontosan ahol lehet
    const bool is_fm = (modulation == 0);
    const float gain = is_fm ? 8.0f : 18.0f;
    const float sample_rate = is_fm ? 24000.0f : 12000.0f;
    const int32_t trigger_level = is_fm ? 1200 : 700;
    const int32_t sig_threshold = is_fm ? 1000 : 400;

    for (size_t i = 0; i < audio_buf.count; i++) {
        float sample_f = audio_buf.p[i] * gain;
        if (sample_f > 1.0f)
            sample_f = 1.0f;
        else if (sample_f < -1.0f)
            sample_f = -1.0f;

        audio_buf.p[i] = sample_f;
        int32_t raw_sample = (int32_t)(sample_f * 32767.0f);

        dc_offset += (raw_sample - dc_offset) / 256;
        int32_t sample = raw_sample - dc_offset;
        lpf_sample = (lpf_sample * 2 + sample) / 3;
        int32_t abs_sample = (sample < 0) ? -sample : sample;

        // Squelch (zajzár)
        if (is_fm) {
            int32_t sq_limit = user_squelch_level * 120;
            if (abs_sample > sq_limit || user_squelch_level == 0) {
                squelch_is_open = true;
                squelch_hold = 2400;
            } else if (squelch_hold > 0)
                squelch_hold--;
            else
                squelch_is_open = false;
        } else
            squelch_is_open = true;

        // Frekvenciamérés (ZC)
        if (abs_sample > trigger_level) {
            samples_in_period++;
            if (!signal_state_high && lpf_sample > (trigger_level / 2)) {
                signal_state_high = true;
                freq_acc_count++;
                if (freq_acc_count >= 8) {
                    float raw_freq = (sample_rate * 8.0f) / (float)samples_in_period;
                    // SSB korrekciók a méréshez
                    if (modulation == 1)
                        raw_freq -= 40.0f;
                    else if (modulation == 2)
                        raw_freq += 30.0f;

                    if (raw_freq > 200.0f && raw_freq < 2500.0f) {
                        display_freq = (display_freq * 0.9f) + (raw_freq * 0.1f);
                        freq_hold_timer = (int32_t)sample_rate;
                    }
                    samples_in_period = 0;
                    freq_acc_count = 0;
                }
            } else if (signal_state_high && lpf_sample < -(trigger_level / 2))
                signal_state_high = false;
        }

        // Morse Dekódolás Goertzel-lel
        bool raw_signal = (abs_sample > sig_threshold);  // Használjuk a küszöböt

        if (squelch_is_open && raw_signal) {
            if (!was_signaling) {
                update_goertzel_coeff(display_freq > 100.0f ? display_freq : (is_fm ? 700.0f : 800.0f));
            }

            int64_t s = (int64_t)sample + (((int64_t)coeff_int * s_prev_i) >> 14) - s_prev2_i;
            s_prev2_i = s_prev_i;
            s_prev_i = (int32_t)s;
            goertzel_count++;

            if (goertzel_count >= 60) {
                int64_t pwr = (int64_t)s_prev_i * s_prev_i + (int64_t)s_prev2_i * s_prev2_i -
                              (((int64_t)s_prev_i * s_prev2_i * coeff_int) >> 14);

                if (!was_signaling) noise_floor = (noise_floor * 255 + pwr) / 256;

                // SSB-ben megengedőbb SNR (2.0), FM-ben szigorúbb (5.0)
                int64_t snr_limit = was_signaling ? 2 : (is_fm ? 5 : 2);
                int64_t min_pwr = is_fm ? 80000 : 30000;
                bool is_tone = (pwr > (noise_floor * snr_limit)) && (pwr > min_pwr);

                if (is_tone != was_signaling) {
                    // Üzenet küldése (időalap korrekcióval)
                    int32_t duration_us = (int32_t)((int64_t)duration_samples * (is_fm ? 41 : 83));
                    message.state_durations[0] = was_signaling ? duration_us : -duration_us;
                    message.measured_frequency = (uint32_t)((display_freq / 5) * 5);
                    message.state_cnt = 1;
                    shared_memory.application_queue.push(message);
                    was_signaling = is_tone;
                    duration_samples = 0;
                }
                duration_samples += 60;
                s_prev_i = s_prev2_i = goertzel_count = 0;
            }
        } else if (was_signaling) {
            // Jel elvesztésekor lezárás
            int32_t duration_us = (int32_t)((int64_t)duration_samples * (is_fm ? 41 : 83));
            message.state_durations[0] = duration_us;
            message.measured_frequency = (uint32_t)((display_freq / 5) * 5);
            message.state_cnt = 1;
            shared_memory.application_queue.push(message);
            was_signaling = false;
            duration_samples = 0;
        }

        // UI Frissítés (leegyszerűsítve a méret miatt)
        if (freq_hold_timer > 0) {
            if (++ui_update_timer > 2400) {
                message.measured_frequency = (uint32_t)((display_freq / 5) * 5);
                message.state_cnt = 0;
                shared_memory.application_queue.push(message);
                ui_update_timer = 0;
            }
            freq_hold_timer--;
        }
        if (!squelch_is_open && is_fm) audio_buf.p[i] = 0;
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