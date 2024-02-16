/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "proc_audiotx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

void AudioTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    buffer_s16_t audio_buffer{audio_data, AUDIO_OUTPUT_BUFFER_SIZE, sampling_rate};
    int16_t audio_sample_s16;

    // Zero-order hold (poop)
    for (size_t i = 0; i < buffer.count; i++) {
        resample_acc += resample_inc;
        if (resample_acc >= 0x10000) {
            resample_acc -= 0x10000;
            if (stream) {
                audio_sample = 0;
                stream->read(&audio_sample, bytes_per_sample);  // assumes little endian when reading 1 byte
                samples_read++;
            }
        }

        if (bytes_per_sample == 1) {
            sample = audio_sample - 0x80;
            audio_sample_s16 = sample * 256;
        } else {
            audio_sample_s16 = (int16_t)audio_sample;
            sample = audio_sample_s16 / 256;
        }

        // Output to speaker too
        if (!tone_key_enabled) {
            uint32_t imod32 = i & (AUDIO_OUTPUT_BUFFER_SIZE - 1);
            audio_data[imod32] = audio_sample_s16;
            if (imod32 == (AUDIO_OUTPUT_BUFFER_SIZE - 1))
                audio_output.write_unprocessed(audio_buffer);
        }

        sample = tone_gen.process(sample);

        // FM
        delta = sample * fm_delta;

        phase += delta;
        sphase = phase + (64 << 24);

        re = sine_table_i8[(sphase & 0xFF000000U) >> 24];
        im = sine_table_i8[(phase & 0xFF000000U) >> 24];

        buffer.p[i] = {(int8_t)re, (int8_t)im};
    }

    progress_samples += buffer.count;
    if (progress_samples >= progress_interval_samples) {
        progress_samples -= progress_interval_samples;

        txprogress_message.progress = samples_read;  // Inform UI about progress
        txprogress_message.done = false;
        shared_memory.application_queue.push(txprogress_message);
    }
}

void AudioTXProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::AudioTXConfig:
            audio_config(*reinterpret_cast<const AudioTXConfigMessage*>(message));
            break;

        case Message::ID::ReplayConfig:
            configured = false;
            samples_read = 0;
            replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
            break;

        case Message::ID::SampleRateConfig:
            sample_rate_config(*reinterpret_cast<const SampleRateConfigMessage*>(message));
            break;

        case Message::ID::FIFOData:
            configured = true;
            break;

        default:
            break;
    }
}

void AudioTXProcessor::audio_config(const AudioTXConfigMessage& message) {
    fm_delta = message.deviation_hz * (0xFFFFFFULL / baseband_fs);
    tone_gen.configure(message.tone_key_delta, message.tone_key_mix_weight);
    progress_interval_samples = message.divider;
    resample_acc = 0;
    bytes_per_sample = message.bits_per_sample / 8;
    audio_output.configure(false);

    tone_key_enabled = (message.tone_key_delta != 0);
    audio::dma::shrink_tx_buffer(!tone_key_enabled);
}

void AudioTXProcessor::replay_config(const ReplayConfigMessage& message) {
    if (message.config) {
        stream = std::make_unique<StreamOutput>(message.config);

        // Tell application that the buffers and FIFO pointers are ready, prefill
        shared_memory.application_queue.push(sig_message);
    } else {
        stream.reset();
    }
}

void AudioTXProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    resample_inc = (((uint64_t)message.sample_rate) << 16) / baseband_fs;  // 16.16 fixed point message.sample_rate
    sampling_rate = message.sample_rate;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<AudioTXProcessor>()};
    event_dispatcher.run();
    return 0;
}
