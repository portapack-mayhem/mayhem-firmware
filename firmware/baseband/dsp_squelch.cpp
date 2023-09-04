/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "dsp_squelch.hpp"

#include <cstdint>
#include <array>

bool FMSquelch::execute(const buffer_f32_t& audio) {
    if (threshold_squared == 0.0f) {
        return true;
    }

    // TODO: alloca temp buffer, assert audio.count
    std::array<float, 32> squelch_energy_buffer;
    const buffer_f32_t squelch_energy{squelch_energy_buffer.data(), audio.count};
    non_audio_hpf.execute(audio, squelch_energy);

    // "Non-audio" implies "noise" here. Find the loudest noise sample.
    float non_audio_max_squared = 0;
    for (size_t i = 0; i < squelch_energy.count; ++i) {
        auto sample = squelch_energy.p[i];
        float sample_squared = sample * sample;

        if (sample_squared > non_audio_max_squared)
            non_audio_max_squared = sample_squared;
    }

    // Is the noise less than the threshold?
    return (non_audio_max_squared < threshold_squared);
}

void FMSquelch::set_threshold(const float new_value) {
    threshold_squared = new_value * new_value;
}

bool FMSquelch::enabled() const {
    return threshold_squared > 0.0;
}
