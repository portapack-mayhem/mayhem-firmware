/*
 * Copyright (C) 2024 PortaPack Mayhem
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

#include "proc_tonedetect.hpp"
#include "audio_dma.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include <cmath>

static constexpr float SAMPLE_RATE = 24000.0f;
static constexpr uint32_t WINDOW_SAMPLES = 960;  // 40 ms at 24 kHz (30 execute() calls)
static constexpr uint32_t WINDOW_MS = (1000 * WINDOW_SAMPLES) / (uint32_t)SAMPLE_RATE;
static constexpr uint32_t AUDIO_BLOCK_SAMPLES = 32;  // demod_fm emits 32 audio samples per execute()
static constexpr uint32_t SQUELCH_HOLD_BLOCKS =
    (100 * (uint32_t)SAMPLE_RATE) / (1000 * AUDIO_BLOCK_SAMPLES);  // 100 ms hold at execute() block rate
static constexpr float PI_F = 3.14159265f;

// Motorola/EIA QCII paging frequencies (×10 to avoid float in table)
static constexpr uint32_t MOTO_FREQS_X10[45] = {
    2885,
    3047,
    3217,
    3396,
    3586,
    3786,
    3998,
    4221,
    4457,
    4705,
    4968,
    5246,
    5539,
    5848,
    6174,
    6519,
    6883,
    7268,
    7674,
    8102,
    8555,
    9032,
    9537,
    10073,
    10642,
    11225,
    11247,
    11534,
    11852,
    11885,
    12178,
    12514,
    12555,
    12858,
    13258,
    13576,
    13950,
    13996,
    14768,
    15579,
    16430,
    17325,
    18262,
    19245,
    20275,
};

// Minimum coherent Goertzel energy for MOTO tone detection.
// For amplitude A over WINDOW_SAMPLES: energy ≈ (A × N/2)² = (A × 240)².
// Threshold 1000 → requires A > ~13% of FM demod full scale (~650 Hz deviation).
// Set high enough that FM broadband noise (σ ≈ 0.3, expected per-bin energy ≈ 43)
// never triggers a false detection even when the carrier gate is briefly open.
static constexpr float MOTO_ENERGY_THRESHOLD = 1000.0f;

// Fixed carrier-detect threshold for the detection gate (independent of user squelch).
// FMSquelch returns true when HF noise is BELOW this value (= FM carrier present).
// 0.20 opens reliably on any clean FM carrier without false-opening on noise.
static constexpr float CARRIER_DETECT_THRESHOLD = 0.20f;

// Goertzel energy threshold for CTCSS detection.
// Pure CTCSS at 5% amplitude over 960 samples → energy ≈ (0.05 × 480)² = 576.
static constexpr float CTCSS_ENERGY_THRESHOLD = 30.0f;

void ToneDetectProcessor::configure(uint8_t squelch, uint32_t ctcss_f_x10) {
    configured = false;
    user_squelch_level = squelch;
    ctcss_freq_x10 = ctcss_f_x10;

    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);
    demod_fm.configure(24000, 5000);
    audio_output.configure(false);
    fm_squelch.set_threshold((float)user_squelch_level / 100.0f);
    channel_spectrum.set_decimation_factor(1);

    // Precompute Goertzel coefficient for CTCSS (if active).
    // k = nearest DFT bin for the CTCSS frequency over WINDOW_SAMPLES.
    if (ctcss_freq_x10 > 0) {
        const float freq = (float)ctcss_freq_x10 / 10.0f;
        const float k = roundf((float)WINDOW_SAMPLES * freq / SAMPLE_RATE);
        const float omega = 2.0f * PI_F * k / (float)WINDOW_SAMPLES;
        goertzel_coeff = 2.0f * cosf(omega);
    } else {
        goertzel_coeff = 0.0f;
    }

    // Precompute Goertzel coefficients for all 45 MOTO frequencies.
    // Each filter is tuned to the EXACT MOTO frequency (not the nearest DFT bin center)
    // so that every entry has a unique coefficient and maximum energy only at its
    // specific frequency.  This eliminates bin-sharing ambiguity and gives the best
    // discrimination between close entries such as 1357.6 Hz and 1395.0 Hz.
    for (size_t i = 0; i < 45; i++) {
        const float freq = (float)MOTO_FREQS_X10[i] / 10.0f;
        const float omega = 2.0f * PI_F * freq / SAMPLE_RATE;
        moto_coeff[i] = 2.0f * cosf(omega);
        moto_s1[i] = 0.0f;
        moto_s2[i] = 0.0f;
    }

    // Fixed carrier-detect squelch — threshold never changes with user settings.
    carrier_sq.set_threshold(CARRIER_DETECT_THRESHOLD);

    // Reset all per-window state
    goertzel_s1 = 0.0f;
    goertzel_s2 = 0.0f;
    window_sample_count = 0;
    was_ctcss_detected = false;
    tone_duration_windows = 0;
    squelch_is_open = false;
    squelch_hold = 0;
    carrier_is_open = false;
    carrier_hold = 0;

    configured = true;
}

void ToneDetectProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    // Feed IQ data into spectrum collector for the RF waterfall.
    channel_spectrum.feed(decim_1_out, -5500, 5500, 3400);

    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    auto audio_buf = demod_fm.execute(channel_out, audio_buffer);

    // --- Audio muting squelch (user-adjustable level) ---
    // FMSquelch returns true when HF noise is LOW (carrier present).
    const bool fm_open = fm_squelch.execute(audio_buf);
    if (fm_open) {
        squelch_hold = SQUELCH_HOLD_BLOCKS;
        squelch_is_open = true;
    } else if (squelch_hold > 0) {
        squelch_hold--;
    } else {
        squelch_is_open = false;
    }

    // --- Fixed carrier-detect gate (independent of user squelch level) ---
    // carrier_sq always uses CARRIER_DETECT_THRESHOLD (0.20) regardless of squelch_val.
    // This ensures the detection gate closes when the carrier disappears even when the
    // user sets squelch=0 (always-open audio), preventing the state machine from
    // accumulating noise windows or getting stuck between transmissions.
    const bool carrier_raw = carrier_sq.execute(audio_buf);
    if (carrier_raw) {
        carrier_hold = SQUELCH_HOLD_BLOCKS;
        carrier_is_open = true;
    } else if (carrier_hold > 0) {
        carrier_hold--;
    } else {
        carrier_is_open = false;
    }

    for (size_t i = 0; i < audio_buf.count; i++) {
        const float s = audio_buf.p[i];

        // Mute audio output when FM squelch is closed (does not affect Goertzel).
        if (!squelch_is_open) audio_buf.p[i] = 0.0f;

        // CTCSS Goertzel step (CTCSS mode only).
        // Uses original unmuted sample so CTCSS energy is unaffected by audio muting.
        if (ctcss_freq_x10 > 0) {
            const float s0 = s + goertzel_coeff * goertzel_s1 - goertzel_s2;
            goertzel_s2 = goertzel_s1;
            goertzel_s1 = s0;
        }

        // MOTO frequency bank — Goertzel step for all 45 MOTO bins.
        // Runs unconditionally so the full window always contributes to energy.
        // Benefit over zero-crossing: coherent detection, no warm-up period,
        // accurate on the very first window after the gate opens.
        for (size_t j = 0; j < 45; j++) {
            const float s0 = s + moto_coeff[j] * moto_s1[j] - moto_s2[j];
            moto_s2[j] = moto_s1[j];
            moto_s1[j] = s0;
        }

        // Window boundary: every WINDOW_SAMPLES samples = one 40 ms estimate window
        if (++window_sample_count >= WINDOW_SAMPLES) {
            window_sample_count = 0;

            // --- Detection gate ---
            // Carrier presence (carrier_is_open) is always required — without it,
            // the FM demod outputs broadband noise that floods every Goertzel bin and
            // triggers false detections regardless of the CTCSS threshold.
            // CTCSS mode adds a second requirement: coherent CTCSS energy must also
            // be present.  This is the standard two-condition squelch used in real radios.
            bool gate_open;
            if (ctcss_freq_x10 > 0) {
                const float power = goertzel_s1 * goertzel_s1 + goertzel_s2 * goertzel_s2 - goertzel_coeff * goertzel_s1 * goertzel_s2;
                gate_open = carrier_is_open && (power > CTCSS_ENERGY_THRESHOLD);
                goertzel_s1 = 0.0f;
                goertzel_s2 = 0.0f;
            } else {
                gate_open = carrier_is_open;
            }

            // --- MOTO frequency identification via Goertzel energy ---
            // Find the MOTO table entry with the highest coherent energy this window.
            // Reset all states regardless of gate so each window starts fresh.
            float energies[45]{};
            uint32_t best_idx = 45;  // 45 = sentinel (no match)
            float best_energy = MOTO_ENERGY_THRESHOLD;
            for (size_t j = 0; j < 45; j++) {
                const float pwr = moto_s1[j] * moto_s1[j] + moto_s2[j] * moto_s2[j] - moto_coeff[j] * moto_s1[j] * moto_s2[j];
                energies[j] = pwr;
                moto_s1[j] = 0.0f;
                moto_s2[j] = 0.0f;
                if (pwr > best_energy) {
                    best_energy = pwr;
                    best_idx = j;
                }
            }
            // Report a raw estimate from the local energy centroid around the best
            // entry, and let the UI do the final table snap from the phase average.
            uint32_t win_freq_hz = 0;
            if (best_idx < 45) {
                const size_t start = (best_idx > 0) ? (best_idx - 1) : best_idx;
                const size_t end = (best_idx + 1 < 45) ? (best_idx + 1) : best_idx;
                float weight_sum = 0.0f;
                float weighted_freq_x10 = 0.0f;
                for (size_t j = start; j <= end; j++) {
                    const float weight = energies[j] - MOTO_ENERGY_THRESHOLD;
                    if (weight > 0.0f) {
                        weight_sum += weight;
                        weighted_freq_x10 += weight * (float)MOTO_FREQS_X10[j];
                    }
                }
                if (weight_sum > 0.0f) {
                    win_freq_hz = (uint32_t)((weighted_freq_x10 / weight_sum) / 10.0f + 0.5f);
                } else {
                    win_freq_hz = MOTO_FREQS_X10[best_idx] / 10;
                }
            }

            if (gate_open) {
                if (!was_ctcss_detected) {
                    tone_duration_windows = 0;
                }
                tone_duration_windows++;
                was_ctcss_detected = true;

                data_message.freq_hz = win_freq_hz;
                data_message.duration_ms = tone_duration_windows * WINDOW_MS;
                data_message.tone_end = false;
                shared_memory.application_queue.push(data_message);

            } else {
                if (was_ctcss_detected) {
                    // Gate just closed — signal tone end to application
                    data_message.freq_hz = 0;
                    data_message.duration_ms = tone_duration_windows * WINDOW_MS;
                    data_message.tone_end = true;
                    shared_memory.application_queue.push(data_message);
                    tone_duration_windows = 0;
                }
                was_ctcss_detected = false;
            }
        }
    }

    audio_output.write(audio_buf);
}

void ToneDetectProcessor::on_message(const Message* const p) {
    switch (p->id) {
        case Message::ID::ToneDetectConfig: {
            const auto& msg = *reinterpret_cast<const ToneDetectConfigureMessage*>(p);
            configure(msg.squelch_level, msg.ctcss_freq_x10);
            break;
        }
        case Message::ID::NBFMConfigure: {
            const auto& msg = *reinterpret_cast<const NBFMConfigureMessage*>(p);
            user_squelch_level = msg.squelch_level;
            fm_squelch.set_threshold((float)user_squelch_level / 100.0f);
            break;
        }
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(p);
            break;
        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<ToneDetectProcessor>()};
    event_dispatcher.run();
    return 0;
}
