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

#include "proc_pocsag2.hpp"

#include "event_m4.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

using namespace std;

namespace {
/* Count of bits that differ between the two values. */
uint8_t diff_bit_count(uint32_t left, uint32_t right) {
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

void BitExtractor::extract_bits(const buffer_f32_t& audio) {
    // Assumes input has been normalized +/- 1.0f.
    for (size_t i = 0; i < audio.count; ++i) {
        auto sample = audio.p[i];
        samples_until_next_ -= 1;

        if (!current_rate_) {
            // Feed the known rate queues for clock detection.
            for (auto& rate : known_rates_) {
                if (handle_sample(rate, sample) &&
                    diff_bit_count(rate.bits.data(), clock_magic_number) <= 2) {
                    // Clock detected.
                    // NB: This block should only happen on the second sample of a pulse.
                    // samples_until_next_ to start sampling the *next* pulse.
                    current_rate_ = &rate;
                    samples_until_next_ = rate.sample_interval;
                    ready_to_send_ = false;
                }
            }
        }

        // Have a clock rate and it's time to process the next sample.
        if (current_rate_ && samples_until_next_ <= 0) {
            // TODO: It seems like it would be possible to combine this
            // code with handle_sample. Nearly the same work.

            // Only send on the second sample of a bit.
            // Sampling twice helps mitigate noisy audio data.
            if (ready_to_send_) {
                auto value = (prev_sample_ + sample) / 2;
                bits_.push(signbit(value));  // NB: negative == '1'
            }

            ready_to_send_ = !ready_to_send_;
            prev_sample_ = sample;
            samples_until_next_ += current_rate_->sample_interval;
        }
    }
}

void BitExtractor::configure(uint32_t sample_rate) {
    sample_rate_ = sample_rate;

    // Build the baud rate info table based on the sample rate.
    // Sampling at 2x the baud rate to synchronize to bit transitions
    // without needing to know exact transition boundaries.
    for (auto& rate : known_rates_)
        rate.sample_interval = sample_rate / (2.0 * rate.baud_rate);
}

void BitExtractor::reset() {
    current_rate_ = nullptr;
    samples_until_next_ = 0.0;
    prev_sample_ = 0.0;
    ready_to_send_ = false;

    for (auto& rate : known_rates_) {
        rate.samples_until_next = 0.0;
        rate.last_sample = 0.0;
        rate.bits.reset();
    }
}

uint16_t BitExtractor::baud_rate() const {
    return current_rate_ ? current_rate_->baud_rate : 0;
}

bool BitExtractor::handle_sample(RateInfo& rate, float sample) {
    // TODO: Still getting some clock misses at the start of messages.
    rate.samples_until_next -= 1;

    // Not time to process a sample yet.
    if (rate.samples_until_next > 0)
        return false;

    // Sample signs are the same, both samples are in the same bit pulse.
    auto has_new_bit = signbit(sample) == signbit(rate.last_sample);
    if (has_new_bit)
        rate.bits.push(signbit(sample));  // NB: negative == '1'

    // How long until the next sample?
    rate.samples_until_next += rate.sample_interval;
    rate.last_sample = sample;

    return has_new_bit;
}

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
            if (diff_bit_count(data_, sync_codeword) <= 2)
                handle_sync(/*inverted=*/false);
            else if (diff_bit_count(data_, ~sync_codeword) <= 2)
                handle_sync(/*inverted=*/true);
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
        // No recent signal, flush and prepare for next message.
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
    bit_extractor.extract_bits(audio);
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

    bit_extractor.configure(demod_input_fs);

    // Set ready to process data.
    configured = true;
}

void POCSAGProcessor::flush() {
    word_extractor.flush();
}

void POCSAGProcessor::reset() {
    bits.reset();
    bit_extractor.reset();
    word_extractor.reset();
    samples_processed = 0;
}

void POCSAGProcessor::send_stats() const {
    POCSAGStatsMessage message(
        word_extractor.current(), word_extractor.count(),
        word_extractor.has_sync(), bit_extractor.baud_rate());
    shared_memory.application_queue.push(message);
}

void POCSAGProcessor::send_packet() {
    packet.set_flag(pocsag::PacketFlag::NORMAL);
    packet.set_timestamp(Timestamp::now());
    packet.set_bitrate(bit_extractor.baud_rate());
    packet.set(word_extractor.batch());

    POCSAGPacketMessage message(packet);
    shared_memory.application_queue.push(message);
}

/* main **************************************************/

int main() {
    EventDispatcher event_dispatcher{std::make_unique<POCSAGProcessor>()};
    event_dispatcher.run();
    return 0;
}
