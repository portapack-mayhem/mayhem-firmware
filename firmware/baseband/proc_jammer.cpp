/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2025 RocketGod - Added modes from my Flipper Zero RF Jammer App - https://betaskynet.com
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

#include "proc_jammer.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void JammerProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    for (size_t i = 0; i < buffer.count; i++) {
        if (!jammer_duration) {
            do {
                current_range++;
                if (current_range == JAMMER_MAX_CH) current_range = 0;
            } while (!jammer_channels[current_range].enabled);

            jammer_duration = jammer_channels[current_range].duration;
            jammer_bw = jammer_channels[current_range].width / 2;

            message.freq = jammer_channels[current_range].center;
            message.range = current_range;
            shared_memory.application_queue.push(message);
        } else {
            jammer_duration--;
        }

        if (!period_counter) {
            period_counter = noise_period;

            if (noise_type == jammer::JammerType::TYPE_FSK) {
                sample = (sample + lfsr) >> 1;
            } else if (noise_type == jammer::JammerType::TYPE_TONE) {
                tone_delta = 150000 + (lfsr >> 9);
            } else if (noise_type == jammer::JammerType::TYPE_SWEEP) {
                sample++;
            } else if (noise_type == jammer::JammerType::TYPE_RANDOM) {
                sample = lfsr & 0xFF;
            } else if (noise_type == jammer::JammerType::TYPE_SINE) {
                wave_phase += 0x01000000;
                sample = sine_table_i8[(wave_phase >> 24) & 0xFF];
            } else if (noise_type == jammer::JammerType::TYPE_SQUARE) {
                wave_index = (wave_index + 1) % 2;
                sample = wave_index ? 127 : -128;
            } else if (noise_type == jammer::JammerType::TYPE_SAWTOOTH) {
                wave_index = (wave_index + 1) % 256;
                sample = (wave_index * 127) / 255 - 128;
            } else if (noise_type == jammer::JammerType::TYPE_TRIANGLE) {
                wave_index = (wave_index + 1) % 256;
                sample = (wave_index < 128 ? wave_index : (255 - wave_index)) * 127 / 127 - 128;
            } else if (noise_type == jammer::JammerType::TYPE_CHIRP) {
                chirp_freq += 0.01f;
                if (chirp_freq > 1.0f) chirp_freq = 0.0f;
                wave_phase += static_cast<uint32_t>(0x01000000 * (1.0f + chirp_freq));
                sample = sine_table_i8[(wave_phase >> 24) & 0xFF];
            } else if (noise_type == jammer::JammerType::TYPE_GAUSSIAN) {
                float u1 = static_cast<float>(lfsr & 0xFFFF) / 0x10000;
                float u2 = static_cast<float>((lfsr >> 16) & 0xFFFF) / 0x10000;
                float gaussian = std::sqrt(-2.0f * std::log(u1)) * std::cos(2 * M_PI * u2);
                sample = static_cast<int8_t>(gaussian * 32);
            } else if (noise_type == jammer::JammerType::TYPE_BRUTEFORCE) {
                sample = 127;
            }

            feedback = ((lfsr >> 31) ^ (lfsr >> 29) ^ (lfsr >> 15) ^ (lfsr >> 11)) & 1;
            lfsr = (lfsr << 1) | feedback;
            if (!lfsr) lfsr = 0x1337;
        } else {
            period_counter--;
        }

        if (noise_type == jammer::JammerType::TYPE_TONE) {
            aphase += tone_delta;
            sample = sine_table_i8[(aphase & 0xFF000000) >> 24];
        }

        delta = sample * jammer_bw;

        phase += delta;
        sphase = phase + (64 << 24);

        re = sine_table_i8[(sphase & 0xFF000000) >> 24];
        im = sine_table_i8[(phase & 0xFF000000) >> 24];

        buffer.p[i] = {re, im};
    }
}

void JammerProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::JammerConfigure) {
        const auto message = *reinterpret_cast<const JammerConfigureMessage*>(msg);

        if (message.run) {
            jammer_channels = (JammerChannel*)shared_memory.bb_data.data;
            noise_type = message.type;
            noise_period = 3072000 / message.speed;
            if (noise_type == jammer::JammerType::TYPE_SWEEP || noise_type == jammer::JammerType::TYPE_SINE ||
                noise_type == jammer::JammerType::TYPE_SQUARE || noise_type == jammer::JammerType::TYPE_SAWTOOTH ||
                noise_type == jammer::JammerType::TYPE_TRIANGLE || noise_type == jammer::JammerType::TYPE_CHIRP ||
                noise_type == jammer::JammerType::TYPE_GAUSSIAN || noise_type == jammer::JammerType::TYPE_BRUTEFORCE)
                noise_period >>= 8;
            period_counter = 0;
            jammer_duration = 0;
            current_range = 0;
            lfsr = 0xDEAD0012;
            wave_phase = 0;
            wave_index = 0;
            chirp_freq = 0.0f;

            configured = true;
        } else {
            configured = false;
        }
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<JammerProcessor>()};
    event_dispatcher.run();
    return 0;
}
