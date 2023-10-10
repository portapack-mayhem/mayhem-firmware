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

#include "proc_fsk_rx.hpp"

#include "event_m4.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

using namespace std;

namespace 
{
    /* Count of bits that differ between the two values. */
    uint8_t diff_bit_count(uint32_t left, uint32_t right) 
    {
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

void AudioNormalizer::execute_in_place(const buffer_f32_t& audio) 
{
    // Decay min/max every second (@24kHz).
    if (counter_ >= 24'000) 
    {
        // 90% decay factor seems to work well.
        // This keeps large transients from wrecking the filter.
        max_ *= 0.9f;
        min_ *= 0.9f;
        counter_ = 0;
        calculate_thresholds();
    }

    counter_ += audio.count;

    for (size_t i = 0; i < audio.count; ++i) 
    {
        auto& val = audio.p[i];

        if (val > max_) 
        {
            max_ = val;
            calculate_thresholds();
        }
        if (val < min_) 
        {
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

void AudioNormalizer::calculate_thresholds() 
{
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

void BitQueue::push(bool bit) 
{
    data_ = (data_ << 1) | (bit ? 1 : 0);
    if (count_ < max_size_) ++count_;
}

bool BitQueue::pop() 
{
    if (count_ == 0) return false;

    --count_;
    return (data_ & (1 << count_)) != 0;
}

void BitQueue::reset() 
{
    data_ = 0;
    count_ = 0;
}

uint8_t BitQueue::size() const 
{
    return count_;
}

uint32_t BitQueue::data() const 
{
    return data_;
}

/* BitExtractor ******************************************/

void BitExtractor::extract_bits(const buffer_f32_t& audio) 
{
    // Assumes input has been normalized +/- 1.0f.
    // Positive == 0, Negative == 1.
    for (size_t i = 0; i < audio.count; ++i) 
    {
        auto sample = audio.p[i];

        if (current_rate_) 
        {
            if (current_rate_->handle_sample(sample)) 
            {
                auto value = (current_rate_->bits.data() & 1) == 1;
                bits_.push(value);
            }
        } 
        else 
        {
            // Feed sample to all known rates for clock detection.
            for (auto& rate : known_rates_) {
                if (rate.handle_sample(sample) &&
                    diff_bit_count(rate.bits.data(), clock_magic_number) <= 3) {
                    // Clock detected, continue with this rate.
                    rate.is_stable = true;
                    current_rate_ = &rate;
                }
            }
        }
    }
}

void BitExtractor::configure(uint32_t sample_rate) 
{
    sample_rate_ = sample_rate;

    // Build the baud rate info table based on the sample rate.
    // Sampling at 2x the baud rate to synchronize to bit transitions
    // without needing to know exact transition boundaries.
    for (auto& rate : known_rates_)
        rate.sample_interval = sample_rate / (2.0 * rate.baud_rate);
}

void BitExtractor::reset() 
{
    current_rate_ = nullptr;

    for (auto& rate : known_rates_)
        rate.reset();
}

uint16_t BitExtractor::baud_rate() const 
{
    return current_rate_ ? current_rate_->baud_rate : 0;
}

bool BitExtractor::RateInfo::handle_sample(float sample) 
{
    samples_until_next -= 1;

    // Time to process a sample?
    if (samples_until_next > 0)
        return false;

    bool value = signbit(sample);  // NB: negative == '1'
    bool bit_pushed = false;

    switch (state) 
    {
        case State::WaitForSample:
            // Just need to wait for the first sample of the bit.
            state = State::ReadyToSend;
            break;

        case State::ReadyToSend:
            if (!is_stable && prev_value != value) 
            {
                // Still looking for the clock signal but found a transition.
                // Nudge the next sample a bit to try avoiding pulse edges.
                samples_until_next += (sample_interval / 8.0);
            } 
            else 
            {
                // Either the clock has been found or both samples were
                // (probably) in the same pulse. Send the bit.
                // TODO: Wider/more samples for noise reduction?
                state = State::WaitForSample;
                bit_pushed = true;
                bits.push(value);
            }

            break;
    }

    // How long until the next sample?
    samples_until_next += sample_interval;
    prev_value = value;

    return bit_pushed;
}

void BitExtractor::RateInfo::reset() 
{
    state = State::WaitForSample;
    samples_until_next = 0.0;
    prev_value = false;
    is_stable = false;
    bits.reset();
}

void FSKRxProcessor::clear_data_bits() {
    data = 0;
    bit_count = 0;
}

void FSKRxProcessor::take_one_bit() {
    data = (data << 1) | bits.pop();
    if (bit_count < data_bit_count)
        ++bit_count;
}

void FSKRxProcessor::handle_sync(bool inverted) {
    clear_data_bits();
    has_sync_ = true;
    inverted = inverted;
    word_count = 0;
}

void FSKRxProcessor::process_bits() 
{
    // Process all of the bits in the bits queue.
    while (bits.size() > 0) 
    {
        take_one_bit();

        // Wait until data_ is full.
        if (bit_count < data_bit_count)
            continue;

        // Wait for the sync frame.
        if (!has_sync_) 
        {
            if (diff_bit_count(data, sync_codeword) <= 2)
                handle_sync(/*inverted=*/false);
            else if (diff_bit_count(data, ~sync_codeword) <= 2)
                handle_sync(/*inverted=*/true);
            continue;
        }
    }
}

/* POCSAGProcessor ***************************************/

void FSKRxProcessor::execute(const buffer_c8_t& buffer) {
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
    if (squelch_history == 0) 
    {
        // No recent signal, flush and prepare for next message.
        // if (word_extractor.current() > 0) {
        //     flush();
        //     reset();
        //     send_stats();
        // }

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
    bit_extractor.extract_bits(audio);
    process_bits();
    
    // Update the status.
    samples_processed += buffer.count;
    if (samples_processed >= stat_update_threshold) 
    {
        //send_packet();
        samples_processed -= stat_update_threshold;
    }
}

void FSKRxProcessor::on_message(const Message* const message) 
{
    if (message->id == Message::ID::FSKRxConfigure)
         configure(*reinterpret_cast<const FSKRxConfigureMessage*>(message));

    if (message->id == Message::ID::CaptureConfig)
    capture_config(*reinterpret_cast<const CaptureConfigMessage*>(message));
}

void FSKRxProcessor::configure(const FSKRxConfigureMessage& message) 
{
    constexpr size_t decim_0_input_fs = baseband_fs;
    constexpr size_t decim_0_output_fs = decim_0_input_fs / decim_0.decimation_factor;

    constexpr size_t decim_1_input_fs = decim_0_output_fs;
    constexpr size_t decim_1_output_fs = decim_1_input_fs / decim_1.decimation_factor;

    constexpr size_t channel_filter_input_fs = decim_1_output_fs;
    const size_t channel_filter_output_fs = channel_filter_input_fs / message.channel_decimation;

    const size_t demod_input_fs = channel_filter_output_fs;

    decim_0.configure(message.decim_0_filter.taps);
    decim_1.configure(message.decim_1_filter.taps);
    channel_filter.configure(message.channel_filter.taps, message.channel_decimation);
    demod.configure(demod_input_fs, message.deviation);

    // Don't process the audio stream.
    audio_output.configure(false);

    bit_extractor.configure(demod_input_fs);

    // Set ready to process data.
    configured = true;
}

void FSKRxProcessor::capture_config(const CaptureConfigMessage& message) 
{
    if (message.config) 
    {
        // stream = std::make_unique<StreamInput>(message.config);
        audio_output.set_stream(std::make_unique<StreamInput>(message.config));
    } 
    else 
    {
        // stream.reset();
        audio_output.set_stream(nullptr);
    }
}

void FSKRxProcessor::flush() 
{
    //word_extractor.flush();
}

void FSKRxProcessor::reset() 
{
    clear_data_bits();
    has_sync_ = false;
    inverted = false;
    word_count = 0;

    bits.reset();
    bit_extractor.reset();
    samples_processed = 0;
}

void FSKRxProcessor::send_packet(uint32_t data)
{
    data_message.is_data = true;
    data_message.value = data;
    shared_memory.application_queue.push(data_message);
}

/* main **************************************************/

int main() 
{
    EventDispatcher event_dispatcher{std::make_unique<FSKRxProcessor>()};
    event_dispatcher.run();
    return 0;
}
