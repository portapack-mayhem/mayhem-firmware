/*
 * Copyright (C) 2026 Matej Sochan
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

#include "proc_signal_hunter.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"
#include "dsp_fir_taps.hpp"

void SignalHunterProcessor::configure() {
    decim_0.configure(taps_4k25_decim_0.taps);

    window_idx = 0;
    window_sum = 0;
    for (auto& v : window_buf) v = 0;

    reset_hunt_state();
    configured = true;
}

void SignalHunterProcessor::reset_hunt_state() {
    iq_ring_idx = 0;
    for (auto& v : iq_ring) v = complex16_t{0, 0};
    hangtime_counter = 0;
    hunt_state = HuntState::IDLE;
    flush_pending = false;
}

void SignalHunterProcessor::execute(const buffer_c8_t& buffer) {
    if (!hunting) return;

    const auto out = decim_0.execute(buffer, dst_buffer);
    feed_channel_stats(out);

    // Pre-roll flush: first execute() call after CaptureConfigMessage creates stream.
    if (flush_pending && stream) {
        // Write ring buffer from oldest to newest sample (2 chunks due to wrap-around)
        size_t first = IQ_RING_SAMPLES - flush_start_idx;
        stream->write(&iq_ring[flush_start_idx], first * sizeof(complex16_t));
        if (flush_start_idx > 0)
            stream->write(&iq_ring[0], flush_start_idx * sizeof(complex16_t));
        flush_pending = false;
    }

    for (size_t i = 0; i < out.count; i++) {
        auto s = out.p[i];

        iq_ring[iq_ring_idx] = s;
        iq_ring_idx = (iq_ring_idx + 1) % IQ_RING_SAMPLES;

        uint32_t energy = ((int32_t)s.real() * s.real() +
                           (int32_t)s.imag() * s.imag()) >> 16;
        window_sum -= window_buf[window_idx];
        window_buf[window_idx] = energy;
        window_sum += energy;
        window_idx = (window_idx + 1) % WINDOW_SIZE;

        uint32_t avg = window_sum / WINDOW_SIZE;

        switch (hunt_state) {
            case HuntState::IDLE:
                if (avg > energy_threshold) {
                    HunterTriggerMessage msg{};
                    msg.energy = avg;
                    shared_memory.application_queue.push(msg);
                    hunt_state = HuntState::AWAITING_STREAM;
                }
                break;
            case HuntState::AWAITING_STREAM:
                break;
            case HuntState::RECORDING:
                if (avg < energy_threshold) {
                    hangtime_counter = hangtime_samples_limit;
                    hunt_state = HuntState::HANGTIME;
                }
                break;
            case HuntState::HANGTIME:
                if (avg > energy_threshold) {
                    hunt_state = HuntState::RECORDING;
                } else if (--hangtime_counter == 0) {
                    HunterStopMessage stop_msg{};
                    shared_memory.application_queue.push(stop_msg);
                    hunt_state = HuntState::AWAITING_CLOSE;
                }
                break;
            case HuntState::AWAITING_CLOSE:
                // Do not transition to IDLE by ourselves — wait for CaptureConfigMessage(nullptr)
                // which arrives via BasebandCapture destructor after CaptureThread is destroyed
                break;
        }
    }

    // CRITICAL: Continue writing to stream whenever it exists, even in AWAITING_CLOSE state.
    if (stream) {
        stream->write(out.p, sizeof(complex16_t) * out.count);
    }
}

void SignalHunterProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::HunterConfig: {
            const auto& m = *reinterpret_cast<const HunterConfigMessage*>(message);
            energy_threshold = m.energy_threshold;

            // Convert hangtime to post-decimation samples:
            // 1 ms = 250 samples @ 250 kHz post-decimation rate (from 2 MHz baseband / 8x decimator)
            // This dynamic hangtime allows configurable silence tolerance (e.g., 500 ms)
            hangtime_samples_limit = m.hangtime_ms * 250;

            if (!configured) configure();

            if (!m.start && hunting) {
                stream.reset();
                reset_hunt_state();
            }
            hunting = m.start;
            break;
        }

        case Message::ID::CaptureConfig: {
            const auto& m = *reinterpret_cast<const CaptureConfigMessage*>(message);
            if (m.config) {
                stream = std::make_unique<StreamInput>(m.config);
                flush_start_idx = iq_ring_idx;  // Snapshot: current write position
                flush_pending = true;           // execute() will process ring buffer pre-roll
                hunt_state = HuntState::RECORDING;
            } else {
                stream.reset();
                reset_hunt_state();
            }
            break;
        } 

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SignalHunterProcessor>()};
    event_dispatcher.run();
    return 0;
}
