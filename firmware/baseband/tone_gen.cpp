/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2024 Mark Thompson
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

#include "tone_gen.hpp"
#include "sine_table_int8.hpp"

// Functions for audio beep (used by Sonde RSSI)
void ToneGen::configure_beep(const uint32_t freq, const uint32_t sample_rate) {
    f_delta_ = (float)(freq * sizeof(sine_table_i8)) / sample_rate;
    f_tone_phase_ = 0.0;

    // For higher frequencies, start at sine peak to handle case of freq=sample_rate/2;
    // we don't want to sample the sine wave only when it's crossing 0!
    // There is still an amplitude issue though depending on magnitude of selected sine sample deltas.
    if (f_delta_ >= sizeof(sine_table_i8) / 4)
        f_tone_phase_ = sizeof(sine_table_i8) / 4;
}

int16_t ToneGen::process_beep() {
    int16_t tone_sample = sine_table_i8[(uint32_t)f_tone_phase_ & 0xFF] * 256;
    f_tone_phase_ += f_delta_;

    return tone_sample;
}

// ----Original available core SW code from fw 1.3.1 ,  Working also well in Mic App CTCSS Gen from fw 1.4.0 onwards

// Original direct-look-up synthesis algorithm with Fractional delta phase. It is OK
// Delta  and Accumulator fase are stored in 32 bits (4 bytes), 1st top byte used as Modulo-256 Sine look-up table [index]
// the lower 3 bytes (24 bits)  are used as a Fractional Detla and Accumulator phase, to have very finer Fstep control.

void ToneGen::configure(const uint32_t delta, const float tone_mix_weight) {
    delta_ = delta;
    tone_phase_ = 0;
    tone_mix_weight_ = tone_mix_weight;
    input_mix_weight_ = 1.0 - tone_mix_weight;
}

int32_t ToneGen::process(const int32_t sample_in) {
    if (!delta_)
        return sample_in;

    int32_t tone_sample = sine_table_i8[(tone_phase_ & 0xFF000000U) >> 24];
    tone_phase_ += delta_;

    return (sample_in * input_mix_weight_) + (tone_sample * tone_mix_weight_);
}