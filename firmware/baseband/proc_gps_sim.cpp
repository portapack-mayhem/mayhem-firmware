/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2020 Shao
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

#include "proc_gps_sim.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include "utility.hpp"

GPSReplayProcessor::GPSReplayProcessor() {
    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * 1000000;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * 1000000;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * 1000000;

    spectrum_samples = 0;

    channel_spectrum.set_decimation_factor(1);

    configured = false;
    baseband_thread.start();
}

void GPSReplayProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.6MHz, 2048 samples */

    if (!configured || !stream) return;

    // File data is in C8 format, which is what we need
    // File samplerate is 2.6MHz, which is what we need
    // To fill up the 2048-sample C8 buffer @ 2 bytes per sample = 4096 bytes
    const size_t bytes_to_read = sizeof(*buffer.p) * 1 * (buffer.count);
    size_t bytes_read_this_iteration = stream->read(iq_buffer.p, bytes_to_read);
    size_t samples_read_this_iteration = bytes_read_this_iteration / sizeof(*buffer.p);

    bytes_read += bytes_read_this_iteration;

    // NB: Couldn't we have just read the data into buffer.p to start with, or some DMA/cache coherency concern?
    //
    // for (size_t i = 0; i < buffer.count; i++) {
    //     auto re_out = iq_buffer.p[i].real();
    //     auto im_out = iq_buffer.p[i].imag();
    //     buffer.p[i] = {(int8_t)re_out, (int8_t)im_out};
    // }
    memcpy(buffer.p, iq_buffer.p, bytes_read_this_iteration);  // memcpy should be more efficient than 1 byte at a time

    spectrum_samples += samples_read_this_iteration;
    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;

        txprogress_message.progress = bytes_read;  // Inform UI about progress

        txprogress_message.done = false;
        shared_memory.application_queue.push(txprogress_message);
    }
}

void GPSReplayProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(message);
            break;

        case Message::ID::SampleRateConfig:
            sample_rate_config(*reinterpret_cast<const SampleRateConfigMessage*>(message));
            break;

        case Message::ID::ReplayConfig:
            configured = false;
            bytes_read = 0;
            replay_config(*reinterpret_cast<const ReplayConfigMessage*>(message));
            break;

        // App has prefilled the buffers, we're ready to go now
        case Message::ID::FIFOData:
            configured = true;
            break;

        default:
            break;
    }
}

void GPSReplayProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    baseband_fs = message.sample_rate;
    baseband_thread.set_sampling_rate(baseband_fs);
    spectrum_interval_samples = baseband_fs / spectrum_rate_hz;
}

void GPSReplayProcessor::replay_config(const ReplayConfigMessage& message) {
    if (message.config) {
        stream = std::make_unique<StreamOutput>(message.config);

        // Tell application that the buffers and FIFO pointers are ready, prefill
        shared_memory.application_queue.push(sig_message);
    } else {
        stream.reset();
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<GPSReplayProcessor>()};
    event_dispatcher.run();
    return 0;
}
