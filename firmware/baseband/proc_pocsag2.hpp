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

#include <cstdint>
#include <functional>

/* Processes audio stream to normalizes it to +/-1.0f */
class AudioNormalizer {
   public:
    void execute_in_place(const buffer_f32_t& audio) {
        // Decay min/max every second (@24kHz).
        if (counter_ >= 24'000) {
            // 90% decay factor seems to work well.
            // This keeps large transients from wrecking the filter.
            max_ *= 0.9f;
            min_ *= 0.9f;
            counter_ = 0;
            calculate_thresholds();
        }

        counter_ += audio.count;

        for (size_t i = 0; i < audio.count; ++i) {
            auto& val = audio.p[i];

            if (val > max_) {
                max_ = val;
                calculate_thresholds();
            }
            if (val < min_) {
                min_ = val;
                calculate_thresholds();
            }

            if (val >= t_hi_)
                val = 1.0f;
            else if (val <= t_lo_)
                val = -1.0f;
            else
                val = 0.0;
        }
    }

   private:
    void calculate_thresholds() {
        auto center = (max_ + min_) / 2.0f;
        auto range = (max_ - min_) / 2.0f;

        // 10% off center force either +/-1.0f.
        // Higher == larger dead zone.
        // Lower == more false positives.
        auto threshold = range * 0.1;
        t_hi_ = center + threshold;
        t_lo_ = center - threshold;
    }

    uint32_t counter_ = 0;
    float min_ = 99.0f;
    float max_ = -99.0f;
    float t_hi_ = 1.0;
    float t_lo_ = 1.0;
};

/* FIFO wrapper over a uint32_t's bits. */
class BitQueue {
   public:
    void push(bool bit) {
        data_ = (data_ << 1) | (bit ? 1 : 0);
        if (count_ < max_size_) ++count_;
    }

    bool pop() {
        if (count_ == 0) return false;

        --count_;
        return (data_ & (1 << count_)) != 0;
    }

    void reset() {
        data_ = 0;
        count_ = 0;
    }

    uint8_t size() const {
        return count_;
    }

    uint32_t data() const {
        return data_;
    }

   private:
    uint32_t data_ = 0;
    uint8_t count_ = 0;

    static constexpr uint8_t max_size_ = sizeof(data_) * 8;
};

// TODO: better clock sync.
/* Extracts bits and bitrate from audio stream. */
class BitExtractor {
   public:
   private:
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

    const batch_t& batch() const { return batch_; }

    uint32_t current() const { return data_; }
    uint8_t count() const { return word_count_; }
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

class POCSAGProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

    int OnDataFrame(int len, int baud);
    int OnDataWord(uint32_t word, int pos);

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

    // Set once app is ready to receive messages.
    bool configured = false;

    // Buffer for decimated IQ data.
    std::array<complex16_t, 256> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    // Buffer for demodulated audio.
    std::array<float, 16> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    // Decimate to 48kHz.
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    // Filter to 24kHz and demodulate.
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};

    // Squelch to ignore noise.
    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    // LPF to reduce noise. POCSAG supports 2400 baud, but that falls
    // nicely into the transition band of this 1800Hz filter.
    // scipy.signal.butter(2, 1800, "lowpass", fs=24000, analog=False)
    IIRBiquadFilter lpf{{{0.04125354f, 0.082507070f, 0.04125354f},
                         {1.00000000f, -1.34896775f, 0.51398189f}}};

    // Attempts to de-noise and normalize signal.
    AudioNormalizer normalizer{};

    // Handles writing audio stream to hardware.
    AudioOutput audio_output{};

    // Holds the data sent to the app.
    pocsag::POCSAGPacket packet{};

    // Used to keep track of how many samples were processed
    // between status update messages.
    uint32_t samples_processed = 0;

    BitQueue bits{};

    // Processes bits into codewords.
    BitExtractor bit_extractor{};

    // Processes bits into codewords.
    CodewordExtractor word_extractor{
        bits, [this](CodewordExtractor&) {
            send_packet();
        }};

    //--------------------------------------------------
    // Transitional code

    // ----------------------------------------
    // Frame extractraction methods and members
    // ----------------------------------------
    void initFrameExtraction();
    void resetVals();
    void setFrameExtractParams(long a_samplesPerSec, long a_maxBaud = 8000, long a_minBaud = 200, long maxRunOfSameValue = 32);
    int processDemodulatedSamples(float* sampleBuff, int noOfSamples);

    uint32_t getRate();

    uint32_t m_averageSymbolLen_1024{0};
    uint32_t m_lastStableSymbolLen_1024{0};

    uint32_t m_samplesPerSec{0};
    uint32_t m_goodTransitions{0};
    uint32_t m_badTransitions{0};

    uint32_t m_sampleNo{0};
    float m_sample{0};
    float m_valMid{0.0f};
    float m_lastSample{0.0f};

    uint32_t m_lastTransPos_1024{0};
    uint32_t m_lastSingleBitPos_1024{0};

    uint32_t m_nextBitPosInt{0};  // Integer rounded up version to save on ops
    uint32_t m_nextBitPos_1024{0};
    uint32_t m_lastBitPos_1024{0};

    uint32_t m_shortestGoodTrans_1024{0};
    uint32_t m_minSymSamples_1024{0};
    uint32_t m_maxSymSamples_1024{0};
    uint32_t m_maxRunOfSameValue{0};

    //--------------------------------------------------

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_POCSAG2_H__*/
