/*
 * ------------------------------------------------------------
 * |  Made by RocketGod                                       |
 * |  Find me at https://betaskynet.com                       |
 * |  Argh matey!                                             |
 * ------------------------------------------------------------
 */

#include "proc_flex.hpp"
#include "event_m4.hpp"
#include "audio_dma.hpp"
#include "dsp_fir_taps.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <memory>

namespace {

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

FlexProcessor::FlexProcessor()
    : BasebandProcessor(),
      state(State::Idle),
      sync_buffer(0),
      frame_buffer(),
      current_word(0),
      bit_count(0),
      current_baud_rate(1600) {
}

void FlexAudioNormalizer::execute_in_place(const buffer_f32_t& audio) {
    if (counter_ >= 24'000) {
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

        if (val >= t_high_)
            val = 1.0f;
        else if (val >= t_mid_high_)
            val = 0.33f;
        else if (val <= t_low_)
            val = -1.0f;
        else if (val <= t_mid_low_)
            val = -0.33f;
        else
            val = 0.0f;
    }
}

void FlexAudioNormalizer::calculate_thresholds() {
    auto center = (max_ + min_) / 2.0f;
    auto range = (max_ - min_) / 2.0f;
    auto step = range / 3.0f;

    t_high_ = center + step * 1.5f;
    t_mid_high_ = center + step * 0.5f;
    t_mid_low_ = center - step * 0.5f;
    t_low_ = center - step * 1.5f;
}

void FlexBitQueue::push(uint8_t symbol) {
    data_ = (data_ << 2) | (symbol & 0x3);
    if (count_ < max_size_) ++count_;
}

bool FlexBitQueue::pop(uint8_t& symbol) {
    if (count_ == 0) return false;

    --count_;
    symbol = (data_ >> (count_ * 2)) & 0x3;
    return true;
}

void FlexBitQueue::reset() {
    data_ = 0;
    count_ = 0;
}

uint8_t FlexBitQueue::size() const {
    return count_;
}

uint32_t FlexBitQueue::data() const {
    return data_;
}

void FlexBitExtractor::extract_bits(const buffer_f32_t& audio) {
    for (size_t i = 0; i < audio.count; ++i) {
        auto sample = audio.p[i];

        if (current_rate_) {
            if (current_rate_->handle_sample(sample)) {
                uint8_t symbol;
                if (sample >= 0.66f)
                    symbol = 0x0;
                else if (sample >= 0.0f)
                    symbol = 0x1;
                else if (sample >= -0.66f)
                    symbol = 0x2;
                else
                    symbol = 0x3;
                bits_.push(symbol);
            }
        } else {
            for (auto& rate : known_rates_) {
                if (rate.handle_sample(sample) &&
                    diff_bit_count(rate.bits.data(), sync1_pattern) <= 4) {
                    rate.is_stable = true;
                    current_rate_ = &rate;
                }
            }
        }
    }
}

void FlexBitExtractor::configure(uint32_t sample_rate) {
    sample_rate_ = sample_rate;
    for (auto& rate : known_rates_) {
        rate.sample_interval = sample_rate / static_cast<float>(rate.baud_rate);
    }
}

void FlexBitExtractor::reset() {
    current_rate_ = nullptr;
    for (auto& rate : known_rates_)
        rate.reset();
}

uint16_t FlexBitExtractor::baud_rate() const {
    return current_rate_ ? current_rate_->baud_rate : 0;
}

bool FlexBitExtractor::RateInfo::handle_sample(float sample) {
    samples_until_next -= 1;
    if (samples_until_next > 0) return false;

    bool bit_pushed = false;
    float delta = std::abs(sample - prev_value);

    switch (state) {
        case State::WaitForSample:
            state = State::ReadyToSend;
            break;

        case State::ReadyToSend:
            if (!is_stable && delta > 0.33f) {
                samples_until_next += (sample_interval / 8.0f);
            } else {
                state = State::WaitForSample;
                bit_pushed = true;
                bits.push(sample >= 0.0f ? (sample >= 0.66f ? 0x0 : 0x1) : (sample >= -0.66f ? 0x2 : 0x3));
            }
            break;
    }

    samples_until_next += sample_interval;
    prev_value = sample;
    return bit_pushed;
}

void FlexBitExtractor::RateInfo::reset() {
    state = State::WaitForSample;
    samples_until_next = 0.0f;
    prev_value = 0.0f;
    is_stable = false;
    bits.reset();
}

void FlexCodewordExtractor::process_bits() {
    uint8_t symbol;
    while (bits_.pop(symbol)) {
        data_ = (data_ << 2) | symbol;
        bit_count_ += 2;

        if (bit_count_ >= data_bit_count) {
            if (!has_sync_) {
                if (diff_bit_count(data_, sync1_codeword) <= 4) {
                    handle_sync();
                }
            } else {
                save_current_codeword();
            }
        }
    }
}

void FlexCodewordExtractor::flush() {
    if (word_count_ == 0) return;
    pad_idle();
    handle_batch_complete();
}

void FlexCodewordExtractor::reset() {
    clear_data_bits();
    has_sync_ = false;
    word_count_ = 0;
}

void FlexCodewordExtractor::clear_data_bits() {
    data_ = 0;
    bit_count_ = 0;
}

void FlexCodewordExtractor::take_one_symbol() {
    uint8_t symbol;
    if (bits_.pop(symbol)) {
        data_ = (data_ << 2) | symbol;
        if (bit_count_ < data_bit_count)
            bit_count_ += 2;
    }
}

void FlexCodewordExtractor::handle_sync() {
    clear_data_bits();
    has_sync_ = true;
    word_count_ = 0;
}

void FlexCodewordExtractor::save_current_codeword() {
    batch_[word_count_++] = data_;
    clear_data_bits();
    
    if (word_count_ >= flex_batch_size) {
        handle_batch_complete();
    }
}

void FlexCodewordExtractor::handle_batch_complete() {
    on_batch_(*this);
    has_sync_ = false;
    word_count_ = 0;
}

void FlexCodewordExtractor::pad_idle() {
    while (word_count_ < flex_batch_size) {
        batch_[word_count_++] = 0xAAAAAAAA;
    }
}

void FlexProcessor::execute(const buffer_c8_t& buffer) {
    if (!configured) return;

    const auto decim_0_out = decim_0.execute(buffer, dst_buffer);
    const auto decim_1_out = decim_1.execute(decim_0_out, dst_buffer);
    const auto channel_out = channel_filter.execute(decim_1_out, dst_buffer);
    auto audio = demod.execute(channel_out, audio_buffer);

    squelch.set_threshold(0.01);
    squelch_history = (squelch_history << 1) | (squelch.execute(audio) ? 1 : 0);

    lpf.execute_in_place(audio);
    normalizer.execute_in_place(audio);
    audio_output.write(audio);

    bit_extractor.extract_bits(audio);
    word_extractor.process_bits();

    static uint64_t sync_buffer64 = 0;
    static uint32_t block_buffer[8] = {0};
    static uint8_t word_count = 0;
    static uint8_t bit_count = 0;

    for (size_t i = 0; i < audio.count; ++i) {
        float sample = audio.p[i];
        
        sync_buffer64 = (sync_buffer64 << 2);
        if (sample >= 0.66f)
            sync_buffer64 |= 0x0;
        else if (sample >= 0.0f)
            sync_buffer64 |= 0x1;
        else if (sample >= -0.66f)
            sync_buffer64 |= 0x2;
        else
            sync_buffer64 |= 0x3;

        switch (state) {
            case State::Idle:
                if (squelch.execute(audio)) {
                    uint32_t sync1 = (sync_buffer64 >> 32) & 0xFFFF;
                    uint32_t sync2 = sync_buffer64 & 0xFFFF;
                    if (diff_bit_count(sync1, 0xA6C6) <= 2 && diff_bit_count(sync2, 0xAAAA) <= 2) {
                        uint32_t sync_pre = (sync_buffer64 >> 48) & 0xFFFF;
                        uint32_t sync_post = (sync_buffer64 >> 16) & 0xFFFF;
                        if (diff_bit_count(sync_pre, 0x870C) <= 2 && diff_bit_count(sync_post, 0x78F3) <= 2) {
                            current_baud_rate = 1600;
                            state = State::FrameData;
                            word_count = 0;
                            bit_count = 0;
                        } else if (diff_bit_count(sync_pre, 0xB068) <= 2 && diff_bit_count(sync_post, 0x4F97) <= 2) {
                            current_baud_rate = 3200;
                            state = State::FrameData;
                            word_count = 0;
                            bit_count = 0;
                        } else if (diff_bit_count(sync_pre, 0xDEA0) <= 2 && diff_bit_count(sync_post, 0x215F) <= 2) {
                            current_baud_rate = 6400;
                            state = State::FrameData;
                            word_count = 0;
                            bit_count = 0;
                        }
                    }
                }
                break;

            case State::FrameData:
                block_buffer[word_count] = (block_buffer[word_count] << 2);
                if (sample >= 0.66f)
                    block_buffer[word_count] |= 0x0;
                else if (sample >= 0.0f)
                    block_buffer[word_count] |= 0x1;
                else if (sample >= -0.66f)
                    block_buffer[word_count] |= 0x2;
                else
                    block_buffer[word_count] |= 0x3;
                
                bit_count += 2;
                
                if (bit_count >= 32) {
                    word_count++;
                    bit_count = 0;
                    if (word_count >= 8) {
                        packet.set_flag(FlexPacketFlag::FLEX_NORMAL);
                        packet.set_timestamp(Timestamp::now());
                        packet.set_bitrate(current_baud_rate);
                        flex_batch_t batch{};
                        for (size_t j = 0; j < 8; j++) {
                            batch[j] = block_buffer[j];
                        }
                        packet.set(batch);
                        FlexPacketMessage message(packet);
                        shared_memory.application_queue.push(message);
                        word_count = 0;
                        state = State::Idle; // Reset to find next sync
                    }
                }
                break;

            default:
                state = State::Idle;
                break;
        }
    }

    samples_processed += buffer.count;
    if (samples_processed >= stat_update_threshold) {
        send_stats();
        samples_processed -= stat_update_threshold;
    }
}

void FlexProcessor::process_frame() {
    // No longer used; packet sent directly in execute()
}

void FlexProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::FlexConfigure:
            configure();
            break;

        case Message::ID::NBFMConfigure:
            squelch.set_threshold(0.01);
            break;

        case Message::ID::AudioBeep:
            on_beep_message(*reinterpret_cast<const AudioBeepMessage*>(message));
            break;

        default:
            break;
    }
}

void FlexProcessor::configure() {
    constexpr size_t decim_0_output_fs = baseband_fs / decim_0.decimation_factor;
    constexpr size_t decim_1_output_fs = decim_0_output_fs / decim_1.decimation_factor;
    constexpr size_t channel_filter_output_fs = decim_1_output_fs / 2;
    constexpr size_t demod_input_fs = channel_filter_output_fs;

    decim_0.configure(taps_200k_decim_0.taps);
    decim_1.configure(taps_16k0_decim_1.taps);
    channel_filter.configure(taps_16k0_channel.taps, 2);
    
    demod.configure(demod_input_fs, 4800);

    audio_output.configure(false);
    bit_extractor.configure(demod_input_fs);

    configured = true;
}

void FlexProcessor::flush() {
    word_extractor.flush();
}

void FlexProcessor::reset() {
    state = State::Idle;
    sync_buffer = 0;
    frame_buffer.clear();
    current_word = 0;
    bit_count = 0;
    
    bits.reset();
    bit_extractor.reset();
    word_extractor.reset();
    samples_processed = 0;
}

void FlexProcessor::send_stats() const {
    FlexStatsMessage message(
        current_word,
        frame_buffer.size(),
        state != State::Idle,
        current_baud_rate);
    shared_memory.application_queue.push(message);
}

void FlexProcessor::send_packet() {
    packet.set_flag(FlexPacketFlag::FLEX_NORMAL);
    packet.set_timestamp(Timestamp::now());
    packet.set_bitrate(bit_extractor.baud_rate());
    packet.set(word_extractor.batch());

    FlexPacketMessage message(packet);
    shared_memory.application_queue.push(message);
}

void FlexProcessor::on_beep_message(const AudioBeepMessage& message) {
    audio::dma::beep_start(message.freq, message.sample_rate, message.duration_ms);
}

int main() {
    audio::dma::init_audio_out();
    EventDispatcher event_dispatcher{std::make_unique<FlexProcessor>()};
    event_dispatcher.run();
    return 0;
}