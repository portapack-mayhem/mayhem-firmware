/*
 * Copyright (C) 2020 Belousov Oleg
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

#include "dsp_modulate.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"
#include "tonesets.hpp"

#include <cmath>

namespace dsp {

namespace am {
// Full-rate beep safety only: deliberately no observer/counter access.
inline int8_t beep_output(float modulation, bool carrier) {
    const float scalar = modulation + (carrier ? 63.0f : 0.0f);
    // Finite carrier addition cannot turn a nonfinite modulation into a finite
    // scalar, so one final finite check covers both values.
    if (!std::isfinite(scalar)) return 0;
    if (scalar > 127.0f) return 127;
    if (scalar < -128.0f) return -128;
    return static_cast<int8_t>(scalar);
}

// Microphone uses the same endpoint/nonfinite protection as the full-rate beep.
inline int8_t output(float modulation, bool carrier) {
    return beep_output(modulation, carrier);
}
}  // namespace am

namespace modulate {

Modulator::~Modulator() {
}

Mode Modulator::get_mode() {
    return mode;
}

void Modulator::set_mode(Mode new_mode) {
    mode = new_mode;
}

void Modulator::set_over(uint32_t new_over) {
    over = new_over;
}

void Modulator::set_gain_shiftbits_vumeter_beep(float new_audio_gain, uint8_t new_audio_shift_bits_s16, bool new_play_beep) {
    // new_audio_shift_bits_s16 are the direct shift bits (FM mod >>x) , and it is fixed to >>8_FM (AK) or  4,5,6, (WM boost OFF) or 6,7 (WM boost ON)
    audio_gain = new_audio_gain;
    audio_shift_bits_s16_FM = new_audio_shift_bits_s16;  // FM :        >>8(AK) fixed ,  >>4,5,6 (WM boost OFF)
    if (new_audio_shift_bits_s16 == 8) {                 // FM : we are in AK codec IC => for AM-SSB-DSB we were using >>2 fixed (wm boost ON) .
        audio_shift_bits_s16_AM_DSB_SSB = 2;             // AM-DSB-SSB: >>2(AK) fixed ,  >>0,1,2 (WM boost OFF)
    } else {
        audio_shift_bits_s16_AM_DSB_SSB = (new_audio_shift_bits_s16 - 4);  // AM-DSB-SSB: >>0,1,2 (WM boost OFF), >>2,3 (WM boost ON)
    }
    play_beep = new_play_beep;
}

int32_t Modulator::apply_beep(int32_t sample_in, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message) {
    if (play_beep) {  // We need to add audio beep sample.
        if (new_beep_timer) {
            new_beep_timer--;
        } else {
            new_beep_timer = baseband_fs * 0.05;  // 50ms

            if (new_beep_index == BEEP_TONES_NB) {
                configured_in = false;
                shared_memory.application_queue.push(new_txprogress_message);
            } else {
                beep_gen.configure(beep_deltas[new_beep_index], 1.0);  // config sequentially the audio beep tone.
                new_beep_index++;
            }
        }
        sample_in = beep_gen.process(0);  // Get sample of the selected sequence of 6  beep tones , and overwrite audio sample. Mix 0%.
    }
    return sample_in;  // Return audio mic scaled with gain , 8 bit sample or audio beep sample.
}

///

SSB::SSB()
    : hilbert() {
    mode = Mode::LSB;
}

void SSB::set_fs_div_factor(float new_bw_ssb) {
    switch ((int)new_bw_ssb / 1000) {
        case 2:
            fs_div_factor = 192;  // TXBW_ssb = 2khz = BW_cut_off LPF = fs/4 ; BW_HT fs Hilbert Transform (4khz=fs/2) ==> (8k=fs) Hilbert_fs = 1.536.000/8000= 192
            break;
        case 3:
            fs_div_factor = 128;  // TXBW_ssb = 3khz = BW_cut_off LPF = fs/4 ; BW_HT fs Hilbert Transform (6khz=fs/2) ==> (12k=fs) Hilbert_fs = 1.536.000/12000= 128
            break;
        default:
            fs_div_factor = 128;  // TXBW_ssb = 3khz = BW_cut_off LPF = fs/4 ; BW_HT fs Hilbert Transform (6khz=fs/2) ==> (12k=fs) Hilbert_fs = 1.536.000/12000= 128
            break;
    }
}

// Called once per reconstructed component, before the existing C8 conversion.
inline float ssb_clamp(float raw, float scaled) {
    if (!std::isfinite(raw) || !std::isfinite(scaled)) {
        return 0.0f;  // Separate nonfinite fallback, not a finite endpoint clamp.
    }

    if (scaled > 127.0f) {
        return 127.0f;
    }
    if (scaled < -128.0f) {
        return -128.0f;
    }
    return scaled;
}

void SSB::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider) {
    // unused
    (void)configured_in;
    (void)new_beep_index;
    (void)new_beep_timer;
    (void)new_txprogress_message;

    // No way to activate correctly  the roger beep in this option, Maybe not enough M4 CPU power , Let's  block roger beep in SSB  selection by now .
    int32_t sample = 0;
    int8_t re = 0, im = 0;
    const bool interpolate_x16 = fs_div_factor == 128;
    std::array<dsp::interpolate::ComplexFIRInterpolate8::Sample, 16> reconstructed{};

    for (size_t counter = 0; counter < buffer.count; counter++) {
        if (counter % fs_div_factor == 0) {  // Ex. TX bw_ssb 3khz =fs/4 Hilbert Transform sample rate,  128 =  1.536.000 hz  / 12.000 hz (fs H.T.)
            float i = 0.0, q = 0.0;

            // over = 1.536.000/24khz = 64 . (Mic audio has fixed SR in audio_p buffer[]  = 24khz), but in tx mode , we are running Transceiver fs @tx = 1.536.000 Hz.
            sample = audio.p[counter / over] >> audio_shift_bits_s16_AM_DSB_SSB;  // originally fixed   >> 2, now >>2 for AK,   0,1,2,3 for WM (boost off)
            sample *= audio_gain;                                                 // Apply GAIN  Scale factor to the audio TX modulation.

            hilbert.execute(sample / 32768.0f, i, q);
            if (interpolate_x16) {
                dsp::interpolate::ComplexFIRInterpolate8::Output stage_x8;
                interpolator.execute(i, q, stage_x8);
                for (size_t phase = 0; phase < stage_x8.size(); ++phase)
                    interpolator_x2.execute(stage_x8[phase], reconstructed[2 * phase], reconstructed[2 * phase + 1]);
            } else
                reconstructed[0] = {i, q};  // Preserve the legacy 2 kHz path.
        }

        // 3 kHz: sixteen reconstructed outputs per Hilbert input, each held 8
        // times. 2 kHz retains its original update/hold cadence unchanged.
        if (interpolate_x16 ? ((counter & 7) == 0) : (counter % fs_div_factor == 0)) {
            const auto& output = reconstructed[interpolate_x16 ? ((counter >> 3) & 15) : 0];
            float i = output.i;
            float q = output.q;
            const float raw_i = i;
            const float raw_q = q;

            i *= 256.0f;  // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)
            q *= 256.0f;  // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)
            i = ssb_clamp(raw_i, i);
            q = ssb_clamp(raw_q, q);

            switch (mode) {
                case Mode::LSB:
                    re = q;
                    im = i;
                    break;
                case Mode::USB:
                    re = i;
                    im = q;
                    break;
                default:
                    re = 0;
                    im = 0;
                    break;
            }
            // re = q;
            // im = i;
            // break;
        }

        buffer.p[counter] = {re, im};

        // Update vu-meter bar in the LCD screen.
        power_acc += (sample < 0) ? -sample : sample;  // Power average for UI vu-meter

        if (new_power_acc_count) {
            new_power_acc_count--;
        } else {  // power_acc_count = 0
            new_power_acc_count = new_divider;
            new_level_message.value = power_acc / (new_divider * 8);  // Why ?  . This division is to adj vu-meter sentitivity, to match saturation point to red-muter .
            shared_memory.application_queue.push(new_level_message);
            power_acc = 0;
        }
    }
}

///

FM::FM() {
    mode = Mode::FM;
}

void FM::set_fm_delta(uint32_t new_delta) {
    fm_delta = new_delta;
}

void FM::set_tone_gen_configure(const uint32_t set_delta, const float set_tone_mix_weight) {
    tone_gen.configure(set_delta, set_tone_mix_weight);
}

void FM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider) {
    int32_t sample = 0;
    int8_t re, im;

    for (size_t counter = 0; counter < buffer.count; counter++) {
        sample = audio.p[counter >> 6] >> audio_shift_bits_s16_FM;  // Orig. >>8 , 	sample = audio.p[counter / over] >> 8;   (not enough efficient running code, over = 1536000/240000= 64 )
        sample *= audio_gain;                                       // Apply GAIN  Scale factor to the audio TX modulation.

        if (play_beep) {
            sample = apply_beep(sample, configured_in, new_beep_index, new_beep_timer, new_txprogress_message);  // Apply beep -if selected - atom ,sample by sample.
        } else {
            // Update vu-meter bar in the LCD screen.
            power_acc += (sample < 0) ? -sample : sample;  // Power average for UI vu-meter

            if (new_power_acc_count) {
                new_power_acc_count--;
            } else {  // power_acc_count = 0
                new_power_acc_count = new_divider;
                new_level_message.value = power_acc / (new_divider / 4);  // Why ?  . This division is to adj vu-meter sentitivity, to match saturation point to red-muter .
                shared_memory.application_queue.push(new_level_message);
                power_acc = 0;
            }
            // TODO: pending to optimize CPU running code.
            // So far , we can not handle all 3 issues at the same time (vu-meter , CTCSS, beep).
            sample = tone_gen.process(sample);  // Add selected Key_Tone or CTCSS subtone , atom function() , sample by sample.
        }

        delta = sample * fm_delta;  // Modulate FM

        phase += delta;
        sphase = phase >> 24;

        re = (sine_table_i8[(sphase + 64) & 255]);
        im = (sine_table_i8[sphase]);

        buffer.p[counter] = {re, im};
    }
}

AM::AM() {
    mode = Mode::AM;
}

void AM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider) {
    const bool carrier = mode == Mode::AM;
    if (play_beep != was_beep_) {
        // No stale microphone history on either side of the full-rate bypass.
        // Reset does not smooth the pre-existing instantaneous transition.
        microphone_ = {};
        was_beep_ = play_beep;
    }
    const float gain = audio_gain / static_cast<float>(1U << audio_shift_bits_s16_AM_DSB_SSB);
    std::array<float, 16> reconstructed;
    int32_t vu_sample = 0;
    int8_t value = 0;
    for (size_t counter = 0; counter < buffer.count; ++counter) {
        if (play_beep) {
            // Full-rate generator/timers unchanged. Division by four equals
            // the old intended (beep * 32) / 32768 * 256 without signed << UB.
            const float modulation = static_cast<float>(apply_beep(0, configured_in, new_beep_index, new_beep_timer, new_txprogress_message)) * 0.25f;
            value = dsp::am::beep_output(modulation, carrier);
        } else {
            if ((counter & 127) == 0) {
                const size_t input = counter / over;
                microphone_.execute(audio.p[input], audio.p[input + 1], gain, reconstructed);
                // Preserve the original VU's selected sample, integer shift,
                // gain truncation, accumulation cadence and divisor exactly.
                vu_sample = audio.p[input] >> audio_shift_bits_s16_AM_DSB_SSB;
                vu_sample *= audio_gain;
            }
            if ((counter & 7) == 0)
                value = dsp::am::output(reconstructed[(counter >> 3) & 15] / 128.0f, carrier);
            power_acc += (vu_sample < 0) ? -vu_sample : vu_sample;
            if (new_power_acc_count) {
                --new_power_acc_count;
            } else {
                new_power_acc_count = new_divider;
                new_level_message.value = power_acc / (new_divider * 8);
                shared_memory.application_queue.push(new_level_message);
                power_acc = 0;
            }
        }
        buffer.p[counter] = {value, value};
    }
}

}  // namespace modulate
}  // namespace dsp
