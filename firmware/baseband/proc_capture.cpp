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
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"
#include "event_m4.hpp"
#include "utility.hpp"

#include <algorithm>

using namespace dsp::decimate;

CaptureProcessor::CaptureProcessor() {
    channel_spectrum.set_decimation_factor(1);
    baseband_thread.start();
}

void CaptureProcessor::execute(const buffer_c8_t& buffer) {
    const auto direction = requested_fs4_direction_.load(std::memory_order_relaxed);
    if (direction != applied_fs4_direction_) {
        decim_0.configure_fs4(decim_0_taps_, direction);
        applied_fs4_direction_ = direction;
        reset_spectrum_capture();
    }

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
        // Bound the residual when one callback exceeds the update interval.
        spectrum_samples = std::min(spectrum_samples - spectrum_interval_samples, spectrum_interval_samples);
        spectrum_capture_credit = std::min<size_t>(256, spectrum_capture_credit + std::min<size_t>(out_buffer.count, 256));
    }
    // Preserve the old frame cadence, but collect each frame contiguously.
    if (!spectrum_capture_remaining && spectrum_capture_credit == 256) {
        spectrum_capture_credit = 0;
        spectrum_capture_remaining = 256;
    }
    feed_spectrum(out_buffer);
}

void CaptureProcessor::reset_spectrum_capture() {
    // Message handling can be preempted by the baseband processing thread.
    chSysLock();
    channel_spectrum.discard_partial();
    spectrum_capture_remaining = 0;
    spectrum_capture_credit = 0;
    spectrum_samples = 0;
    chSysUnlock();
}

void CaptureProcessor::feed_spectrum(const buffer_c16_t& buffer) {
    if (!spectrum_capture_remaining || !buffer.count) return;
    const size_t take = std::min<size_t>(buffer.count, spectrum_capture_remaining);
    // No surplus samples enter a second frame, even with 512-sample buffers.
    if (channel_spectrum.feed({buffer.p, take, buffer.sampling_rate},
                              channel_filter_low_f, channel_filter_high_f, channel_filter_transition)) {
        spectrum_capture_remaining = 0;
    } else {
        spectrum_capture_remaining -= take;
    }
}

void CaptureProcessor::on_signal_message(const RequestSignalMessage& message) {
    if (message.signal == RequestSignalMessage::Signal::BeepStopRequest) {
        audio::dma::beep_stop();
    }
}

void CaptureProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

void CaptureProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::RxFs4Config:
            requested_fs4_direction_.store(
                static_cast<const RxFs4ConfigMessage*>(message)->direction,
                std::memory_order_relaxed);
            break;

        case Message::ID::UpdateSpectrum:
            channel_spectrum.on_message(message);
            break;

        case Message::ID::SpectrumStreamingConfig:
            reset_spectrum_capture();
            channel_spectrum.on_message(message);
            break;

        case Message::ID::SampleRateConfig:
            sample_rate_config(*reinterpret_cast<const SampleRateConfigMessage*>(message));
            break;

        case Message::ID::CaptureConfig:
            capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
            break;

        case Message::ID::RequestSignal:
            on_signal_message(*reinterpret_cast<const RequestSignalMessage*>(message));
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(message));
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

    // For high sample rates, the M4 is busy collecting samples so the
    // waterfall runs slower. Reduce the update interval so it runs faster.
    // NB: Trade off: looks nicer, but more frequent updates == more CPU.
    if (sample_rate >= 1'500'000)
        spectrum_interval_samples /= (sample_rate / 750'000);

    switch (message.oversample_rate) {
        case OversampleRate::x4:
            // M4 can't handle 2 decimation passes for sample rates needing x4.
            configure_fs4<FIRC8xR16x24FS4Decim4>(taps_200k_decim_0.taps);
            decim_1.set<NoopDecim>();
            break;

        case OversampleRate::x8:
            // M4 can't handle 2 decimation passes for sample rates <= 600k.
            if (message.sample_rate < 600'000) {
                configure_fs4<FIRC8xR16x24FS4Decim4>(taps_200k_decim_0.taps);
                decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);
            } else {
                // Using 180k taps to provide better filtering with a single pass.
                configure_fs4<FIRC8xR16x24FS4Decim8>(taps_180k_wfm_decim_0.taps);
                decim_1.set<NoopDecim>();
            }
            break;

        case OversampleRate::x16:
            configure_fs4<FIRC8xR16x24FS4Decim8>(taps_200k_decim_0.taps);
            decim_1.set<FIRC16xR16x16Decim2>().configure(taps_200k_decim_1.taps);
            break;

        case OversampleRate::x32:
            configure_fs4<FIRC8xR16x24FS4Decim4>(taps_200k_decim_0.taps);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps);
            break;

        case OversampleRate::x64:
            configure_fs4<FIRC8xR16x24FS4Decim8>(taps_200k_decim_0.taps);
            decim_1.set<FIRC16xR16x32Decim8>().configure(taps_16k0_decim_1.taps);
            break;

        default:
            chDbgPanic("Unhandled OversampleRate");
            break;
    }
    reset_spectrum_capture();
}

void CaptureProcessor::capture_config(const CaptureConfigMessage& message) {
    if (message.config)
        stream = std::make_unique<StreamInput>(message.config);
    else
        stream.reset();
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<CaptureProcessor>()};
    event_dispatcher.run();
    return 0;
}
