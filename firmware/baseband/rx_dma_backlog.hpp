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

#ifndef RX_DMA_BACKLOG_HPP
#define RX_DMA_BACKLOG_HPP

#include <cstdint>

namespace baseband {

// Four cyclic DMA blocks: one is being written, at most three are readable.
// Caller serializes all access between the ISR and the processing thread.

class RxDmaBacklog {
   public:
    struct Block {
        uint32_t sequence;
        uint32_t index;
    };

    void reset() {
        produced_ = consumed_ = missed_ = 0;
        newest_ = 3;
    }

    void complete(uint32_t index) {
        // Also account for adjacent completions coalesced into one IRQ.
        const uint32_t advance = (index - newest_) & 3u;
        produced_ += advance ? advance : 4u;
        newest_ = index;
    }

    bool take(Block& block) {
        uint32_t available = produced_ - consumed_;

        if (!available) return false;

        if (available > 3) {
            missed_ += available - 3;
            consumed_ = produced_ - 3;
            available = 3;
        }
        block = {++consumed_, (newest_ - (available - 1u)) & 3u};
        return true;
    }

    bool readable(const Block& block) const {
        return produced_ - block.sequence < 3u;
    }

    void discard_copy() { ++missed_; }
    uint32_t missed() const { return missed_; }

   private:
    uint32_t produced_ = 0;
    uint32_t consumed_ = 0;
    uint32_t missed_ = 0;
    uint32_t newest_ = 3;
};
}  // namespace baseband

#endif
