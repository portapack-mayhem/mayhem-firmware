/*
 * Copyright (C) 2024 EPIRB Receiver Implementation
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

// Specan is disable to keep application size below the 32k limit
// #define SPECAN

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

// COSPAS / SARSAT 406 frame constants
// Size of preamble (bits)
#define COSPAS_PREAMBLE_SIZE 24
// Size of long frame (bits)
#define COSPAS_LONG_FRAME_SIZE 144
// Size of short frame (bits)
#define COSPAS_SHORT_FRAME_SIZE 112
// Preamble for real frames
#define COSPAS_REAL_PREAMBLE 0b1111'1111'1111'1110'0010'1111
// Preamble for test frames
#define COSPAS_TEST_PREAMBLE 0b1111'1111'1111'1110'1101'0000

// Dedicated EPIRB PacketBuilder
// Uses dedicated preamble detection logic to find both real and test frames
// Also as a dedicated packet size detection based on frame's size bit
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
                // Detect both real and test fram preambles
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
        // Timestamp is not set here to save some app space (not used on ui side)
        // packet.set_timestamp(Timestamp::now());
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
    // Baseband frequency is set to 3,072,000 samples / sec
    static constexpr uint32_t BASEBAND_SAMPLE_RATE = 3072000;
    static constexpr uint32_t SAMPLE_RATE = BASEBAND_SAMPLE_RATE / 8 / 8;    // We use to decimators with factor 8 each
    static constexpr uint32_t SYMBOL_RATE = 800;                             // 400 bps + Manchester (2 1/2 bits per symbol) => 800
    static constexpr size_t SAMPLES_PER_SYMBOL = SAMPLE_RATE / SYMBOL_RATE;  // = 60 samples per symbol
    static constexpr size_t SAMPLES_PER_BIT = SAMPLES_PER_SYMBOL * 2;        // = 120 samples per bit
    static constexpr size_t SAMPLES_MARGIN = SAMPLES_PER_SYMBOL / 3;         // = Allow 20 sample drift
    static constexpr size_t SAMPLES_ACCUMULATOR = SAMPLES_PER_SYMBOL / 5;    // Accumulate phase change across 12 samples
    static constexpr size_t RISE_FILTER_SAMPLES = SAMPLES_PER_SYMBOL / 20;   // Filter peaks of less than 3 samples

    static constexpr size_t CARRIER_SAMPLES_THRESHOLD = 0.080f * SAMPLE_RATE;    // Carrier before frame lasts 160ms, require at least 80ms
    static constexpr size_t CARRIER_MAX_SAMPLES = 0.900f * SAMPLE_RATE;          // Carrier + frame lasts 160ms + 520ms + 100ms post carrier = 880ms
    static constexpr size_t FRAME_MAX_SAMPLES = SAMPLES_PER_BIT * (144 * 1.1f);  // Frame max length (add 1% error margin)

    AudioOutput audio_output{};

    // Config
    uint8_t squelch_level{50};
    // Audio on/off logic is disabled to save app space
    // bool audio_on{true};
#ifdef SPECAN
    bool spectrum_on{false};
#endif

    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{
        audio.data(),
        audio.size()};

    // Last received bit (for manchester deconding)
    bool last_bit = false;
    // Sample count since last symbol
    uint16_t sample_count{0};
    // Sample count since frame start
    uint16_t frame_sample_count{0};
    // True if last phase shift was positive, false otherwise
    bool last_phase_positive = false;
    // Counter used for peak filtering
    uint16_t rise_detection_count{0};

    // Frame detection state machine states
    enum State { IDLE,
                 CARRIER_LOCKED,
                 DATA_SYNC,
                 POST_FRAME };
    // Current state for frame detection state machine
    State current_state = IDLE;
    // Carrier detection counter
    uint32_t stability_counter = 0;

    // Phase delta accumulator (12 samples)
    static constexpr size_t PHASE_DELTA_ACC_SIZE = SAMPLES_PER_SYMBOL / 5;  // 12 samples
    float phase_delta_buffer[PHASE_DELTA_ACC_SIZE] = {0.0f};
    size_t phase_delta_index = 0;
    float phase_delta_acc = 0.0f;

    // Automatic Frequency Control (AFC)
    // A residual carrier frequency offset between the tuner and the beacon shows
    // up as a constant per-sample phase rotation. We measure its mean over the
    // unmodulated carrier preamble and subtract it from every raw phase delta so
    // the carrier-stability detection and the +/-2.2 rad data jumps stay centered.
    // Current estimate (rad/sample), removed from each raw phase delta.
    float freq_offset_est = 0.0f;
    // Carrier-tracking loop gain. Applied per sample in IDLE so the estimate
    // pulls in any offset within the discriminator's +/-SAMPLE_RATE/2 (~24 kHz)
    // range *before* the carrier-detection thresholds run. First-order loop with
    // time constant ~1/ALPHA samples (= 200 samples ~ 4 ms at 48 kHz). Tuned so a
    // +/-5 kHz offset (0.654 rad/sample) decays the 12-sample accumulator below
    // the 0.6 rad lock threshold in ~11 ms (~13 ms at 7 kHz) -- well inside the
    // 160 ms preamble / 80 ms stability window, even if reception starts partway
    // through the carrier -- while keeping added acquisition jitter negligible.
    static constexpr float AFC_TRACK_ALPHA = 0.005f;
    // AFC update gating: ignore large per-sample phase jumps (likely noise)
    static constexpr float AFC_UPDATE_PHASE_MAX = 0.8f;  // rad/sample
    // AFC Convergence detection: track variance of AFC estimate to ensure it has stabilized
    // before transitioning from IDLE to CARRIER_LOCKED state
    static constexpr float AFC_CONVERGENCE_THRESHOLD = 0.001f;  // Max variance for convergence
    float afc_mean = 0.0f;                                      // Running mean of AFC estimate
    float afc_m2 = 0.0f;                                        // Sum of squared differences (Welford's algorithm)
    uint32_t afc_convergence_n = 0;                             // Sample count for AFC convergence calculation
    // Running mean of the raw phase delta while a stable carrier is present.
    float carrier_phase_sum = 0.0f;
    uint32_t carrier_phase_n = 0;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    // Decimation chain for 406 MHz EPIRB signal processing
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    // Audio filtering
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    // Audio demodulation
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
    // Compute phase diff between two samples
    float get_phase_diff(const complex16_t& sample0, const complex16_t& sample1);
    // End current frame
    void frame_end();
    // Rise detection with peak filtering
    bool filtered_rise_detect(bool condition);
    // Configure audio processing
    void configure_audio();

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{
        BASEBAND_SAMPLE_RATE, this, baseband::Direction::Receive, /*auto_start*/ false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_EPIRB_H__*/
