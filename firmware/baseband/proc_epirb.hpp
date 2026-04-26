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
#include "packet_builder.hpp"
#include "baseband_packet.hpp"
#include "message.hpp"
#include "buffer.hpp"

#ifdef SPECAN
#include "spectrum_collector.hpp"
#endif

#include "audio_output.hpp"
#include "dsp_demodulate.hpp"

// Forward declarations for types only used as pointers/references
class Message;
namespace baseband {
class Packet;
}

#define COSPAS_PREAMBLE_SIZE 24
#define COSPAS_LONG_FRAME_SIZE 144
#define COSPAS_SHORT_FRAME_SIZE 112
#define COSPAS_REAL_PREAMBLE 0b1111'1111'1111'1110'0010'1111
#define COSPAS_TEST_PREAMBLE 0b1111'1111'1111'1110'1101'0000
class EPIRBPacketBuilder {
   public:
    using EPIRBHandler = void (*)(void* context, const baseband::Packet& packet);

    EPIRBPacketBuilder(
        void* context,
        EPIRBHandler handler)
        : context(context),
          handler(handler) {
    }

    void execute(
        const uint_fast8_t symbol) {
        bit_history.add(symbol);

        switch (state) {
            case State::Preamble: {
                bool is_real = real_sync_matcher(bit_history, packet.size());
                bool is_test = test_sync_matcher(bit_history, packet.size());
                if (is_real || is_test) {
                    // Append preamble to the begining of the packet
                    uint64_t preamble = is_real ? COSPAS_REAL_PREAMBLE : COSPAS_TEST_PREAMBLE;
                    for (int8_t i = (COSPAS_PREAMBLE_SIZE - 1); i >= 0; i--) {
                        packet.add((preamble >> i) & 0x1);
                    }
                    state = State::Format;
                }
            } break;
            case State::Format:
                packet.add(symbol);
                // 144 bits for long frames and 112 for short frames
                size = symbol ? COSPAS_LONG_FRAME_SIZE : COSPAS_SHORT_FRAME_SIZE;
                state = State::Payload;

                break;
            case State::Payload:
                packet.add(symbol);

                if (packet.size() >= size) {
                    flush();
                } else {
                    if (packet.size() >= packet.capacity()) {
                        reset_state();
                    }
                }
                break;

            default:
                reset_state();
                break;
        }
    }

    void flush() {
        //packet.set_timestamp(Timestamp::now());
        if (handler) handler(context, packet);
        reset_state();
    }

    void reset_state() {
        packet.clear();
        bit_history = BitHistory();
        state = State::Preamble;
    }

   private:
    enum State {
        Preamble,
        Format,
        Payload,
    };

    BitHistory bit_history{};
    BitPattern real_sync_matcher{COSPAS_REAL_PREAMBLE, COSPAS_PREAMBLE_SIZE};
    BitPattern test_sync_matcher{COSPAS_TEST_PREAMBLE, COSPAS_PREAMBLE_SIZE};
    void* context;
    EPIRBHandler handler;
    uint8_t size{0};

    State state{State::Preamble};
    baseband::Packet packet{};
};

class EPIRBProcessor : public BasebandProcessor {
   public:
    EPIRBProcessor();

    void execute(const buffer_c8_t& buffer) override;

    void on_message(const Message* const message) override;

   private:
    // Baseband frequency is 3,072,000 samples / sec
    static constexpr uint32_t BASEBAND_SAMPLE_RATE = 3072000;
    static constexpr uint32_t SAMPLE_RATE = BASEBAND_SAMPLE_RATE / 8 / 8;    // We use to decimators with factor 8 each
    static constexpr uint32_t SYMBOL_RATE = 800;                             // 400 bps + Manchester (2 1/2 bits per symbol) => 800
    static constexpr size_t SAMPLES_PER_SYMBOL = SAMPLE_RATE / SYMBOL_RATE;  // = 60 samples per symbol
    static constexpr size_t SAMPLES_PER_BIT = SAMPLES_PER_SYMBOL * 2;        // = 120 samples per bit
    static constexpr size_t SAMPLES_MARGIN = SAMPLES_PER_SYMBOL / 3;         // = Allow 20 sample drift
    static constexpr size_t SAMPLES_ACCUMUMLATOR = SAMPLES_PER_SYMBOL / 5;  // Accumulate phase change across 6 samples
    static constexpr size_t RISE_FILTER_SAMPLES = SAMPLES_PER_SYMBOL / 20;   // Frame max length (add 1% error margin)

    static constexpr size_t CARRIER_SAMPLES_THRESHOLD = 0.080f * SAMPLE_RATE;    // Carrier before frame lasts 160ms, require at least 80ms
    static constexpr size_t CARRIER_MAX_SAMPLES = 0.900f * SAMPLE_RATE;          // Carrier + frame lasts 160ms + 520ms + 100ms post carrier = 880ms
    static constexpr size_t FRAME_MAX_SAMPLES = SAMPLES_PER_BIT * (144 * 1.1f);  // Frame max length (add 1% error margin)

    AudioOutput audio_output{};

    // Config
    uint8_t squelch_level{50};
    //bool audio_on{true};
#ifdef SPECAN
    bool spectrum_on{false};
#endif

    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    // Last received bit (for manchester deconding)
    bool last_bit = false;
    // Bit history for debug purpose
    // BitHistory bit_history{};
    // uint8_t history_size{0};
    uint16_t sample_count{0};
    uint16_t frame_sample_count{0};
    bool last_phase_positive = false;
    uint16_t rise_detection_count{0};

    enum State { IDLE,
                 CARRIER_LOCKED,
                 DATA_SYNC,
                 POST_FRAME };
    State current_state = IDLE;

    uint32_t stability_counter = 0;

    // Phase delta accumulator (6 samples)
    static constexpr size_t PHASE_DELTA_ACC_SIZE = SAMPLES_ACCUMUMLATOR;
    float phase_delta_buffer[PHASE_DELTA_ACC_SIZE] = {0.0f};
    size_t pahse_delta_index = 0;
    float phase_delta_acc = 0.0f;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    // Decimation chain for 406 MHz EPIRB signal processing
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};
#ifdef SPECAN
    SpectrumCollector channel_spectrum{};
#endif
    // Store last stample for phase delta calculation
    complex16_t last_sample{};

    // EPIRB packet structure:
    // - Sync pattern: 111111111111111 (15 bits)
    // - Frame sync: 000101111(real) / 011010000(test) (9 bits)
    // - Data: 120 bits (long frame) / // bits (short frame)
    // - BCH error correction: 10 bits
    // Total: 144 bits (long frame) / 112  bits (short frame)
    EPIRBPacketBuilder packet_builder{
        this,
        [](void* ctx, const baseband::Packet& p) {
            static_cast<EPIRBProcessor*>(ctx)->payload_handler(p);
        }};

    void payload_handler(const baseband::Packet& packet);
    // void send_packet(uint64_t data);
    float get_phase_diff(const complex16_t& sample0, const complex16_t& sample1);
    void frame_end();
    bool filtered_rise_detect(bool condition);
    void configure_audio();

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        BASEBAND_SAMPLE_RATE, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_EPIRB_H__*/