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

    spectrum_samples += out_buffer.count;
    if (spectrum_samples >= spectrum_interval_samples) {
        spectrum_samples -= spectrum_interval_samples;
        channel_spectrum.feed(out_buffer, channel_filter_low_f,
                              channel_filter_high_f, channel_filter_transition);
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
    const auto sample_rate = message.sample_rate;

    // The actual sample rate is the requested rate * the oversample rate.
    // See oversample.hpp for more details on oversampling.
    baseband_fs = sample_rate * toUType(message.oversample_rate);
    baseband_thread.set_sampling_rate(baseband_fs);

    // TODO: Do we need to use the taps that the decimators get configured with?
    channel_filter_low_f = taps_200k_decim_1.low_frequency_normalized * sample_rate;
    channel_filter_high_f = taps_200k_decim_1.high_frequency_normalized * sample_rate;
    channel_filter_transition = taps_200k_decim_1.transition_normalized * sample_rate;

    // Compute the scalar that corrects the oversample_rate to be x8 when computing
    // the spectrum update interval. The original implementation only supported x8.
    // TODO: Why is this needed here but not in proc_replay? There must be some other
    // assumption about x8 oversampling in some component that makes this necessary.
    const auto oversample_correction = toUType(message.oversample_rate) / 8.0;

    // The spectrum update interval controls how often the waterfall is fed new samples.
    spectrum_interval_samples = sample_rate / (spectrum_rate_hz * oversample_correction);
    spectrum_samples = 0;

    // For high sample rates, the M4 is busy collecting samples so the
    // waterfall runs slower. Reduce the update interval so it runs faster.
    // NB: Trade off: looks nicer, but more frequent updates == more CPU.
    if (sample_rate >= 1'500'000)
        spectrum_interval_samples /= (sample_rate / 500'000);

    // Mystery scalars for decimator configuration.
    // TODO: figure these out and add a real comment.
    constexpr int decim_0_scale = 0x2000000;
    constexpr int decim_1_scale = 0x20000;

    switch (message.oversample_rate) {
        case OversampleRate::x4:
            // M4 can't handle 2 decimation passes for sample rates needing x4.
            decim_0.set<FIRC8xR16x24FS4Decim4>().configure(taps_200k_decim_0.taps, decim_0_scale);
            decim_1.set<NoopDecim>();
            break;

        case OversampleRate::x8:
            // M4 can't handle 2 decimation passes for sample rates <= 600k.
            if (message.sample_rate < 600'000) {
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