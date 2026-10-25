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

// Debugger-visible Replay timing/clipping diagnostics; enabled by default.
// Disable with -DREPLAY_INTERPOLATION_DIAGNOSTICS=0.
#ifndef REPLAY_INTERPOLATION_DIAGNOSTICS
#define REPLAY_INTERPOLATION_DIAGNOSTICS 1
#endif

#if REPLAY_INTERPOLATION_DIAGNOSTICS
struct ReplayInterpolationDiagnostics {
    uint32_t callback_count;
    uint32_t last_cycles;
    uint32_t max_cycles;
    uint32_t clipped_components;
    uint32_t short_reads;
    uint32_t logical_sample_rate;
    uint32_t interpolation_factor;  // Total factor, including post-FIR repetition.
    uint32_t source_sample_rate;
    uint32_t fir_factor;
    uint32_t repetition_factor;
    uint32_t estimated_cycles;
    uint32_t deadline_cycles;
};
volatile ReplayInterpolationDiagnostics replay_interpolation_diagnostics{};
#endif

ReplayProcessor::ReplayProcessor() {
    interpolation.configure(schedule.total_factor, schedule.fir_factor());
#if REPLAY_INTERPOLATION_DIAGNOSTICS
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif
    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * 1000000;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * 1000000;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * 1000000;

    spectrum_samples = 0;

    channel_spectrum.set_decimation_factor(1);

    configured = false;
    baseband_thread.start();
}

void ReplayProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured || !stream) {
        std::fill_n(buffer.p, buffer.count, complex8_t{});
        return;
    }
#if REPLAY_INTERPOLATION_DIAGNOSTICS
    const auto start_cycles = DWT->CYCCNT;
    uint32_t clipped = 0;
#endif

    const size_t factor = schedule.total_factor;
    const size_t source_count = buffer.count / factor;
    if (source_count * factor != buffer.count) {
        std::fill_n(buffer.p, buffer.count, complex8_t{});
        return;
    }
    spectrum_samples += buffer.count;
    const bool update_spectrum = spectrum_samples >= spectrum_interval_samples;
    if (update_spectrum) spectrum_samples -= spectrum_interval_samples;
    // Keep the old average frame cadence, but collect each frame contiguously.
    if (update_spectrum)
        spectrum_capture_credit = std::min<size_t>(256, spectrum_capture_credit + std::min<size_t>(source_count, 256));
    if (!spectrum_capture_remaining && spectrum_capture_credit == 256) {
        spectrum_capture_credit = 0;
        spectrum_capture_remaining = 256;
    }
    size_t current_bytes_read = 0;
    const size_t bytes_to_read = source_count * (input_is_c8 ? sizeof(complex8_t) : sizeof(complex16_t));

    if (input_is_c8) {
        // Native signed C8 is already the hardware format. This path is only
        // enabled for x1; expand a bounded prefix solely for the spectrum UI.
        current_bytes_read = stream->read(buffer.p, bytes_to_read);
        const size_t complete = current_bytes_read / sizeof(complex8_t);
        std::fill_n(buffer.p + complete, buffer.count - complete, complex8_t{});
        if (spectrum_capture_remaining && complete) {
            const size_t count = std::min<size_t>(complete, spectrum_capture_remaining);
            for (size_t n = 0; n < count; ++n)
                iq[n] = complex16_t{static_cast<int16_t>(buffer.p[n].real() * 256),
                                    static_cast<int16_t>(buffer.p[n].imag() * 256)};
            feed_spectrum(iq.data(), count);
        }
        if (current_bytes_read != bytes_to_read && spectrum_capture_remaining) {
            channel_spectrum.discard_partial();
            spectrum_capture_remaining = 0;
        }
        // Progress is expressed in common C16 byte units by existing callers.
        bytes_read += current_bytes_read * 2;
    } else {
        bool starved = false;
        for (size_t offset = 0; offset < source_count;) {
            const size_t count = std::min(iq.size(), source_count - offset);
            const size_t got = starved ? 0 : stream->read(iq.data(), count * sizeof(complex16_t));
            current_bytes_read += got;
            starved |= got != count * sizeof(complex16_t);
            const size_t samples = got / sizeof(complex16_t);
            interpolation.execute_buffer(iq.data(), samples, buffer.p + offset * factor, count * factor
#if REPLAY_INTERPOLATION_DIAGNOSTICS
                                         ,
                                         &clipped
#endif
            );
            feed_spectrum(iq.data(), samples);
            if (got != count * sizeof(complex16_t) && spectrum_capture_remaining) {
                // Do not join a partial frame across a missing source interval.
                channel_spectrum.discard_partial();
                spectrum_capture_remaining = 0;
            }
            // After a short read, deterministic zeros flush all remaining
            // chunks this callback; retry the FIFO on the next callback.
            offset += count;
        }
        bytes_read += current_bytes_read;
    }
    if (update_spectrum) {
        txprogress_message.progress = bytes_read;
        txprogress_message.done = false;
        shared_memory.application_queue.push(txprogress_message);
    }
#if REPLAY_INTERPOLATION_DIAGNOSTICS
    const uint32_t elapsed = DWT->CYCCNT - start_cycles;
    ++replay_interpolation_diagnostics.callback_count;
    replay_interpolation_diagnostics.last_cycles = elapsed;
    if (elapsed > replay_interpolation_diagnostics.max_cycles)
        replay_interpolation_diagnostics.max_cycles = elapsed;
    replay_interpolation_diagnostics.clipped_components += clipped;
    if (current_bytes_read != bytes_to_read) ++replay_interpolation_diagnostics.short_reads;
#endif
}

void ReplayProcessor::reset_spectrum_capture() {
    channel_spectrum.discard_partial();
    spectrum_capture_remaining = 0;
    spectrum_capture_credit = 0;
}

void ReplayProcessor::feed_spectrum(complex16_t* samples, size_t count) {
    if (!spectrum_capture_remaining || !count) return;
    const size_t take = std::min<size_t>(count, spectrum_capture_remaining);
    // At most one block can complete; no surplus enters the next frame.
    if (channel_spectrum.feed({samples, take, schedule.source_rate},
                              channel_filter_low_f, channel_filter_high_f, channel_filter_transition)) {
        spectrum_capture_remaining = 0;
    } else {
        spectrum_capture_remaining -= take;
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
            configured = schedule.valid() && stream && (!input_is_c8 || schedule.total_factor == 1);
            break;

        default:
            break;
    }
}

void ReplayProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    configured = false;
    reset_spectrum_capture();
    schedule = replay::for_total(message.sample_rate, toUType(message.oversample_rate));
    if (!schedule.valid()) return;
    baseband_fs = schedule.output_rate;
    interpolation.configure(schedule.total_factor, schedule.fir_factor());
    baseband_thread.set_sampling_rate(baseband_fs);

    spectrum_interval_samples = baseband_fs / spectrum_rate_hz;
}

void ReplayProcessor::replay_config(const ReplayConfigMessage& message) {
    reset_spectrum_capture();
    interpolation.reset();
    spectrum_samples = 0;
#if REPLAY_INTERPOLATION_DIAGNOSTICS
    // Preserve the final measurement when stopping, so it remains inspectable.
    if (message.config) {
        replay_interpolation_diagnostics.callback_count = 0;
        replay_interpolation_diagnostics.last_cycles = 0;
        replay_interpolation_diagnostics.max_cycles = 0;
        replay_interpolation_diagnostics.clipped_components = 0;
        replay_interpolation_diagnostics.short_reads = 0;
        replay_interpolation_diagnostics.logical_sample_rate = baseband_fs;
        replay_interpolation_diagnostics.interpolation_factor = schedule.total_factor;
        replay_interpolation_diagnostics.source_sample_rate = schedule.source_rate;
        replay_interpolation_diagnostics.fir_factor = schedule.fir_factor();
        replay_interpolation_diagnostics.repetition_factor = schedule.repetition_factor;
        replay_interpolation_diagnostics.estimated_cycles = schedule.estimated_cycles;
        replay_interpolation_diagnostics.deadline_cycles = uint64_t(200000000) * 2048 / baseband_fs;
    }
#endif
    if (message.config) {
        stream = std::make_unique<StreamOutput>(message.config);
        input_is_c8 = message.config->input_is_c8;

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
