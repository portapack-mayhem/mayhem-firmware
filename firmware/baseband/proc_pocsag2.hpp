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

/*
Can't we just send codewords as we get them? Why wait for 16?
Is there some kind of cross-word CRC or something?
I think there's a sync frame and then a series of codewords per message.
But there's no good reason to buffer them, is there?
*/

class BitExtractor {};

class WordExtractor {};

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
    void send_stats() const;

    // Set once app is ready to receive messages.
    bool configured = false;

    // Buffer for decimated IQ data.
    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    // Buffer for demodulated audio.
    std::array<float, 32> audio{};
    const buffer_f32_t audio_buffer{audio.data(), audio.size()};

    // Decimate to 48kHz.
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};

    // Filter to 24kHz and demodulate.
    dsp::decimate::FIRAndDecimateComplex channel_filter{};
    dsp::demodulate::FM demod{};

    // LPF to reduce noise - NB: can be BAUD/2.
    // scipy.signal.butter(2, 1200, "lowpass", fs=24000, analog=False)
    IIRBiquadFilter lpf{{{0.02008337f, 0.04016673f, 0.02008337f},
                         {1.00000000f, -1.56101808f, 0.64135154f}}};

    // Squelch to ignore noise.
    FMSquelch squelch{};
    uint64_t squelch_history = 0;

    // Handles writing audio stream to hardware.
    AudioOutput audio_output{};

    // Holds the data sent to the app.
    pocsag::POCSAGPacket packet{};

    bool has_been_reset = true;
    uint32_t samples_processed = 0;

    //--------------------------------------------------

    // ----------------------------------------
    // Frame extractraction methods and members
    // ----------------------------------------
    void initFrameExtraction();
    struct FIFOStruct {
        unsigned long codeword;
        int numBits;
    };

    void resetVals();
    void setFrameExtractParams(long a_samplesPerSec, long a_maxBaud = 8000, long a_minBaud = 200, long maxRunOfSameValue = 32);

    int processDemodulatedSamples(float* sampleBuff, int noOfSamples);
    int extractFrames();

    void storeBit();
    short getBit();

    int getNoOfBits();
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

    static constexpr long BIT_BUF_SIZE = 64;
    std::bitset<64> m_bits{0};
    long m_bitsStart{0};
    long m_bitsEnd{0};

    FIFOStruct m_fifo{0, 0};
    bool m_gotSync{false};
    int m_numCode{0};
    bool m_inverted{false};

    //--------------------------------------------------

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_POCSAG2_H__*/
