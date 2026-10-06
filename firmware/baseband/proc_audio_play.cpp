/*
 * Copyright (C) 2026 SecLBL
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

#include "proc_audio_play.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"
#include "utility.hpp"

#include <hal.h>
#include <cmath>

namespace {

/* Equalizer bands, as http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt filters at
 * fs = 48 kHz. cos/sin of w0 = 2*pi*f/fs are tabulated to keep libm trig out of the image. */
enum class Shape { LowShelf,
                   Peak,
                   HighShelf };

struct band_t {
    Shape shape;
    float cos_w0;
    float sin_w0;
};

constexpr band_t bands[AudioPlayConfigMessage::eq_bands] = {
    {Shape::LowShelf, 0.99991433f, 0.01308960f},   // 100 Hz
    {Shape::Peak, 0.99922904f, 0.03925982f},       // 300 Hz
    {Shape::Peak, 0.99144486f, 0.13052619f},       // 1 kHz
    {Shape::Peak, 0.89687274f, 0.44228869f},       // 3.5 kHz
    {Shape::HighShelf, 0.25881905f, 0.96592583f},  // 10 kHz
};

constexpr float peak_q = 0.8f;  // bands are ~1.8 octaves apart
constexpr int max_db = 12;

// 10^(dB/40) for 0..12 dB, the cookbook's "A".
constexpr float amplitude[max_db + 1] = {
    1.000000f, 1.059254f, 1.122018f, 1.188502f, 1.258925f, 1.333521f, 1.412538f,
    1.496236f, 1.584893f, 1.678804f, 1.778279f, 1.883649f, 1.995262f};

// sin^2(w/2) at 24 frequencies from 30 Hz to 18 kHz, where the EQ curve is sampled.
constexpr float probes[] = {
    3.855309e-06f, 6.724147e-06f, 1.172775e-05f, 2.045464e-05f,
    3.567534e-05f, 6.222181e-05f, 1.085211e-04f, 1.892696e-04f,
    3.300950e-04f, 5.756808e-04f, 1.003918e-03f, 1.750525e-03f,
    3.051813e-03f, 5.318720e-03f, 9.264271e-03f, 1.612084e-02f,
    2.800383e-02f, 4.850010e-02f, 8.355753e-02f, 1.426352e-01f,
    2.395641e-01f, 3.909503e-01f, 6.058505e-01f, 8.535534e-01f};

// |c0 + c1 z^-1 + c2 z^-2|^2 on the unit circle, with s = sin^2(w/2). This form keeps
// its precision at low frequencies, where the coefficients nearly cancel.
float magnitude_squared(const std::array<float, 3>& c, float s) {
    const float sum = c[0] + c[1] + c[2];
    return sum * sum - 4.0f * (c[0] * c[1] + 4.0f * c[0] * c[2] + c[1] * c[2]) * s + 16.0f * c[0] * c[2] * s * s;
}

iir_biquad_config_t band_config(const band_t& band, int db) {
    const float A = (db >= 0) ? amplitude[db] : 1.0f / amplitude[-db];
    const float c = band.cos_w0;
    float b0, b1, b2, a0, a1, a2;

    if (band.shape == Shape::Peak) {
        const float alpha = band.sin_w0 / (2.0f * peak_q);
        b0 = 1.0f + alpha * A;
        b1 = -2.0f * c;
        b2 = 1.0f - alpha * A;
        a0 = 1.0f + alpha / A;
        a1 = -2.0f * c;
        a2 = 1.0f - alpha / A;
    } else {
        // Shelf slope S = 1, and the high shelf is the low shelf with cos(w0) negated
        // (which also flips the sign of b1 and a1).
        const float s = (band.shape == Shape::LowShelf) ? 1.0f : -1.0f;
        const float k = 2.0f * sqrtf(A) * band.sin_w0 * 0.70710678f;
        const float cs = c * s;
        b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + k);
        b1 = s * 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs);
        b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - k);
        a0 = (A + 1.0f) + (A - 1.0f) * cs + k;
        a1 = s * -2.0f * ((A - 1.0f) + (A + 1.0f) * cs);
        a2 = (A + 1.0f) + (A - 1.0f) * cs - k;
    }

    return {{{b0 / a0, b1 / a0, b2 / a0}}, {{1.0f, a1 / a0, a2 / a0}}};
}

}  // namespace

void AudioPlayProcessor::execute(const buffer_c8_t& buffer) {
    for (size_t i = 0; i < buffer.count; i++)
        buffer.p[i] = {0, 0};

    if (wanted_ready) {
        wanted_ready = false;
        channels = wanted.channels;
        preamp = wanted.preamp;
        eq_flat = wanted.flat;
        for (size_t b = 0; b < eq_bands; b++) {
            eq[0][b].configure(wanted.band[b]);
            eq[1][b].configure(wanted.band[b]);
        }
    }

    // Always feed the codec: left alone it loops its last buffers, which is heard as a
    // tone between tracks. No stream or a short read (end of file, late SD card) is silence.
    int16_t audio[audio_block * 2]{};
    const size_t frame_bytes = channels * sizeof(int16_t);
    if (configured && stream)
        stream->read(audio, audio_block * frame_bytes);

    if (!eq_flat) {
        for (size_t c = 0; c < channels; c++) {
            float samples[audio_block];
            for (size_t i = 0; i < audio_block; i++)
                samples[i] = audio[i * channels + c] * preamp;
            for (auto& filter : eq[c])
                filter.execute_in_place(buffer_f32_t{samples, audio_block, 48000});
            for (size_t i = 0; i < audio_block; i++)
                audio[i * channels + c] = __SSAT((int32_t)samples[i], 16);
        }
    }

    auto out = audio::dma::tx_empty_buffer();
    for (size_t i = 0; i < out.count; i++) {
        out.p[i].left = audio[i * channels];
        out.p[i].right = audio[i * channels + channels - 1];
    }
}

void AudioPlayProcessor::prepare(const AudioPlayConfigMessage& message) {
    wanted_ready = false;  // execute() must not take it while it is being written

    wanted.channels = (message.channels == 2) ? 2 : 1;
    wanted.flat = true;
    for (size_t b = 0; b < eq_bands; b++) {
        const int db = clip<int>(message.eq_gain_db[b], -max_db, max_db);
        if (db != 0) wanted.flat = false;
        wanted.band[b] = band_config(bands[b], db);
    }

    // Neighbouring bands overlap, so the curve can peak above the largest single boost.
    // Sample the curve for its peak and scale the input down by that much, plus 0.5 dB
    // for what can hide between two samples.
    float peak = 1.0f;
    for (const float s : probes) {
        float power = 1.0f;
        for (const auto& band : wanted.band)
            power *= magnitude_squared(band.b, s) / magnitude_squared(band.a, s);
        if (power > peak) peak = power;
    }
    wanted.preamp = wanted.flat ? 1.0f : 1.0f / sqrtf(peak * 1.122f);

    wanted_ready = true;
}

void AudioPlayProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::ReplayConfig: {
            const auto config = reinterpret_cast<const ReplayConfigMessage*>(message)->config;
            configured = false;
            if (config) {
                stream = std::make_unique<StreamOutput>(config);
                // Tell application that the buffers and FIFO pointers are ready, prefill
                shared_memory.application_queue.push(sig_message);
            } else {
                stream.reset();
            }
            break;
        }

        case Message::ID::AudioPlayConfig:
            prepare(*reinterpret_cast<const AudioPlayConfigMessage*>(message));
            break;

        case Message::ID::FIFOData:
            configured = true;
            break;

        default:
            break;
    }
}

int main() {
    audio::dma::init_audio_out();

    EventDispatcher event_dispatcher{std::make_unique<AudioPlayProcessor>()};
    event_dispatcher.run();
    return 0;
}
