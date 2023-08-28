/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "proc_capture.hpp"

#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"
#include "utility.hpp"

using namespace dsp::decimate;

CaptureProcessor::CaptureProcessor() {
    channel_spectrum.set_decimation_factor(1);
    baseband_thread.start();
}

void CaptureProcessor::execute(const buffer_c8_t& buffer) {
    auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    auto out_buffer = decim_1.execute(decim_0_out, dst_buffer);

    if (stream) {
        const size_t bytes_to_write = sizeof(*out_buffer.p) * out_buffer.count;
        const size_t written = stream->write(out_buffer.p, bytes_to_write);
        if (written != bytes_to_write) {
            // TODO: Send an error message to the app?
        }
    }

    feed_channel_stats(out_buffer);

    // TODO: per Brumi spectrum seems to only want 256 samples?
    // Just call twice in that case?
    spectrum_samples += out_buffer.count;
    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        channel_spectrum.feed(
            out_buffer,
            channel_filter_low_f,
            channel_filter_high_f,
            channel_filter_transition);
    }
}

void CaptureProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::UpdateSpectrum:
        case Message::ID::SpectrumStreamingConfig:
            channel_spectrum.on_message(message);
            break;

        case Message::ID::SampleRateConfig:
            sample_rate_config(*reinterpret_cast<const SampleRateConfigMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        default:
            break;
    }
}

void CaptureProcessor::sample_rate_config(const SampleRateConfigMessage& message) {
    // The actual sample rate is the requested rate * the oversample rate.
    // See oversample.hpp for more details on oversampling.
    baseband_fs = message.sample_rate * toUType(message.oversample_rate);
    baseband_thread.set_sampling_rate(baseband_fs);

    constexpr int decim_0_scale = 0x2000000;
    constexpr int decim_1_scale = 0x20000;

    switch (message.oversample_rate) {
        case OversampleRate::x4:
            // M4 can't handle 2 decimation passes for sample rates needing x4.
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<NoopDecim>();
            break;

        case OversampleRate::x8:
            // M4 can't handle 2 decimation passes for sample rates at or above 600k.
            if (message.sample_rate < 600'00) {
                decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
                decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps, decim_1_scale);
            } else {
                // Using 180k taps to provide better filtering with a single pass.
                decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_180k_wfm_decim_0.taps, decim_0_scale);
                decim_1.set<NoopDecim>();
            }
            break;

        case OversampleRate::x16:
            decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps, decim_1_scale);
            break;

        case OversampleRate::x32:
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps, decim_1_scale);
            break;

        case OversampleRate::x64:
            decim_0.set<FIRC8xR16x24FS4Decim8>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps, decim_1_scale);
            break;

        default:
            chDbgPanic("Unhandled OversampleRate");
            break;
    }

    size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor();
    size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor();

    // TODO: Use the right taps give the decimator?
    // TODO: Why was this using decim_0's output fs?
    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * decim_1_output_fs;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * decim_1_output_fs;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * decim_1_output_fs;

    spectrum_interval_samples = decim_1_output_fs / spectrum_rate_hz;
    spectrum_samples = 0;
}

void CaptureProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config)
        stream = std::make_unique<StreamInput>(message.config);
    else
        stream.reset();
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<CaptureProcessor>()};
    event_dispatcher.run();
    return 0;
}