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

#include "proc_wideband_spectrum.hpp"
#include "audio_dma.hpp"

#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

void WidebandSpectrum::execute(const buffer_c8_t& buffer) {
    // 2048 complex8_t samples per buffer.
    // 102.4us per buffer. 20480 instruction cycles per buffer.

    if (!configured) return;

    if (output_mode == WidebandSpectrumConfigMessage::OutputMode::TimeDomain) {
        execute_time_domain(buffer);
    } else {
        execute_frequency_domain(buffer);
    }
}

void WidebandSpectrum::execute_frequency_domain(const buffer_c8_t& buffer) {
    if (phase == 0) {
        std::fill(spectrum.begin(), spectrum.end(), 0);
    }

    for (size_t i = 0; i < spectrum.size(); i++) {
        // TODO: Removed window-presum windowing, due to lack of available code RAM.
        // TODO: Apply window to improve spectrum bin sidelobes.
        spectrum[i] += buffer.p[i + 0];
        spectrum[i] += buffer.p[i + 1024];
    }

    if (phase == trigger) {
        const buffer_c16_t buffer_c16{
            spectrum.data(),
            spectrum.size(),
            buffer.sampling_rate};
        channel_spectrum.feed(
            buffer_c16,
            0, 0, 0);
        phase = 0;
    } else {
        phase++;
    }
}

void WidebandSpectrum::execute_time_domain(const buffer_c8_t& buffer) {
    if (!time_streaming || buffer.count == 0) {
        return;
    }

    // In time mode, keep samples from a single snapshot instead of averaging
    // across multiple buffers so the trace reflects instantaneous waveform shape.
    if (phase < trigger) {
        phase++;
        return;
    }

    phase = 0;

    if (time_domain_request_update) {
        return;
    }

    const size_t stride = std::max<size_t>(1, buffer.count / time_domain_spectrum.db.size());
    time_domain_spectrum.sampling_rate = buffer.sampling_rate / stride;
    time_domain_spectrum.channel_filter_low_frequency = 0;
    time_domain_spectrum.channel_filter_high_frequency = 0;
    time_domain_spectrum.channel_filter_transition = 0;

    for (size_t i = 0; i < time_domain_spectrum.db.size(); i++) {
        const size_t sample_index = std::min(i * stride, buffer.count - 1);
        const int32_t normalized = std::clamp<int32_t>(
            static_cast<int32_t>(buffer.p[sample_index].real()) + 128,
            0,
            255);
        time_domain_spectrum.db[i] = static_cast<uint8_t>(normalized);
    }

    time_domain_request_update = true;
    EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
}

void WidebandSpectrum::on_signal_message(const RequestSignalMessage& message) {
    if (message.signal == RequestSignalMessage::Signal::BeepStopRequest) {
        audio::dma::beep_stop();
    }
}

void WidebandSpectrum::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

void WidebandSpectrum::set_time_streaming_state(const SpectrumStreamingConfigMessage& message) {
    if (message.mode == SpectrumStreamingConfigMessage::Mode::Running) {
        time_streaming = true;
        ChannelSpectrumConfigMessage fifo_message{&fifo};
        shared_memory.application_queue.push(fifo_message);
    } else {
        time_streaming = false;
        time_domain_request_update = false;
        fifo.reset_in();
    }
}

void WidebandSpectrum::update_time_domain() {
    if (time_streaming && time_domain_request_update) {
        fifo.in(time_domain_spectrum);
    }

    time_domain_request_update = false;
}

void WidebandSpectrum::apply_streaming_state() {
    const SpectrumStreamingConfigMessage message{
        spectrum_streaming
            ? SpectrumStreamingConfigMessage::Mode::Running
            : SpectrumStreamingConfigMessage::Mode::Stopped};

    if (output_mode == WidebandSpectrumConfigMessage::OutputMode::TimeDomain) {
        // Ensure only one producer is active while in time-domain mode.
        const SpectrumStreamingConfigMessage stop_message{
            SpectrumStreamingConfigMessage::Mode::Stopped};
        channel_spectrum.on_message(&stop_message);
        set_time_streaming_state(message);
    } else {
        channel_spectrum.on_message(&message);
        time_streaming = false;
        time_domain_request_update = false;
        fifo.reset_in();
    }
}

void WidebandSpectrum::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::RequestSignal:
            on_signal_message(*reinterpret_cast<const RequestSignalMessage*>(msg));
            return;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(msg));
            return;

        default:
            break;
    }

    switch (msg->id) {
        case Message::ID::UpdateSpectrum:
            if (output_mode == WidebandSpectrumConfigMessage::OutputMode::TimeDomain) {
                update_time_domain();
            } else {
                channel_spectrum.on_message(msg);
            }
            break;

        case Message::ID::SpectrumStreamingConfig:
            spectrum_streaming = reinterpret_cast<const SpectrumStreamingConfigMessage*>(msg)->mode ==
                                 SpectrumStreamingConfigMessage::Mode::Running;
            apply_streaming_state();
            break;

        case Message::ID::WidebandSpectrumConfig: {
            const auto& message = *reinterpret_cast<const WidebandSpectrumConfigMessage*>(msg);
            baseband_fs = message.sampling_rate;
            trigger = message.trigger;
            output_mode = message.output_mode;
            baseband_thread.set_sampling_rate(baseband_fs);
            phase = 0;
            time_domain_request_update = false;
            configured = true;
            break;
        }

        default:
            break;
    }

    if (msg->id == Message::ID::WidebandSpectrumConfig) {
        apply_streaming_state();
    }
}

int main() {
    audio::dma::init_audio_out();  // for AudioRX app (enables audio output while this baseband image is running)
    EventDispatcher event_dispatcher{std::make_unique<WidebandSpectrum>()};
    event_dispatcher.run();
    return 0;
}
