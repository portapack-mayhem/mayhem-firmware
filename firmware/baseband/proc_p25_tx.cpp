/*
 * Copyright (C) 2024 PortaPack-MAYHEM Contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "proc_p25_tx.hpp"
#include "portapack_shared_memory.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

#include <cstdint>

// Gray-coded dibit → FM deviation level:
// dibit 0 → +1 unit (+600 Hz)
// dibit 1 → +3 units (+1800 Hz)
// dibit 2 → -1 unit  (-600 Hz)
// dibit 3 → -3 units (-1800 Hz)
static constexpr int8_t dibit_to_level[4] = {1, 3, -1, -3};

void P25TxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        // Zero output - never emit garbage while idle
        for (size_t i = 0; i < buffer.count; i++)
            buffer.p[i] = {0, 0};
        return;
    }

    for (size_t i = 0; i < buffer.count; i++) {
        if (sample_count == 0) {
            if (dibit_index >= frame_length) {
                // Frame complete - notify A7 and stop
                txprogress_message.done = true;
                shared_memory.application_queue.push(txprogress_message);
                configured = false;
                for (size_t j = i; j < buffer.count; j++)
                    buffer.p[j] = {0, 0};
                return;
            }
        }

        // Current dibit → frequency level
        uint8_t dibit = shared_memory.bb_data.data[dibit_index] & 0x03;
        int8_t level = dibit_to_level[dibit];

        // Accumulate FM phase at ±level × base_phase_step per sample
        phase += (uint32_t)((int32_t)level * (int32_t)base_phase_step);

        int8_t re = sine_table_i8[(phase + 0x40000000) >> 24];  // cos
        int8_t im = sine_table_i8[phase >> 24];                 // sin
        buffer.p[i] = {re, im};

        sample_count++;
        if (sample_count >= P25_SAMPLES_PER_DIBIT) {
            sample_count = 0;
            dibit_index++;
        }
    }
}

void P25TxProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::P25TxConfigure) {
        const auto& m = *reinterpret_cast<const P25TxConfigureMessage*>(msg);
        if (m.frame_length == 0) {
            configured = false;
            return;
        }
        frame_length = m.frame_length;
        dibit_index = 0;
        sample_count = 0;
        txprogress_message.done = false;
        configured = true;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<P25TxProcessor>()};
    event_dispatcher.run();
    return 0;
}
