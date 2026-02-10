/*
 * Copyright (C) 2026 HTotoo
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include "proc_rtty_tx.hpp"
#include "sine_table_int8.hpp"
#include "event_m4.hpp"
#include <algorithm>

// Coefficient for 3.072 MHz
// 2^32 / 3072000 = 1398.101
static constexpr uint32_t PHASE_DELTA_COEFF = 1398;

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

        // 3. LINEAR SLEW RATE LIMITING
        int32_t diff = (int32_t)target_delta - (int32_t)current_delta;

        if (abs(diff) <= (int32_t)slew_rate) {
            current_delta = target_delta;
        } else {
            if (diff > 0)
                current_delta += slew_rate;
            else
                current_delta -= slew_rate;
        }

        // 4. GENERATION
        phase += current_delta;

        int8_t re = sine_table_i8[((phase + 0x40000000) & 0xFF000000) >> 24];
        int8_t im = sine_table_i8[(phase & 0xFF000000) >> 24];

        buffer.p[i] = {re, im};
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
                baud_phase_increment = base_baud_phase_increment;
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
                if (configured_stop_bits > 0) {
                    baud_phase_increment = (base_baud_phase_increment * 2) / configured_stop_bits;
                }
            }
            break;

        case State::StopBit:
            baud_phase_increment = base_baud_phase_increment;
            if (buffer_pop(current_char)) {
                state = State::StartBit;
            } else {
                state = State::LeadOut;
                lead_counter = 0;
            }
            break;

        case State::LeadOut:
            break;
    }
}

void RTTYTXProcessor::configure(uint16_t baud, uint16_t shift, int16_t mark_tone_, int16_t space_tone_, uint8_t stop_bits_, bool inverted_) {
    if (baud == 0) return;
    // Baud Rate
    uint64_t calc = (uint64_t)baud * 13981;
    uint32_t new_base_baud_inc = (uint32_t)(calc / 1000);

    // Stop Bits
    configured_stop_bits = stop_bits_;
    if (configured_stop_bits < 2) configured_stop_bits = 2;

    // Frequencies
    int32_t freq_mark = mark_tone_;
    int32_t freq_space = space_tone_;

    if (inverted_) {
        std::swap(freq_mark, freq_space);
    }

    // Convert Frequencies to Phase Deltas
    uint32_t new_delta_mark = (uint32_t)(freq_mark * (int32_t)PHASE_DELTA_COEFF);
    uint32_t new_delta_space = (uint32_t)(freq_space * (int32_t)PHASE_DELTA_COEFF);

    // Slew Rate Calculation
    // We aim for the transition to take about 1/15th of a bit width
    uint32_t samples_per_bit = (baseband_fs * 100) / baud;
    uint32_t transition_samples = samples_per_bit / 15;
    if (transition_samples == 0) transition_samples = 1;

    uint32_t shift_delta = (uint32_t)((uint32_t)shift * PHASE_DELTA_COEFF);
    uint32_t new_slew_rate = shift_delta / transition_samples;
    if (new_slew_rate == 0) new_slew_rate = 1;

    base_baud_phase_increment = new_base_baud_inc;
    delta_mark = new_delta_mark;
    delta_space = new_delta_space;
    slew_rate = new_slew_rate;

    if (!configured) {
        current_delta = delta_mark;
        lead_counter = 0;
        phase = 0;
        baud_phase = 0;
        baud_phase_increment = base_baud_phase_increment;
        configured = true;
    }
}

void RTTYTXProcessor::on_message(const Message* const msg) {
    if (msg->id == Message::ID::RTTYData) {
        const auto& rtty_msg = *reinterpret_cast<const RTTYDataMessage*>(msg);
        configure(rtty_msg.baud,
                  rtty_msg.shift,
                  rtty_msg.mark_tone,
                  rtty_msg.space_tone,
                  rtty_msg.stopbits,
                  rtty_msg.inverted);

        buffer_push(0x1F);  // LTRS
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

// Ring Buffer Logic
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