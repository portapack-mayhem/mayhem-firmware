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

#ifndef __PROC_EPIRB_H__
#define __PROC_EPIRB_H__

#include <cstdint>
#include <cstddef>
#include <array>
#include <complex>

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "channel_decimator.hpp"
#include "matched_filter.hpp"
#include "clock_recovery.hpp"
#include "symbol_coding.hpp"
#include "packet_builder.hpp"
#include "baseband_packet.hpp"
#include "message.hpp"
#include "buffer.hpp"

// Forward declarations for types only used as pointers/references
class Message;
namespace baseband {
class Packet;
}

// EPIRB 406 MHz Emergency Position Indicating Radio Beacon
// Signal characteristics:
// - Frequency: 406.025 - 406.028 MHz (typically 406.028 MHz)
// - Modulation: BPSK (Binary Phase Shift Keying)
// - Data rate: 400 bps
// - Encoding: Bi-phase L (Manchester)
// - Transmission: Every 50 seconds ± 2.5 seconds
// - Power: 5W ± 2dB
// - Message length: 144 bits (including sync pattern)

// Matched filter for BPSK demodulation at 400 bps
// Using raised cosine filter taps optimized for 400 bps BPSK
static constexpr std::array<std::complex<float>, 64> bpsk_taps = {{// Raised cosine filter coefficients for BPSK 400 bps
                                                                   -5, -8, -12, -15, -17, -17, -15, -11,
                                                                   -5, 2, 11, 20, 29, 37, 43, 47,
                                                                   48, 46, 42, 35, 26, 16, 4, -8,
                                                                   -21, -33, -44, -53, -59, -62, -62, -58,
                                                                   -51, -41, -28, -13, 3, 19, 36, 51,
                                                                   64, 74, 80, 82, 80, 74, 64, 51,
                                                                   36, 19, 3, -13, -28, -41, -51, -58,
                                                                   -62, -62, -59, -53, -44, -33, -21, -8}};

class EPIRBProcessor : public BasebandProcessor {
   public:
    EPIRBProcessor();

    void execute(const buffer_c8_t& buffer) override;

    void on_message(const Message* const message) override;

   private:
    // EPIRB operates at 406 MHz with narrow bandwidth
    static constexpr size_t baseband_fs = 2457600;
    static constexpr uint32_t epirb_center_freq = 406028000;  // 406.028 MHz
    static constexpr uint32_t symbol_rate = 400;              // 400 bps
    static constexpr size_t decimation_factor = 64;           // Decimate to ~38.4kHz

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    // Decimation chain for 406 MHz EPIRB signal processing
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    dsp::matched_filter::MatchedFilter mf{bpsk_taps, 2};

    // Clock recovery for 400 bps symbol rate
    // Sampling rate after decimation: ~38.4kHz
    // Symbols per sample: 38400 / 400 = 96 samples per symbol
    clock_recovery::ClockRecovery<clock_recovery::FixedErrorFilter> clock_recovery{
        38400,      // sampling_rate
        400,        // symbol_rate (400 bps)
        {0.0555f},  // error_filter coefficient
        [this](const float symbol) { this->consume_symbol(symbol); }};

    // Simple bi-phase L decoder state
    uint_fast8_t last_symbol = 0;

    // EPIRB packet structure:
    // - Sync pattern: 000101010101... (15 bits)
    // - Frame sync: 0111110 (7 bits)
    // - Data: 112 bits
    // - BCH error correction: 10 bits
    // Total: 144 bits
    PacketBuilder<BitPattern, BitPattern, BitPattern> packet_builder{
        {0b010101010101010, 15, 1},  // Preamble pattern
        {0b0111110, 7},              // Frame sync pattern
        {0b0111110, 7},              // End pattern (same as sync for simplicity)
        [this](const baseband::Packet& packet) {
            this->payload_handler(packet);
        }};

    void consume_symbol(const float symbol);
    void payload_handler(const baseband::Packet& packet);

    // Statistics
    uint32_t packets_received = 0;
    Timestamp last_packet_timestamp{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        baseband_fs, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_EPIRB_H__*/