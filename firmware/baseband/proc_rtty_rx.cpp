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

#include "proc_rtty_rx.hpp"
#include "portapack_shared_memory.hpp"
#include "event_m4.hpp"

void RTTYRxProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // Reset message payload counters for this cycle.
    // We do NOT reset configuration (baud/shift) here, only the data container.
    tx_message.data_len = 0;

    // 1. Decimate 4MHz -> 500kHz
    // decim_0 output: 2048 / 8 = 256 complex samples
    auto decim_0_out = decim_0.execute(buffer, dst_buffer);

    // 2. Decimate 500kHz -> 62.5kHz
    // decim_1 output: 256 / 8 = 32 complex samples
    // We execute frequently with small blocks, so efficiency here is key.
    auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);

    feed_channel_stats(decim_1_out);

    // 3. FM Demodulation & Processing Loop
    // Loop runs ~32 times per execute call.
    for (size_t i = 0; i < decim_1_out.count; i++) {
        const complex16_t sample = decim_1_out.p[i];

        // Quadrature Product Discriminator: (I[n] * Q[n-1] - I[n-1] * Q[n])
        // Efficient approximation of instantaneous frequency
        int32_t fm_val = ((int32_t)sample.real() * sample_prev.imag()) -
                         ((int32_t)sample.imag() * sample_prev.real());

        sample_prev = sample;
        fm_val *= DISCRIM_GAIN;

        // DC Blocker (IIR High Pass)
        // Removes carrier frequency offset / tuning error.
        fm_dc_val += (fm_val - fm_dc_val) >> DC_ALPHA_SHIFT;
        int32_t val_centered = fm_val - fm_dc_val;

        // Low Pass Filter (IIR)
        // Smooths the signal for the slicer.
        fm_val_smoothed += (val_centered - fm_val_smoothed) >> LPF_ALPHA_SHIFT;

        process_demodulated_sample(fm_val_smoothed);
    }

    // 4. Dispatch Message
    // Only send if we actually decoded one or more characters in this pass.
    if (tx_message.data_len > 0) {
        // Re-attach config data to message just in case receiver needs it
        tx_message.baud = baud_rate;
        tx_message.shift = shift_hz;
        shared_memory.application_queue.push(tx_message);
    }
}

void RTTYRxProcessor::process_demodulated_sample(int32_t sample) {
    // Slicer
    // Mark (1) > 0, Space (0) < 0
    // This assumes standard USB demodulation mapping.
    const uint8_t current_bit = (sample > 0) ? 1 : 0;

    // Soft-UART State Machine
    switch (uart_state) {
        case WAIT_START:
            // Idle is Mark (1). Start bit is Space (0).
            if (current_bit == 0) {
                phase_counter = samples_per_bit / 2;  // Align to center of bit
                uart_state = WAIT_MID_START;
            }
            break;

        case WAIT_MID_START:
            if (--phase_counter == 0) {
                if (current_bit == 0) {  // Valid Start Bit confirmed
                    phase_counter = samples_per_bit;
                    bit_counter = 0;
                    shift_reg = 0;
                    uart_state = READ_BITS;
                } else {
                    uart_state = WAIT_START;  // Glitch
                }
            }
            break;

        case READ_BITS:
            if (--phase_counter == 0) {
                // RTTY LSB First
                if (current_bit) {
                    shift_reg |= (1 << bit_counter);
                }
                phase_counter = samples_per_bit;
                bit_counter++;

                if (bit_counter >= 5) {
                    uart_state = WAIT_STOP;
                }
            }
            break;

        case WAIT_STOP:
            if (--phase_counter == 0) {
                // We expect Stop bit (Mark/1), but we take the char regardless.
                append_data(shift_reg & 0x1F);
                uart_state = WAIT_START;
            }
            break;
    }
}

void RTTYRxProcessor::append_data(uint8_t raw_baudot_code) {
    if (tx_message.data_len < tx_message.max_len) {
        tx_message.data[tx_message.data_len] = raw_baudot_code;
        tx_message.data_len++;
    }
}

void RTTYRxProcessor::on_message(const Message* const message) {
    // Handling "Simplified Config": Configuration comes via RTTYData message ID
    if (message->id == Message::ID::RTTYData) {
        const auto& rtty_msg = static_cast<const RTTYDataMessage&>(*message);

        bool reconfigure_timing = false;

        if (rtty_msg.baud != baud_rate) {
            baud_rate = rtty_msg.baud;
            reconfigure_timing = true;
        }
        shift_hz = rtty_msg.shift;

        if (!configured) {
            // First time setup
            decim_0.configure(taps_4k25_decim_0.taps);
            decim_1.configure(taps_4k25_decim_1.taps);
            configured = true;
            reconfigure_timing = true;
        }

        if (reconfigure_timing && baud_rate > 0) {
            // baud_rate is stored as (Baud * 100). E.g. 4545 -> 45.45 Baud.
            const float real_baud = (float)baud_rate / 100.0f;
            samples_per_bit = (uint32_t)((float)decim_1_out_fs / real_baud);

            // Reset UART state to ensure clean sync on rate change
            uart_state = WAIT_START;
        }
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<RTTYRxProcessor>()};
    event_dispatcher.run();
    return 0;
}