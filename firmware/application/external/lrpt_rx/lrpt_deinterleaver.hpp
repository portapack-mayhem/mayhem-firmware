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

#ifndef __LRPT_DEINTERLEAVER_H__
#define __LRPT_DEINTERLEAVER_H__

// Meteor-M LRPT 80k, M0 side of the deinterleaver: keeps the input blocks of the last 18 s in
// a ring in a file and builds each output block from the blocks its branches are late by.
// The storage is given as two functions, so the same code runs in a PC test harness.

#include <cstddef>
#include <cstdint>

#include "lrpt_shared.hpp"

namespace ui::external_app::lrpt_rx {

class Deinterleaver {
   public:
    // offset and length in bytes; false on an error
    using Write = bool (*)(void* context, uint32_t offset, const uint8_t* data, size_t length);
    using Read = bool (*)(void* context, uint32_t offset, uint8_t* data, size_t length);

    // blocks in the ring: the longest delay and a few to spare
    static constexpr uint32_t RING_BLOCKS = lrpt::DEINT_FILL + 4;
    // a block takes whole sectors in the file
    static constexpr uint32_t FILE_BLOCK = (lrpt::DEINT_BLOCK + 511) / 512 * 512;
    static constexpr uint32_t FILE_BYTES = RING_BLOCKS * FILE_BLOCK;

    Deinterleaver(Write write, Read read, void* context)
        : write_{write}, read_{read}, context_{context} {}

    // does what the M4 is waiting for, cheap when there is nothing to do
    void service(lrpt::Shared* shared);
    void reset() { have_generation_ = false; }

    // blocks on the card since the start, up to a full deinterleaver
    uint32_t fill_percent() const;
    uint32_t errors() const { return errors_; }

   private:
    Write write_;
    Read read_;
    void* context_;

    bool have_generation_{false};
    uint32_t generation_{0};
    uint32_t written_{0};   // input blocks before this one are handled
    uint32_t prepared_{0};  // next output block to build
    uint32_t errors_{0};
    uint8_t valid_[(RING_BLOCKS + 7) / 8]{};

    bool valid(uint32_t seq) const;
    void set_valid(uint32_t seq, bool v);
};

}  // namespace ui::external_app::lrpt_rx

#endif /*__LRPT_DEINTERLEAVER_H__*/
