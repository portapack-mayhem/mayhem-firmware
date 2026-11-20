/*
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

#include "cc1101_decoder.hpp"

namespace cc1101 {

namespace {

constexpr float PLL_GAIN = 0.3f;

}  // namespace

void Decoder::configure(const float sampling_rate) {
    bit_step_ = BAUD / sampling_rate;
    // about two bits: the 0xAA preamble averages out within it
    dc_alpha_ = 1.0f / (2.0f * sampling_rate / BAUD);
    bit_phase_ = 0.0f;
    dc_ = 0.0f;
    state_ = State::Search;
}

void Decoder::execute(const float sample) {
    if (state_ == State::Search)
        dc_ += (sample - dc_) * dc_alpha_;
    const bool level = sample > dc_;

    // Bit clock: transitions belong half a bit from the sampling point.
    if (level != last_level_) {
        bit_phase_ -= (bit_phase_ - 0.5f) * PLL_GAIN;
        last_level_ = level;
    }
    bit_phase_ += bit_step_;
    if (bit_phase_ >= 1.0f) {
        bit_phase_ -= 1.0f;
        bit(level ? 1 : 0);
    }
}

void Decoder::bit(const uint8_t b) {
    shift_ = (shift_ << 1) | b;

    if (state_ == State::Search) {
        // the end of the preamble and the sync word, exactly as the CC1101 wants it (16/16)
        if ((shift_ & 0xFFFF) == SYNC_WORD && ((shift_ >> 16) & 0xFF) == 0xAA) {
            state_ = State::Length;
            bit_count_ = 0;
            byte_ = 0;
        }
        return;
    }

    byte_ = (byte_ << 1) | b;
    if (++bit_count_ < 8)
        return;
    bit_count_ = 0;

    if (state_ == State::Length) {
        length_ = byte_;
        if (length_ == 0 || length_ > MAX_PAYLOAD) {
            state_ = State::Search;
            return;
        }
        buffer_[0] = byte_;
        received_ = 1;
        state_ = State::Data;
        return;
    }

    buffer_[received_++] = byte_;
    if (received_ < length_ + 3)
        return;
    state_ = State::Search;
    const uint16_t crc = ((uint16_t)buffer_[length_ + 1] << 8) | buffer_[length_ + 2];
    if (crc16(buffer_.data(), length_ + 1) == crc) {
        packets_++;
        handler_(context_, buffer_.data() + 1, length_);
    }
}

}  // namespace cc1101
