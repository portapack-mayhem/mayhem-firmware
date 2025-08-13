/*
 * Copyright (C) 2024 EPIRB Receiver Implementation
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

#include "proc_epirb.hpp"

#include "portapack_shared_memory.hpp"

#include "dsp_fir_taps.hpp"

#include "event_m4.hpp"
#include <ch.h>

EPIRBProcessor::EPIRBProcessor() {
    // Configure the decimation filters for narrowband EPIRB signal
    // Target: Reduce 2.457600 MHz to ~38.4 kHz for 400 bps processing
    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    baseband_thread.start();
}

void EPIRBProcessor::execute(const buffer_c8_t& buffer) {
    /* 2.4576MHz, 2048 samples */

    // First decimation stage: 2.4576 MHz -> 307.2 kHz
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);

    // Second decimation stage: 307.2 kHz -> 38.4 kHz
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto decimator_out = decim_1_out;

    /* 38.4kHz, 32 samples (approximately) */
    feed_channel_stats(decimator_out);

    // Process each decimated sample through the matched filter
    for (size_t i = 0; i < decimator_out.count; i++) {
        // Apply matched filter for BPSK demodulation
        if (mf.execute_once(decimator_out.p[i])) {
            // Feed symbol to clock recovery when matched filter triggers
            clock_recovery(mf.get_output());
        }
    }
}

void EPIRBProcessor::consume_symbol(const float raw_symbol) {
    // BPSK demodulation: positive = 1, negative = 0
    const uint_fast8_t sliced_symbol = (raw_symbol >= 0.0f) ? 1 : 0;

    // Decode bi-phase L encoding manually
    // In bi-phase L: 0 = no transition, 1 = transition
    // This is a simple edge detector
    const auto decoded_symbol = sliced_symbol ^ last_symbol;
    last_symbol = sliced_symbol;

    // Build packet from decoded symbols
    packet_builder.execute(decoded_symbol);
}

void EPIRBProcessor::payload_handler(const baseband::Packet& packet) {
    // EPIRB packet received - validate and process
    if (packet.size() >= 112) {  // Minimum EPIRB data payload size (112 bits)
        packets_received++;
        last_packet_timestamp = Timestamp::now();

        // Create and send EPIRB packet message to application layer
        const EPIRBPacketMessage message{packet};
        shared_memory.application_queue.push(message);
    }
}

void EPIRBProcessor::on_message(const Message* const message) {
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<EPIRBProcessor>()};
    event_dispatcher.run();
    return 0;
}