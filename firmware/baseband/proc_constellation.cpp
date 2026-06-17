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
    phase_acc = 0;
    prev_m_phase = 0;
    have_prev = false;
}

void ConstellationProcessor::process_sample(int32_t i_in, int32_t q_in) {
    // Advance the NCO. The full de-rotation angle is the running frequency
    // accumulator plus the static phase correction from the PLL.
    nco_phase += static_cast<uint32_t>(nco_freq);
    const uint32_t angle = nco_phase + phase_acc;

    // sin/cos from the shared int8 table (top 8 bits index one of 256 steps).
    // cos(x) == sin(x + quarter turn); a quarter of 256 entries is 64.
    const uint8_t idx = static_cast<uint8_t>(angle >> 24);
    const int32_t s = sine_table_i8[idx];
    const int32_t c = sine_table_i8[static_cast<uint8_t>(idx + 64)];

    // De-rotate by -angle: (I + jQ) * (cos - j sin). Table is ~Q7, so >>7.
    const int32_t di = (i_in * c + q_in * s) >> 7;
    const int32_t dq = (q_in * c - i_in * s) >> 7;

    // Magnitude gate. With no symbol-timing recovery most samples sit on
    // inter-symbol transitions (low amplitude); skipping them de-smears the
    // display and keeps the loops from locking onto transition noise. env2 is
    // a slow running mean of |z|^2; keep samples in the upper ~3/4.
    const int32_t mag2 = di * di + dq * dq;
    env2 += (mag2 - env2) >> 4;
    if (mag2 < (env2 - (env2 >> 2))) {
        return;
    }

    if (correct_frequency || correct_phase) {
        // Strip the M-fold modulation: read the phase (fast fixed-point atan2,
        // one turn == 65536) and multiply by the rotational symmetry order. The
        // symbol rotation wraps away, leaving M * carrier-phase-error.
        const uint16_t a = static_cast<uint16_t>(
            fxpt_atan2(static_cast<int16_t>(dq), static_cast<int16_t>(di)));
        const uint16_t m_phase = static_cast<uint16_t>(a * order);

        if (correct_phase) {
            // Residual M*phase as a signed deviation ~ M * static phase error.
            const int16_t err_p = static_cast<int16_t>(m_phase);
            phase_acc += static_cast<int32_t>(pll_gain * static_cast<float>(err_p));
        }

        if (correct_frequency && have_prev) {
            // Change in M*phase between samples ~ M * frequency error.
            // Clamp via int64 so a transient never overflows the int32 word.
            const int16_t err_f = static_cast<int16_t>(m_phase - prev_m_phase);
            const int64_t updated =
                static_cast<int64_t>(nco_freq) +
                static_cast<int64_t>(fll_gain * static_cast<float>(err_f));
            nco_freq = static_cast<int32_t>(
                std::clamp<int64_t>(updated, -(1LL << 30), (1LL << 30)));
        }

        prev_m_phase = m_phase;
        have_prev = true;
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

            // Loop-bandwidth presets. Gains are in accumulator units per unit
            // int16 phase error and are divided by the order so the behaviour is
            // consistent across M. Tune live from the UI.
            static constexpr float pll_presets[3] = {200.0f, 800.0f, 3000.0f};
            static constexpr float fll_presets[3] = {20.0f, 80.0f, 320.0f};
            const size_t bw = std::min<size_t>(message.loop_bw, 2);
            pll_gain = pll_presets[bw] / static_cast<float>(order);
            fll_gain = fll_presets[bw] / static_cast<float>(order);

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
