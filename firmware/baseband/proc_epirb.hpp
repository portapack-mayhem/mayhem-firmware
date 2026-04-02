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

#include "audio_output.hpp"
#include "dsp_demodulate.hpp"

// Forward declarations for types only used as pointers/references
class Message;
namespace baseband {
class Packet;
}

// On travaille à 38400 Hz après décimation par 8
/*static constexpr uint32_t BASEBAND_SAMPLE_RATE = 3072000;//2457600;
static constexpr uint32_t SAMPLE_RATE = BASEBAND_SAMPLE_RATE / 8 / 4; //8;                 // = 38400
static constexpr uint32_t SYMBOL_RATE = 800;                             // 400 bps + Manchester (2 1/2 bits per symbol) => 800
static constexpr size_t SAMPLES_PER_SYMBOL = SAMPLE_RATE / SYMBOL_RATE / 2;  // = 48 samples
static constexpr size_t SAMPLES_PER_BIT = SAMPLES_PER_SYMBOL * 2 * 2;  // = 48 samples
static constexpr size_t SAMPLES_MARGIN = SAMPLES_PER_SYMBOL / 3;  // = 48 samples
static constexpr size_t SAMPLES_ACCUMUMLATOR = SAMPLES_PER_SYMBOL / 20 * 2;  // = 48 samples

static constexpr size_t CARRIER_SAMPLES_THRESHOLD = 0.001f * SAMPLE_RATE; // 120ms de stabilité requise
static constexpr size_t CARRIER_MAX_SAMPLES = 0.200f * SAMPLE_RATE; // 120ms de stabilité requise*/

// Baseband frequency is 3,072,000 samples / sec
static constexpr uint32_t BASEBAND_SAMPLE_RATE = 3072000;
static constexpr uint32_t SAMPLE_RATE = BASEBAND_SAMPLE_RATE / 8 / 8;    // We use to decimators with factor 8 each
static constexpr uint32_t SYMBOL_RATE = 800;                             // 400 bps + Manchester (2 1/2 bits per symbol) => 800
static constexpr size_t SAMPLES_PER_SYMBOL = SAMPLE_RATE / SYMBOL_RATE;  // = 60 samples per symbol
static constexpr size_t SAMPLES_PER_BIT = SAMPLES_PER_SYMBOL * 2;        // = 120 samples per bit
static constexpr size_t SAMPLES_MARGIN = SAMPLES_PER_SYMBOL / 3;         // = Allow 20 sample drift
static constexpr size_t SAMPLES_ACCUMUMLATOR = SAMPLES_PER_SYMBOL / 10;  // Accumulate phase change across 6 samples

static constexpr size_t CARRIER_SAMPLES_THRESHOLD = 0.100f * SAMPLE_RATE;    // Carrier before frame lasts 160ms, require at least 20ms
static constexpr size_t CARRIER_MAX_SAMPLES = 0.900f * SAMPLE_RATE;          // Carrier + frame lasts 160ms + 520ms + 100ms post carrier = 880ms
static constexpr size_t FRAME_MAX_SAMPLES = SAMPLES_PER_BIT * (144 * 1.1f);  // Frame max length (add 1% error margin)

template <typename PreambleMatcher, typename EndMatcher>
class EPIRBPacketBuilder {
   public:
    using EPIRBHandler = void (*)(void* context, const baseband::Packet& packet);

    EPIRBPacketBuilder(
        const PreambleMatcher preamble_matcher,
        const EndMatcher end_matcher,
        void* context,
        EPIRBHandler handler)
        : preamble(preamble_matcher),
          end(end_matcher),
          context(context),
          handler(handler) {
    }

    void execute(
        const uint_fast8_t symbol) {
        bit_history.add(symbol);

        switch (state) {
            case State::Preamble:
                if (preamble(bit_history, packet.size())) {
                    // TODO
                    /*for (size_t i = 0; i < preamble.size(); ++i) {
                        packet.add(bit_history.at_index(i));
                    }*/
                    state = State::Payload;
                }
                break;

            case State::Payload:
                packet.add(symbol);

                if (end(bit_history, packet.size())) {
                    packet.set_timestamp(Timestamp::now());
                    if (handler) handler(context, packet);
                    reset_state();
                } else {
                    if (packet_truncated()) {
                        reset_state();
                    }
                }
                break;

            default:
                reset_state();
                break;
        }
    }

    void reset_state() {
        packet.clear();
        bit_history = BitHistory();
        state = State::Preamble;
    }

   private:
    enum State {
        Preamble,
        Payload,
    };

    bool packet_truncated() const {
        return packet.size() >= packet.capacity();
    }


    BitHistory bit_history{};
    PreambleMatcher preamble{};
    EndMatcher end{};
    void* context;
    EPIRBHandler handler;

    State state{State::Preamble};
    baseband::Packet packet{};
};

class EPIRBProcessor : public BasebandProcessor {
   public:
    EPIRBProcessor();

    void execute(const buffer_c8_t& buffer) override;

    void on_message(const Message* const message) override;

   private:
    AudioOutput audio_output{};

    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    // Variable pour le décodage Manchester (différentiel)
    bool last_bit = false;
    // Time of the last sent frame
    uint32_t last_frame_time{0};
    BitHistory bit_history{};
    uint8_t history_size{0};
    uint16_t sample_count{0};
    uint16_t frame_sample_count{0};
    bool last_phase_positive = false;

    enum State { IDLE,
                 CARRIER_LOCKED,
                 DATA_SYNC,
                 POST_FRAME};
    State current_state = IDLE;

    uint32_t stability_counter = 0;
    float last_phase = 0.0f;

    // Moving Average (30 samples = 1 symbole à 800Hz / 24kHz)
    static constexpr size_t MA_SIZE = SAMPLES_ACCUMUMLATOR;
    float ma_buffer[MA_SIZE] = {0.0f};
    size_t ma_index = 0;
    float ma_sum = 0.0f;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    // Decimation chain for 406 MHz EPIRB signal processing
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    FMSquelch squelch{};
    dsp::demodulate::FM demod{};

    complex16_t last_sample{};

    // Simple bi-phase L decoder state
    uint_fast8_t last_symbol = 0;

    // EPIRB packet structure:
    // - Sync pattern: 000101010101... (15 bits)
    // - Frame sync: 0111110 (7 bits)
    // - Data: 112 bits
    // - BCH error correction: 10 bits
    // Total: 144 bits
    /*    EPIRBPacketBuilder packet_builder{
            //{0b1111111111111110, 16, 0},  // Preamble pattern
            BitPattern{0b11111111, 8, 0},  // Preamble pattern
            //{144-16},                       // Fixed length
            FixedLength{144 - 8},  // Fixed length
            [this](const baseband::Packet& packet) {
                this->payload_handler(packet);
            }};*/

    EPIRBPacketBuilder<BitPattern, FixedLength> packet_builder{
        {0b11111111, 8, 0},
        {144 - 8},
        this,
        [](void* ctx, const baseband::Packet& p) {
            static_cast<EPIRBProcessor*>(ctx)->payload_handler(p);
        }};

    void payload_handler(const baseband::Packet& packet);
    void send_packet(uint64_t data);
    float get_phase_diff(const complex16_t& sample0, const complex16_t& sample1);
    void frame_end();

    // Statistics
    uint32_t packets_received = 0;
    Timestamp last_packet_timestamp{};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        BASEBAND_SAMPLE_RATE, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_EPIRB_H__*/