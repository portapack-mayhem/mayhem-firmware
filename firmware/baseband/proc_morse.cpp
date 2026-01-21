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

    // Szűrők beállítása (egyesítve a pontosabb konfigurációval)
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

    meas_samples_in_period = 0;
    meas_last_period_len = 0;
    meas_consistency_count = 0;
    meas_signal_state_high = false;
    meas_freq_accumulator = 0.0f;
    meas_freq_count = 0;
    ui_update_timer = 0;

    set_decoder_freq(700.0f);

    configured = true;
}

inline buffer_f32_t MorseProcessor::demodulate(const buffer_c16_t& channel) {
    if (modulation > 0) {
        squelch_is_open = true;
        return demod_ssb.execute(channel, audio_buffer);
    }
    return demod_cw_fm.execute(channel, audio_buffer);
}

void MorseProcessor::set_decoder_freq(float freq) {
    float sample_rate = (modulation == 0) ? 24000.0f : 12000.0f;
    float omega = 2.0f * M_PI * freq / sample_rate;
    float omega_sq = omega * omega;
    float cos_approx = 1.0f - (omega_sq * 0.5f);
    dec_coeff_int = (int32_t)(2.0f * cos_approx * 16384.0f);
}

void MorseProcessor::measure_frequency(int32_t sample) {
    // Zajküszöb amplitúdóra
    const int32_t gate_threshold = (modulation == 0) ? 4000 : 2000;

    if (sample > gate_threshold || sample < -gate_threshold) {
        if (sample > 0 && !meas_signal_state_high) {
            // --- Felfutó él (Egy periódus vége) ---
            meas_signal_state_high = true;

            if (meas_samples_in_period > 0) {
                // --- SZIGORÚ STABILITÁS VIZSGÁLAT ---
                // Megnézzük, hogy az aktuális periódus hossza EGYEZIK-E az előzővel.
                // A zajnál ez véletlenszerű, CW jelnél konstans.

                bool period_is_stable = false;
                if (meas_last_period_len > 0) {
                    // Kiszámoljuk a különbséget
                    int32_t diff = std::abs((int)meas_samples_in_period - (int)meas_last_period_len);

                    // Nagyon szigorú tűrés: Max 1 minta eltérés engedélyezett!
                    if (diff <= 1) {
                        meas_consistency_count++;
                        period_is_stable = true;
                    } else {
                        // Ha ugrál, nullázzuk a bizalmi számlálót
                        meas_consistency_count = 0;
                    }
                }
                meas_last_period_len = meas_samples_in_period;

                // Csak akkor hiszünk a mérésnek, ha már LEGALÁBB 3 STABIL CIKLUS volt egymás után.
                // Ez megöli az FM zaj véletlenszerű egyezéseit.
                if (period_is_stable && meas_consistency_count > 3) {
                    float base_rate = (modulation == 0) ? 24000.0f : 12000.0f;
                    float inst_freq = base_rate / (float)meas_samples_in_period;

                    // Frekvencia ablak szűrés (300Hz - 3000Hz)
                    if (inst_freq > 300.0f && inst_freq < 3000.0f) {
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
        // Csend detektálás (reseteljük a stabilitást, ne ragadjon be)
        if (meas_samples_in_period > 200) {
            meas_last_period_len = 0;
            meas_consistency_count = 0;
        }
    }

    meas_samples_in_period++;

    // --- UI FRISSÍTÉS ---
    ui_update_timer++;
    uint32_t update_limit = (modulation == 0) ? 4800 : 2400;  // ~200ms

    if (ui_update_timer > update_limit) {
        if (meas_freq_count > 0) {
            float avg_freq = meas_freq_accumulator / (float)meas_freq_count;

            // Nincs SSB kompenzáció, nyers mérés megy ki
            uint32_t stable_disp = (uint32_t)avg_freq;

            // Kerekítés 5Hz-re
            stable_disp = (stable_disp / 5) * 5;

            message.measured_frequency = stable_disp;
            message.state_cnt = 0;
            shared_memory.application_queue.push(message);
        }

        meas_freq_accumulator = 0.0f;
        meas_freq_count = 0;
        ui_update_timer = 0;
    }
}

void MorseProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    auto audio_buf = demodulate(channel_out);

    for (size_t i = 0; i < audio_buf.count; i++) {
        // Skálázás (Float -> Int32)
        int32_t sample_int = (int32_t)(audio_buf.p[i] * 32768.0f);

        // A proc_all logikáját tartalmazó függvény hívása
        measure_frequency(sample_int);

        // Hang kimenet
        // if (modulation == 0 && squelch_closed) audio_buf.p[i] = 0;
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