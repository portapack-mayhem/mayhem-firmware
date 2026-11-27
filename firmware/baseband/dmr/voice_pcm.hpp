/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#ifndef DMR_VOICE_PCM_HPP
#define DMR_VOICE_PCM_HPP

#include <cstdint>

// Caller serializes main-thread pushes/reset with audio ISR reads.

class DmrVoicePcm {
   public:
    static constexpr unsigned capacity = 640;
    explicit DmrVoicePcm(int16_t* storage)
        : storage_{storage} {}
    unsigned free() const { return capacity - count_; }
    bool has_audio() const { return count_ || playing_ || previous_ || next_; }
    void reset() {
        head_ = tail_ = count_ = phase_ = consumed_ = 0;
        previous_ = next_ = 0;
        playing_ = wake_ = false;
    }
    bool push(const int16_t* samples, unsigned count) {
        if (count > free()) return false;

        for (unsigned i = 0; i < count; ++i) {
            storage_[head_] = samples[i];
            if (++head_ == capacity) head_ = 0;
        }
        count_ += count;
        return true;
    }

    int16_t sample24k() {
        if (phase_ == 0) {
            if (!playing_ && count_ == capacity) playing_ = true;

            previous_ = next_;

            if (playing_ && count_) {
                next_ = storage_[tail_];

                if (++tail_ == capacity) tail_ = 0;

                --count_;

                if (++consumed_ == 160) {
                    consumed_ = 0;
                    wake_ = true;
                }
            } else {
                next_ = 0;
                playing_ = false;
            }
        }

        const int16_t value = previous_ + (next_ - previous_) * static_cast<int32_t>(phase_ + 1) / 3;

        if (++phase_ == 3) phase_ = 0;
        return value;
    }

    bool take_wakeup() {
        const bool result = wake_;

        wake_ = false;
        return result;
    }

   private:
    int16_t* const storage_;
    unsigned head_ = 0, tail_ = 0, count_ = 0, phase_ = 0, consumed_ = 0;
    int32_t previous_ = 0, next_ = 0;
    bool playing_ = false, wake_ = false;
};
#endif
