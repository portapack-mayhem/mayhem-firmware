/*
 * Copyright (C) 1996 Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 * Copyright (C) 2012-2014 Elias Oenal (multimon-ng@eliasoenal.com)
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

#ifndef __PROC_POCSAG2_H__
#define __PROC_POCSAG2_H__

/* https://www.aaroncake.net/schoolpage/pocsag.htm */

#include "audio_output.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "dsp_decimate.hpp"
#include "dsp_demodulate.hpp"
#include "dsp_iir_config.hpp"
#include "message.hpp"
#include "pocsag.hpp"
#include "pocsag_packet.hpp"
#include "portapack_shared_memory.hpp"
#include "rssi_thread.hpp"

#include <array>
#include <cstdint>
#include <functional>

/* Normalizes audio stream to +/-1.0f */
class AudioNormalizer {
   public:
    void execute_in_place(const buffer_f32_t& audio);

   private:
    void calculate_thresholds();

    uint32_t counter_ = 0;
    float min_ = 99.0f;
    float max_ = -99.0f;
    float t_hi_ = 1.0;
    float t_lo_ = 1.0;
};

/* FIFO wrapper over a uint32_t's bits. */
class BitQueue {
   public:
    void push(bool bit);
    bool pop();
    void reset();
    uint8_t size() const;
    uint32_t data() const;

   private:
    uint32_t data_ = 0;
    uint8_t count_ = 0;

    static constexpr uint8_t max_size_ = sizeof(data_) * 8;
};

/* Extracts bits and bitrate from audio stream. */
class BitExtractor {
   public:
    BitExtractor(BitQueue& bits)
        : bits_{bits} {}

    void extract_bits(const buffer_f32_t& audio);
    void configure(uint32_t sample_rate);
    void reset();
    uint16_t baud_rate() const;

   private:
    /* Clock signal detection magic number. */
    static constexpr uint32_t clock_magic_number = 0xAAAAAAAA;

    struct RateInfo {
        enum class State : uint8_t {
            WaitForSample,
            ReadyToSend
        };

        const int16_t baud_rate = 0;
        float sample_interval = 0.0;

        State state = State::WaitForSample;
        float samples_until_next = 0.0;
        bool prev_value = false;
        bool is_stable = false;
        BitQueue bits{};

        /* Updates a rate info with the given sample.
         * Returns true if the rate info has a new bit in its queue. */
        bool handle_sample(float sample);
        void reset();
    };

    std::array<RateInfo, 3> known_rates_{
        RateInfo{512},
        RateInfo{1200},
        RateInfo{2400}};

    BitQueue& bits_;

    uint32_t sample_rate_ = 0;
    RateInfo* current_rate_ = nullptr;
};

/* Extracts codeword batches from the BitQueue. */
class CodewordExtractor {
   public:
    using batch_t = pocsag::batch_t;
    using batch_handler_t = std::function<void(CodewordExtractor&)>;

    CodewordExtractor(BitQueue& bits, batch_handler_t on_batch)
        : bits_{bits}, on_batch_{on_batch} {}

    /* Process the BitQueue to extract codeword batches. */
    void process_bits();

    /* Pad then send any pending frames. */
    void flush();

    /* Completely reset to prepare for a new message. */
    void reset();

    /* Gets the underlying batch array. */
    const batch_t& batch() const { return batch_; }

    /* Gets in-progress codeword. */
    uint32_t current() const { return data_; }

    /* Gets the count of completed codewords. */
    uint8_t count() const { return word_count_; }

    /* Returns true if the batch has as sync frame. */
    bool has_sync() const { return has_sync_; }

   private:
    /* Sync frame codeword. */
    static constexpr uint32_t sync_codeword = 0x7cd215d8;

    /* Idle codeword used to pad a 16 codeword "batch". */
    static constexpr uint32_t idle_codeword = 0x7a89c197;

    /* Number of bits in 'data_' member. */
    static constexpr uint8_t data_bit_count = sizeof(uint32_t) * 8;

    /* Clears data_ and bit_count_ to prepare for next codeword. */
    void clear_data_bits();

    /* Pop a bit off the queue and add it to data_. */
    void take_one_bit();

    /* Handles receiving the sync frame codeword, start of batch. */
    void handle_sync(bool inverted);

    /* Saves the current codeword in data_ to the batch. */
    void save_current_codeword();

    /* Sends the batch to the handler, resets for next batch. */
    void handle_batch_complete();

    /* Fill the rest of the batch with 'idle' codewords. */
    void pad_idle();

    BitQueue& bits_;
    batch_handler_t on_batch_{};

    /* When true, sync frame has been received. */
    bool has_sync_ = false;

    /* When true, bit vales are flipped in the codewords. */
    bool inverted_ = false;

    uint32_t data_ = 0;
    uint8_t bit_count_ = 0;
    uint8_t word_count_ = 0;
    batch_t batch_{};
};

/* Processes POCSAG signal into codeword batches. */
class POCSAGProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    static constexpr size_t baseband_fs = 3072000;
    static constexpr uint8_t stat_update_interval = 10;
    static constexpr uint32_t stat_update_threshold =
        baseband_fs / stat_update_interval;

    void configure();
    void flush();
    void reset();
    void send_stats() const;
    void send_packet();
    void on_beep_message(const AudioBeepMessage& message);

    /* Set once app is ready to receive messages. */
    bool configured = false;

    /* Buffer for decimated IQ data. */
    std::array<complex16_t, 256> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    /* Buffer for demodulated audio. */
    std::array<float, 16> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    /* Decimate to 48kHz. */
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    /* Filter to 24kHz and demodulate. */
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};

    /* Squelch to ignore noise. */
    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    /* LPF to reduce noise. POCSAG supports 2400 baud, but that falls
     * nicely into the transition band of this 1800Hz filter.
     * scipy.signal.butter(2, 1800, "lowpass", fs=24000, analog=False) */
    IIRBiquadFilter lpf{{{0.04125354f, 0.082507070f, 0.04125354f},
                         {1.00000000f, -1.34896775f, 0.51398189f}}};

    /* Attempts to de-noise and normalize signal. */
    AudioNormalizer normalizer{};

    /* Handles writing audio stream to hardware. */
    AudioOutput audio_output{};

    /* Holds the data sent to the app. */
    pocsag::POCSAGPacket packet{};

    /* Used to keep track of how many samples were processed
     * between status update messages. */
    uint32_t samples_processed = 0;

    BitQueue bits{};

    /* Processes audio into bits. */
    BitExtractor bit_extractor{bits};

    /* Processes bits into codewords. */
    CodewordExtractor word_extractor{
        bits, [this](CodewordExtractor&) {
            send_packet();
        }};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_POCSAG2_H__*/
