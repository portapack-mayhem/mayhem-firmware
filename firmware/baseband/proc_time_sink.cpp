/*
 * Copyleft zxkmm (>) 2026
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

#include "proc_time_sink.hpp"

#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

void TimeSinkProcessor::execute(const buffer_c8_t& buffer) {
    // 2048 complex8_t samples per buffer.
    // 102.4us per buffer. 20480 instruction cycles per buffer.

    if (!configured) {
        return;
    }

    execute_time_domain(buffer);
}

void TimeSinkProcessor::execute_time_domain(const buffer_c8_t& buffer) {
    if (!time_streaming || buffer.count == 0) {
        return;
    }

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

void TimeSinkProcessor::set_time_streaming_state(const SpectrumStreamingConfigMessage& message) {
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

void TimeSinkProcessor::update_time_domain() {
    if (time_streaming && time_domain_request_update) {
        fifo.in(time_domain_spectrum);
    }

    time_domain_request_update = false;
}

void TimeSinkProcessor::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::UpdateSpectrum:
            update_time_domain();
            break;

        case Message::ID::SpectrumStreamingConfig:
            set_time_streaming_state(*reinterpret_cast<const SpectrumStreamingConfigMessage*>(msg));
            break;

        case Message::ID::TimeSinkConfig: {
            const auto& message = *reinterpret_cast<const TimeSinkConfigMessage*>(msg);
            baseband_fs = message.sampling_rate;
            trigger = message.trigger;
            baseband_thread.set_sampling_rate(baseband_fs);
            phase = 0;
            time_domain_request_update = false;
            configured = true;
            break;
        }

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<TimeSinkProcessor>()};
    event_dispatcher.run();
    return 0;
}
