/*
 * Copyright (C) 1996 Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 * Copyright (C) 2012-2014 Elias Oenal (multimon-ng@eliasoenal.com)
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2016 Kyle Reed
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

#include "proc_pocsag2.hpp"

#include "event_m4.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

using namespace std;

namespace {
/* Count of bits that differ between the two values. */
uint8_t differ_bit_count(uint32_t left, uint32_t right) {
    uint32_t diff = left ^ right;
    uint8_t count = 0;
    for (size_t i = 0; i < sizeof(diff) * 8; ++i) {
        if (((diff >> i) & 0x1) == 1)
            ++count;
    }

    return count;
}
}  // namespace

/* AudioNormalizer ***************************************/

void AudioNormalizer::execute_in_place(const buffer_f32_t& audio) {
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

void AudioNormalizer::calculate_thresholds() {
    auto center = (max_ + min_) / 2.0f;
    auto range = (max_ - min_) / 2.0f;

    // 10% off center force either +/-1.0f.
    // Higher == larger dead zone.
    // Lower == more false positives.
    auto threshold = range * 0.1;
    t_hi_ = center + threshold;
    t_lo_ = center - threshold;
}

/* BitQueue **********************************************/

void BitQueue::push(bool bit) {
    data_ = (data_ << 1) | (bit ? 1 : 0);
    if (count_ < max_size_) ++count_;
}

bool BitQueue::pop() {
    if (count_ == 0) return false;

    --count_;
    return (data_ & (1 << count_)) != 0;
}

void BitQueue::reset() {
    data_ = 0;
    count_ = 0;
}

uint8_t BitQueue::size() const {
    return count_;
}

uint32_t BitQueue::data() const {
    return data_;
}

/* BitExtractor ******************************************/


/* CodewordExtractor *************************************/

void CodewordExtractor::process_bits() {
    // Process all of the bits in the bits queue.
    while (bits_.size() > 0) {
        take_one_bit();

        // Wait until data_ is full.
        if (bit_count_ < data_bit_count)
            continue;

        // Wait for the sync frame.
        if (!has_sync_) {
            if (differ_bit_count(data_, sync_codeword) <= 2)
                handle_sync(false);
            else if (differ_bit_count(data_, ~sync_codeword) <= 2)
                handle_sync(true);
            continue;
        }

        save_current_codeword();

        if (word_count_ == pocsag::batch_size)
            handle_batch_complete();
    }
}

void CodewordExtractor::flush() {
    // Don't bother flushing if there's no pending data.
    if (word_count_ == 0) return;

    pad_idle();
    handle_batch_complete();
}

void CodewordExtractor::reset() {
    clear_data_bits();
    has_sync_ = false;
    inverted_ = false;
    word_count_ = 0;
}

void CodewordExtractor::clear_data_bits() {
    data_ = 0;
    bit_count_ = 0;
}

void CodewordExtractor::take_one_bit() {
    data_ = (data_ << 1) | bits_.pop();
    if (bit_count_ < data_bit_count)
        ++bit_count_;
}

void CodewordExtractor::handle_sync(bool inverted) {
    clear_data_bits();
    has_sync_ = true;
    inverted_ = inverted;
    word_count_ = 0;
}

void CodewordExtractor::save_current_codeword() {
    batch_[word_count_++] = inverted_ ? ~data_ : data_;
    clear_data_bits();
}

void CodewordExtractor::handle_batch_complete() {
    on_batch_(*this);
    has_sync_ = false;
    word_count_ = 0;
}

void CodewordExtractor::pad_idle() {
    while (word_count_ < pocsag::batch_size)
        batch_[word_count_++] = idle_codeword;
}

/* POCSAGProcessor ***************************************/

void POCSAGProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    // buffer has 2048 samples
    // decim0 out: 2048/8 = 256 samples
    // decim1 out: 256/8 = 32 samples
    // channel out: 32/2 = 16 samples

    // Get 24kHz audio
    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    auto audio = demod.execute(channel_out, audio_buffer);

    // Check if there's any signal in the audio buffer.
    bool has_audio = squelch.execute(audio);
    squelch_history = (squelch_history << 1) | (has_audio ? 1 : 0);

    // Has there been any signal recently?
    if (squelch_history == 0) {
        // No recent signal, flush and rprepare for next message.
        if (word_extractor.current() > 0) {
            flush();
            reset();
            send_stats();
        }

        // Clear the audio stream before sending.
        for (size_t i = 0; i < audio.count; ++i)
            audio.p[i] = 0.0;

        audio_output.write(audio);
        return;
    }

    // Filter out high-frequency noise then normalize.
    lpf.execute_in_place(audio);
    normalizer.execute_in_place(audio);
    audio_output.write(audio);

    // Decode the messages from the audio.
    processDemodulatedSamples(audio.p, 16);
    word_extractor.process_bits();

    // Update the status.
    samples_processed += buffer.count;
    if (samples_processed >= stat_update_threshold) {
        send_stats();
        samples_processed -= stat_update_threshold;
    }
}

void POCSAGProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::POCSAGConfigure:
            configure();
            break;

        case Message::ID::NBFMConfigure: {
            auto config = reinterpret_cast<const NBFMConfigureMessage*>(message);
            squelch.set_threshold(config->squelch_level / 99.0);
            break;
        }

        default:
            break;
    }
}

void POCSAGProcessor::configure() {
    constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor;
    constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor;
    constexpr size_t channel_filter_output_fs = decim_1_output_fs / 2;
    constexpr size_t demod_input_fs = channel_filter_output_fs;

    decim_0.configure(taps_11k0_decim_0.taps);
    decim_1.configure(taps_11k0_decim_1.taps);
    channel_filter.configure(taps_11k0_channel.taps, 2);
    demod.configure(demod_input_fs, 4'500);  // FSK +/- 4k5Hz.

    // Don't process the audio stream.
    audio_output.configure(false);

    // Set up the frame extraction, limits of baud.
    setFrameExtractParams(demod_input_fs, 4000, 300, 32);

    // Set ready to process data.
    configured = true;
}

void POCSAGProcessor::flush() {
    word_extractor.flush();
}

void POCSAGProcessor::reset() {
    bits.reset();
    word_extractor.reset();
    samples_processed = 0;
}

void POCSAGProcessor::send_stats() const {
    const auto& ex = word_extractor;
    POCSAGStatsMessage message(ex.current(), ex.count(), ex.has_sync());
    shared_memory.application_queue.push(message);
}

void POCSAGProcessor::send_packet() {
    // TODO: Assert on batch size here?

    packet.set_flag(pocsag::PacketFlag::NORMAL);
    packet.set_timestamp(Timestamp::now());
    packet.set_bitrate(getRate());  // TODO
    packet.set(word_extractor.batch());

    POCSAGPacketMessage message(packet);
    shared_memory.application_queue.push(message);
}

//--------------------------------------------------
// Transitional code

int POCSAGProcessor::OnDataWord(uint32_t word, int pos) {
    packet.set(pos, word);
    return 0;
}

int POCSAGProcessor::OnDataFrame(int len, int baud) {
    if (len > 0) {
        packet.set_bitrate(baud);
        packet.set_flag(pocsag::PacketFlag::NORMAL);
        packet.set_timestamp(Timestamp::now());
        send_packet();
    }
    return 0;
}

#define BAUD_STABLE (104)
#define MAX_CONSEC_SAME (32)
#define MAX_WITHOUT_SINGLE (64)
#define MAX_BAD_TRANS (10)

#define M_SYNC (0x7cd215d8)
#define M_NOTSYNC (0x832dea27)

#define M_IDLE (0x7a89c197)

inline int bitsDiff(unsigned long left, unsigned long right) {
    unsigned long xord = left ^ right;
    int count = 0;
    for (int i = 0; i < 32; i++) {
        if ((xord & 0x01) != 0) ++count;
        xord = xord >> 1;
    }
    return (count);
}

void POCSAGProcessor::initFrameExtraction() {
    m_averageSymbolLen_1024 = m_maxSymSamples_1024;
    m_lastStableSymbolLen_1024 = m_minSymSamples_1024;

    m_badTransitions = 0;
    // m_inverted = false;

    bits.reset();
    resetVals();
}

void POCSAGProcessor::resetVals() {
    // Reset the parameters
    m_goodTransitions = 0;
    m_badTransitions = 0;
    m_averageSymbolLen_1024 = m_maxSymSamples_1024;
    m_shortestGoodTrans_1024 = m_maxSymSamples_1024;
    m_valMid = 0;

    // And reset the counts
    m_lastTransPos_1024 = 0;
    m_lastBitPos_1024 = 0;
    m_lastSample = 0;
    m_sampleNo = 0;
    m_nextBitPos_1024 = m_maxSymSamples_1024;
    m_nextBitPosInt = (long)m_nextBitPos_1024;

    /*// Extraction
    m_fifo.numBits = 0;
    m_fifo.codeword = 0;
    m_gotSync = false;
    m_numCode = 0;*/

    samples_processed = 0;
}

void POCSAGProcessor::setFrameExtractParams(long a_samplesPerSec, long a_maxBaud, long a_minBaud, long maxRunOfSameValue) {
    m_samplesPerSec = a_samplesPerSec;
    m_minSymSamples_1024 = (uint32_t)(1024.0f * (float)a_samplesPerSec / (float)a_maxBaud);
    m_maxSymSamples_1024 = (uint32_t)(1024.0f * (float)a_samplesPerSec / (float)a_minBaud);
    m_maxRunOfSameValue = maxRunOfSameValue;

    m_shortestGoodTrans_1024 = m_maxSymSamples_1024;
    m_averageSymbolLen_1024 = m_maxSymSamples_1024;
    m_lastStableSymbolLen_1024 = m_minSymSamples_1024;

    m_nextBitPos_1024 = m_averageSymbolLen_1024 / 2;
    m_nextBitPosInt = m_nextBitPos_1024 >> 10;

    initFrameExtraction();
}

int POCSAGProcessor::processDemodulatedSamples(float* sampleBuff, int noOfSamples) {
    bool transition = false;
    uint32_t samplePos_1024 = 0;
    uint32_t len_1024 = 0;

    // Loop through the block of data
    // ------------------------------
    for (int pos = 0; pos < noOfSamples; ++pos) {
        m_sample = sampleBuff[pos];
        m_valMid += (m_sample - m_valMid) / 1024.0f;

        ++m_sampleNo;

        // Detect Transition
        // -----------------
        transition = !((m_lastSample < m_valMid) ^ (m_sample >= m_valMid));  // use XOR for speed

        // If this is a transition
        // -----------------------
        if (transition) {
            // Calculate samples since last trans
            // ----------------------------------
            int32_t fractional_1024 = (int32_t)(((m_sample - m_valMid) * 1024) / (m_sample - m_lastSample));
            if (fractional_1024 < 0) {
                fractional_1024 = -fractional_1024;
            }

            samplePos_1024 = (m_sampleNo << 10) - fractional_1024;
            len_1024 = samplePos_1024 - m_lastTransPos_1024;
            m_lastTransPos_1024 = samplePos_1024;

            // If symbol is large enough to be valid
            // -------------------------------------
            if (len_1024 > m_minSymSamples_1024) {
                // Check for shortest good transition
                // ----------------------------------
                if ((len_1024 < m_shortestGoodTrans_1024) &&
                    (m_goodTransitions < BAUD_STABLE))  // detect change of symbol size
                {
                    int32_t fractionOfShortest_1024 = (len_1024 << 10) / m_shortestGoodTrans_1024;

                    // If currently at half the baud rate
                    // ----------------------------------
                    if ((fractionOfShortest_1024 > 410) && (fractionOfShortest_1024 < 614))  // 0.4 and 0.6
                    {
                        m_averageSymbolLen_1024 /= 2;
                        m_shortestGoodTrans_1024 = len_1024;
                    }
                    // If currently at the wrong baud rate
                    // -----------------------------------
                    else if (fractionOfShortest_1024 < 768)  // 0.75
                    {
                        m_averageSymbolLen_1024 = len_1024;
                        m_shortestGoodTrans_1024 = len_1024;
                        m_goodTransitions = 0;
                        m_lastSingleBitPos_1024 = samplePos_1024 - len_1024;
                    }
                }

                // Calc the number of bits since events
                // ------------------------------------
                int32_t halfSymbol_1024 = m_averageSymbolLen_1024 / 2;
                int bitsSinceLastTrans = max((uint32_t)1, (len_1024 + halfSymbol_1024) / m_averageSymbolLen_1024);
                int bitsSinceLastSingle = (((m_sampleNo << 10) - m_lastSingleBitPos_1024) + halfSymbol_1024) / m_averageSymbolLen_1024;

                // Check for single bit
                // --------------------
                if (bitsSinceLastTrans == 1) {
                    m_lastSingleBitPos_1024 = samplePos_1024;
                }

                // If too long since last transition
                // ---------------------------------
                if (bitsSinceLastTrans > MAX_CONSEC_SAME) {
                    resetVals();
                }
                // If too long sice last single bit
                // --------------------------------
                else if (bitsSinceLastSingle > MAX_WITHOUT_SINGLE) {
                    resetVals();
                } else {
                    // If this is a good transition
                    // ----------------------------
                    int32_t offsetFromExtectedTransition_1024 = len_1024 - (bitsSinceLastTrans * m_averageSymbolLen_1024);
                    if (offsetFromExtectedTransition_1024 < 0) {
                        offsetFromExtectedTransition_1024 = -offsetFromExtectedTransition_1024;
                    }
                    if (offsetFromExtectedTransition_1024 < ((int32_t)m_averageSymbolLen_1024 / 4))  // Has to be within 1/4 of symbol to be good
                    {
                        ++m_goodTransitions;
                        uint32_t bitsCount = min((uint32_t)BAUD_STABLE, m_goodTransitions);

                        uint32_t propFromPrevious = m_averageSymbolLen_1024 * bitsCount;
                        uint32_t propFromCurrent = (len_1024 / bitsSinceLastTrans);
                        m_averageSymbolLen_1024 = (propFromPrevious + propFromCurrent) / (bitsCount + 1);
                        m_badTransitions = 0;
                        // if ( len < m_shortestGoodTrans ){m_shortestGoodTrans = len;}
                        //  Store the old symbol size
                        if (m_goodTransitions >= BAUD_STABLE) {
                            m_lastStableSymbolLen_1024 = m_averageSymbolLen_1024;
                        }
                    }
                }

                // Set the point of the last bit if not yet stable
                // -----------------------------------------------
                if ((m_goodTransitions < BAUD_STABLE) || (m_badTransitions > 0)) {
                    m_lastBitPos_1024 = samplePos_1024 - (m_averageSymbolLen_1024 / 2);
                }

                // Calculate the exact positiom of the next bit
                // --------------------------------------------
                int32_t thisPlusHalfsymbol_1024 = samplePos_1024 + (m_averageSymbolLen_1024 / 2);
                int32_t lastPlusSymbol = m_lastBitPos_1024 + m_averageSymbolLen_1024;
                m_nextBitPos_1024 = lastPlusSymbol + ((thisPlusHalfsymbol_1024 - lastPlusSymbol) / 16);

                // Check for bad pos error
                // -----------------------
                if (m_nextBitPos_1024 < samplePos_1024) m_nextBitPos_1024 += m_averageSymbolLen_1024;

                // Calculate integer sample after next bit
                // ---------------------------------------
                m_nextBitPosInt = (m_nextBitPos_1024 >> 10) + 1;

            }  // symbol is large enough to be valid
            else {
                // Bad transition, so reset the counts
                // -----------------------------------
                ++m_badTransitions;
                if (m_badTransitions > MAX_BAD_TRANS) {
                    resetVals();
                }
            }
        }  // end of if transition

        // Reached the point of the next bit
        // ---------------------------------
        if (m_sampleNo >= m_nextBitPosInt) {
            // Everything is good so extract a bit
            // -----------------------------------
            if (m_goodTransitions > 20) {
                // Store value at the center of bit
                // --------------------------------
                // Calculate the bit value
                float sample = (m_sample + m_lastSample) / 2;
                // int32_t sample_1024 = m_sample_1024;
                // NB: "negative" indicates 1.
                bits.push(sample < m_valMid);
            }
            // Check for long 1 or zero
            // ------------------------
            uint32_t bitsSinceLastTrans = ((m_sampleNo << 10) - m_lastTransPos_1024) / m_averageSymbolLen_1024;
            if (bitsSinceLastTrans > m_maxRunOfSameValue) {
                resetVals();
            }

            // Store the point of the last bit
            // -------------------------------
            m_lastBitPos_1024 = m_nextBitPos_1024;

            // Calculate the exact point of the next bit
            // -----------------------------------------
            m_nextBitPos_1024 += m_averageSymbolLen_1024;

            // Look for the bit after the next bit pos
            // ---------------------------------------
            m_nextBitPosInt = (m_nextBitPos_1024 >> 10) + 1;

        }  // Reached the point of the next bit

        m_lastSample = m_sample;

    }  // Loop through the block of data

    return bits.size();
}

uint32_t POCSAGProcessor::getRate() {
    return ((m_samplesPerSec << 10) + 512) / m_lastStableSymbolLen_1024;
}

/* main **************************************************/

int main() {
    EventDispatcher event_dispatcher{std::make_unique<POCSAGProcessor>()};
    event_dispatcher.run();
    return 0;
}
