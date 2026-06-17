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

#include "proc_constellation.hpp"

#include "event_m4.hpp"
#include "fxpt_atan2.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

void ConstellationProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        return;
    }

    execute_constellation(buffer);
}

void ConstellationProcessor::reset_loops() {
    decim_counter = 0;
    point_index = 0;
    env2 = 0;
    nco_phase = 0;
    nco_freq = 0;
}

void ConstellationProcessor::process_sample(int32_t i_in, int32_t q_in) {
    // De-rotate the current sample by the NCO phase.
    // sin/cos from the shared int8 table (top 8 bits index one of 256 steps).
    // cos(x) == sin(x + quarter turn); a quarter of 256 entries is 64.
    const uint8_t idx = static_cast<uint8_t>(nco_phase >> 24);
    const int32_t s = sine_table_i8[idx];
    const int32_t c = sine_table_i8[static_cast<uint8_t>(idx + 64)];

    // De-rotate by -phase: (I + jQ) * (cos - j sin). Table is ~Q7, so >>7.
    const int32_t di = (i_in * c + q_in * s) >> 7;
    const int32_t dq = (q_in * c - i_in * s) >> 7;

    // Magnitude gate. With no symbol-timing recovery most samples sit on
    // inter-symbol transitions (low amplitude); skipping them de-smears the
    // display and keeps the loop from locking onto transition noise. env2 is
    // a slow running mean of |z|^2; keep samples in the upper ~3/4.
    const int32_t mag2 = di * di + dq * dq;
    env2 += (mag2 - env2) >> 4;
    if (mag2 < (env2 - (env2 >> 2))) {
        return;
    }

    if (correct_frequency || correct_phase) {
        // M-fold phase detector: read the phase (fast fixed-point atan2, one
        // turn == 65536) and multiply by the rotational symmetry order so the
        // symbol rotation wraps away. The signed residual is the phase error.
        const uint16_t a = static_cast<uint16_t>(
            fxpt_atan2(static_cast<int16_t>(dq), static_cast<int16_t>(di)));
        const int16_t e_p = static_cast<int16_t>(static_cast<uint16_t>(a * order));
        const float ferr = static_cast<float>(e_p);

        // 2nd-order PLL loop filter into a single NCO.
        int32_t increment = 0;

        if (correct_frequency) {
            // Integral path -> NCO frequency (removes the frequency offset).
            // Clamp via int64 so a transient never overflows the int32 word.
            const int64_t updated =
                static_cast<int64_t>(nco_freq) + static_cast<int64_t>(ki_gain * ferr);
            nco_freq = static_cast<int32_t>(
                std::clamp<int64_t>(updated, -(1LL << 30), (1LL << 30)));
            increment += nco_freq;
        } else {
            nco_freq = 0;  // don't hold a stale frequency estimate when disabled
        }

        if (correct_phase) {
            // Proportional path -> phase (removes the static phase offset).
            // Recomputed each sample, never accumulated, so the loop is stable.
            increment += static_cast<int32_t>(kp_gain * ferr);
        }

        nco_phase += static_cast<uint32_t>(increment);
    }

    // Stash the corrected point (offset-binary int8) for the M0 side.
    const int32_t oi = std::clamp<int32_t>(di, -128, 127) + 128;
    const int32_t oq = std::clamp<int32_t>(dq, -128, 127) + 128;
    points.db[point_index * 2] = static_cast<uint8_t>(oi);
    points.db[point_index * 2 + 1] = static_cast<uint8_t>(oq);
    point_index++;

    if (point_index >= points_per_frame) {
        point_index = 0;
        if (!request_update) {
            request_update = true;
            EventDispatcher::events_flag(EVT_MASK_SPECTRUM);
        }
    }
}

void ConstellationProcessor::execute_constellation(const buffer_c8_t& buffer) {
    if (!streaming || buffer.count == 0) {
        return;
    }

    const size_t step = std::max<size_t>(1, decimation);

    for (size_t n = 0; n < buffer.count; n++) {
        if (decim_counter < step - 1) {
            decim_counter++;
            continue;
        }
        decim_counter = 0;

        process_sample(buffer.p[n].real(), buffer.p[n].imag());
    }
}

void ConstellationProcessor::set_streaming_state(const SpectrumStreamingConfigMessage& message) {
    if (message.mode == SpectrumStreamingConfigMessage::Mode::Running) {
        streaming = true;
        reset_loops();
        ChannelSpectrumConfigMessage fifo_message{&fifo};
        shared_memory.application_queue.push(fifo_message);
    } else {
        streaming = false;
        request_update = false;
        fifo.reset_in();
    }
}

void ConstellationProcessor::update_fifo() {
    if (streaming && request_update) {
        fifo.in(points);
    }

    request_update = false;
}

void ConstellationProcessor::on_message(const Message* const msg) {
    switch (msg->id) {
        case Message::ID::UpdateSpectrum:
            update_fifo();
            break;

        case Message::ID::SpectrumStreamingConfig:
            set_streaming_state(*reinterpret_cast<const SpectrumStreamingConfigMessage*>(msg));
            break;

        case Message::ID::ConstellationConfig: {
            const auto& message = *reinterpret_cast<const ConstellationConfigMessage*>(msg);
            baseband_fs = message.sampling_rate;
            decimation = std::max<size_t>(1, message.decimation);
            order = std::max<size_t>(1, message.order);
            correct_frequency = message.correct_frequency;
            correct_phase = message.correct_phase;

            // Loop-bandwidth presets (proportional kp, integral ki) in
            // accumulator units per unit int16 phase error. ki << kp keeps the
            // loop well damped; both are divided by the order so the behaviour
            // is consistent across M. Tune live from the UI.
            static constexpr float kp_presets[3] = {200.0f, 800.0f, 3000.0f};
            static constexpr float ki_presets[3] = {0.5f, 4.0f, 30.0f};
            const size_t bw = std::min<size_t>(message.loop_bw, 2);
            kp_gain = kp_presets[bw] / static_cast<float>(order);
            ki_gain = ki_presets[bw] / static_cast<float>(order);

            baseband_thread.set_sampling_rate(baseband_fs);
            reset_loops();
            configured = true;
            break;
        }

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<ConstellationProcessor>()};
    event_dispatcher.run();
    return 0;
}
