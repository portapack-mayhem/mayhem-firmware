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

namespace dsp {
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

void SSB::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer, bool& configured_in, uint32_t& new_beep_index, uint32_t& new_beep_timer, TXProgressMessage& new_txprogress_message, AudioLevelReportMessage& new_level_message, uint32_t& new_power_acc_count, uint32_t& new_divider) {
    // unused
    (void)configured_in;
    (void)new_beep_index;
    (void)new_beep_timer;
    (void)new_txprogress_message;

    // No way to activate correctly  the roger beep in this option, Maybe not enough M4 CPU power , Let's  block roger beep in SSB  selection by now .
    int32_t sample = 0;
    int8_t re = 0, im = 0;

    for (size_t counter = 0; counter < buffer.count; counter++) {
        if (counter % fs_div_factor == 0) {  // Ex. TX bw_ssb 3khz =fs/4 Hilbert Transform sample rate,  128 =  1.536.000 hz  / 12.000 hz (fs H.T.)
            float i = 0.0, q = 0.0;

            // over = 1.536.000/24khz = 64 . (Mic audio has fixed SR in audio_p buffer[]  = 24khz), but in tx mode , we are running Transceiver fs @tx = 1.536.000 Hz.
            sample = audio.p[counter / over] >> audio_shift_bits_s16_AM_DSB_SSB;  // originally fixed   >> 2, now >>2 for AK,   0,1,2,3 for WM (boost off)
            sample *= audio_gain;                                                 // Apply GAIN  Scale factor to the audio TX modulation.

            // switch (mode) {
            // case Mode::LSB:
            hilbert.execute(sample / 32768.0f, i, q);
            // case Mode::USB:	hilbert.execute(sample / 32768.0f, q, i);
            // default:	break;
            // }

            i *= 256.0f;  // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)
            q *= 256.0f;  // Original 64.0f,  now x 4 (+12 dB's SSB BB modulation)
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
    int32_t sample = 0;
    int8_t re = 0, im = 0;
    float q = 0.0;

    for (size_t counter = 0; counter < buffer.count; counter++) {
        if (counter % 128 == 0) {
            sample = audio.p[counter / over] >> audio_shift_bits_s16_AM_DSB_SSB;  // originally fixed >> 2, now >>2 for AK, 0,1,2,3 for WM (boost off)
            sample *= audio_gain;                                                 // Apply GAIN  Scale factor to the audio TX modulation.
        }

        if (play_beep) {
            sample = apply_beep(sample, configured_in, new_beep_index, new_beep_timer, new_txprogress_message) << 5;  // Apply beep -if selected - atom sample by sample.
        } else {
            // Update vu-meter bar in the LCD screen.
            power_acc += (sample < 0) ? -sample : sample;  // Power average for UI vu-meter

            if (new_power_acc_count) {
                new_power_acc_count--;
            } else {  // power_acc_count = 0
                new_power_acc_count = new_divider;
                new_level_message.value = power_acc / (new_divider * 8);  // Why ?orig / (new_divider / 4);	// Why ?
                shared_memory.application_queue.push(new_level_message);
                power_acc = 0;
            }
        }

        q = sample / 32768.0f;
        q *= 256.0f;  // Original 64.0f,now x4 (+12 dB's BB_modulation in AM & DSB)
        switch (mode) {
            case Mode::AM:
                re = q + 80;
                im = q + 80;
                break;  // Original DC add +20_DC_level=carrier,now x4 (+12dB's AM carrier)
            case Mode::DSB:
                re = q;
                im = q;
                break;
            default:
                break;
        }
        buffer.p[counter] = {re, im};
    }
}

}  // namespace modulate
}  // namespace dsp
