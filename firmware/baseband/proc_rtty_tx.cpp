/*
 * Copyright (C) 2026 HTotoo
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

#include "proc_rtty_tx.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"

// Coefficient for 3.072 MHz
// 1.536M uses 1398.
// 3.072M (double freq) requires HALF the increment to reach same freq.
static constexpr uint32_t PHASE_DELTA_COEFF = 699;

// Lead-in: 1.0 Second (3,072,000 samples)
static constexpr uint32_t LEAD_IN_SAMPLES = 3072000;

void RTTYTXProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) {
        for (size_t i = 0; i < buffer.count; i++) {
            buffer.p[i] = {0, 0};
        }
        return;
    }

    for (size_t i = 0; i < buffer.count; i++) {
        // 1. PRECISION TIMING
        bool advance = false;

        if (state == State::LeadIn) {
            lead_counter++;
            if (lead_counter >= LEAD_IN_SAMPLES) {
                advance = true;
            }
        } else if (state == State::LeadOut) {
            lead_counter++;
            // Hold for ~150ms (460k samples)
            if (lead_counter >= 460000) {
                txprogress_message.done = true;
                shared_memory.application_queue.push(txprogress_message);
                configured = false;
                state = State::Idle;
            }
        } else if (state != State::Idle) {
            uint32_t previous_phase = baud_phase;
            baud_phase += baud_phase_increment;

            if (baud_phase < previous_phase) {
                advance = true;
            }
        }

        if (advance) {
            advance_state();
        }

        // 2. TONE SELECTION
        uint32_t target_delta;
        bool is_mark = true;

        switch (state) {
            case State::StartBit:
                is_mark = false;  // Space
                break;
            case State::DataBits:
                is_mark = (current_char >> bit_pos) & 1;  // LSB First
                break;
            case State::Idle:
            case State::LeadIn:
            case State::LeadOut:
            case State::StopBit:
            default:
                is_mark = true;  // Mark
                break;
        }

        if (is_mark)
            target_delta = delta_mark;
        else
            target_delta = delta_space;

        // 3. LINEAR SLEW RATE LIMITING (Cleaner Spectrum)
        // Instead of exponential curve (RC), we use a constant slope (Trapezoid).
        // This provides the cleanest trade-off between edge sharpness and splatter.

        if (current_delta < target_delta) {
            current_delta += slew_rate;
            if (current_delta > target_delta) current_delta = target_delta;  // Clamp
        } else if (current_delta > target_delta) {
            // Check underflow before subtracting
            if (current_delta >= slew_rate) {
                current_delta -= slew_rate;
                if (current_delta < target_delta) current_delta = target_delta;  // Clamp
            } else {
                current_delta = target_delta;
            }
        }

        // 4. GENERATION
        phase += current_delta;

        int8_t re = sine_table_i8[((phase + 0x40000000) & 0xFF000000) >> 24];
        int8_t im = sine_table_i8[(phase & 0xFF000000) >> 24];

        // Output Half Scale (64) for Linearity
        buffer.p[i] = {static_cast<int8_t>(re >> 1), static_cast<int8_t>(im >> 1)};
    }
}

void RTTYTXProcessor::advance_state() {
    switch (state) {
        case State::Idle:
            state = State::LeadIn;
            lead_counter = 0;
            break;

        case State::LeadIn:
            if (buffer_pop(current_char)) {
                state = State::StartBit;
                baud_phase = 0;
            } else {
                state = State::LeadOut;
                lead_counter = 0;
            }
            break;

        case State::StartBit:
            state = State::DataBits;
            bit_pos = 0;
            break;

        case State::DataBits:
            bit_pos++;
            if (bit_pos >= 5) {
                state = State::StopBit;
                stop_bit_extended = false;
            }
            break;

        case State::StopBit:
            if (!stop_bit_extended) {
                stop_bit_extended = true;
            } else {
                if (buffer_pop(current_char)) {
                    state = State::StartBit;
                } else {
                    state = State::LeadOut;
                    lead_counter = 0;
                }
            }
            break;

        case State::LeadOut:
            break;
    }
}

void RTTYTXProcessor::configure(uint16_t baud, uint16_t shift) {
    if (baud == 0) return;

    // Baud Calculation for 3.072 MHz
    // Increment = (Baud/100) * (2^32 / 3072000)
    // 2^32 / 3072000 = 1398.1
    // Increment ~= (baud/100) * 1398.1 ~= baud * 13.981

    // High precision: baud * 13981 / 1000
    uint64_t calc = (uint64_t)baud * 13981;
    baud_phase_increment = (uint32_t)(calc / 1000);

    // FSK Deltas
    delta_space = 0;  // 0 Hz
    delta_mark = (uint32_t)(shift * PHASE_DELTA_COEFF);

    // Slew Rate Calculation
    // We want the transition to take about 10% of a bit width.
    // Samples per bit = 3072000 / (Baud/100)
    uint32_t samples_per_bit = (baseband_fs * 100) / baud;

    // Transition duration = Samples / 10
    uint32_t transition_samples = samples_per_bit / 10;
    if (transition_samples == 0) transition_samples = 1;

    // Slew = Total_Diff / Steps
    // Diff is just delta_mark (since space is 0)
    slew_rate = delta_mark / transition_samples;
    if (slew_rate == 0) slew_rate = 1;

    // Init
    current_delta = delta_mark;
    lead_counter = 0;
    phase = 0;
    baud_phase = 0;

    configured = true;
}

void RTTYTXProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::RTTYData) {
        const auto& rtty_msg = *reinterpret_cast<const RTTYDataMessage*>(msg);

        configure(rtty_msg.baud, rtty_msg.shift);

        // buffer_push(0x1F);  // LTRS
        buffer_push(0x08);  // CR
        buffer_push(0x02);  // LF

        for (uint16_t i = 0; i < rtty_msg.data_len && i < rtty_msg.max_len; i++) {
            buffer_push(rtty_msg.data[i]);
        }

        if (state == State::Idle) {
            state = State::LeadIn;
            lead_counter = 0;
        }
    }
}

// Ring Buffer

bool RTTYTXProcessor::buffer_push(uint8_t byte) {
    size_t next_head = (head + 1) % data_buffer.size();
    if (next_head == tail) return false;
    data_buffer[head] = byte;
    head = next_head;
    return true;
}

bool RTTYTXProcessor::buffer_pop(uint8_t& byte) {
    if (head == tail) return false;
    byte = data_buffer[tail];
    tail = (tail + 1) % data_buffer.size();
    return true;
}

bool RTTYTXProcessor::buffer_empty() const {
    return head == tail;
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<RTTYTXProcessor>()};
    event_dispatcher.run();
    return 0;
}