/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "proc_replay.hpp"
#include "sine_table_int8.hpp"
#include "portapack_shared_memory.hpp"

#include "event_m4.hpp"

#include "utility.hpp"

ReplayProcessor::ReplayProcessor() {
    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * 1000000;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * 1000000;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * 1000000;

    spectrum_samples = 0;

    channel_spectrum.set_decimation_factor(1);

    configured = false;
    baseband_thread.start();
}

// Change to 1 to enable buffer assertions in replay.
#define BUFFER_SIZE_ASSERT 0

void ReplayProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured || !stream) return;

    // Because this is actually adding samples, alias
    // oversample_rate so the math below is more clear.
    const size_t interpolation_factor = toUType(oversample_rate);

    // Wrap the IQ data array in a buffer with the correct sample_rate.
    buffer_c16_t iq_buffer{iq.data(), iq.size(), baseband_fs / interpolation_factor};

    // The IQ data in stream is C16 format and needs to be converted to C8 (N * 2).
    // The data also needs to be interpolated so the effective sample rate is closer
    // to 4Mhz. Because interpolation repeats a sample multiple times, fewer bytes
    // are needed from the source stream in order to fill the buffer (count / oversample).
    // Together the C16->C8 conversion and the interpolation give the number of
    // bytes that need to be read from the source stream.
    const size_t samples_to_read = buffer.count / interpolation_factor;
    const size_t bytes_to_read = samples_to_read * sizeof(buffer_c16_t::Type);

#if BUFFER_SIZE_ASSERT
    // Verify the output buffer size is divisible by the interpolation factor.
    if (samples_to_read * interpolation_factor != buffer.count)
        chDbgPanic("Output not div.");

    // Is the input smaple buffer big enough?
    if (samples_to_read > iq_buffer.count)
        chDbgPanic("IQ buf ovf.");
#endif

    // Read the C16 IQ data from the source stream.
    size_t current_bytes_read = stream->read(iq_buffer.p, bytes_to_read);

    // Compute the number of samples were actually read from the source.
    size_t samples_read = current_bytes_read / sizeof(buffer_c16_t::Type);

    // Write converted source samples to the output buffer with interpolation.
    for (auto i = 0u; i < samples_read; ++i) {
        int8_t re_out = iq_buffer.p[i].real() >> 8;
        int8_t im_out = iq_buffer.p[i].imag() >> 8;
        auto out_value = buffer_c8_t::Type{re_out, im_out};

        // Interpolate sample.
        for (auto j = 0u; j < interpolation_factor; ++j) {
            size_t index = i * interpolation_factor + j;
            buffer.p[index] = out_value;

#if BUFFER_SIZE_ASSERT
            // Verify the index is within bounds.
            if (index >= buffer.count)
                chDbgPanic("Output bounds");
#endif
        }
    }

    // Update tracking stats.
    bytes_read += current_bytes_read;
    spectrum_samples += samples_read * interpolation_factor;

    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        channel_spectrum.feed(
            iq_buffer, channel_filter_low_f,
            channel_filter_high_f, channel_filter_transition);

        // Inform UI about progress.
        txprogress_message.progress = bytes_read;
        txprogress_message.done = false;
        shared_memory.application_queue.push(txprogress_message);
    }
}

void ReplayProcessor::on_message(const Message* const message) {
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

void ReplayProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    baseband_fs = message.sample_rate * toUType(message.oversample_rate);
    oversample_rate = message.oversample_rate;
    baseband_thread.set_sampling_rate(baseband_fs);

    spectrum_interval_samples = baseband_fs / spectrum_rate_hz;
}

void ReplayProcessor::replay_config(const ReplayConfigMessage& message) {
    if (message.config) {
        stream = std::make_unique<StreamOutput>(message.config);

        // Tell application that the buffers and FIFO pointers are ready, prefill
        shared_memory.application_queue.push(sig_message);
    } else {
        stream.reset();
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<ReplayProcessor>()};
    event_dispatcher.run();
    return 0;
}
