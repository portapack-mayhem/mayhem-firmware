/*
 * Copyright (C) 2026 zxkmm
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

#ifndef __PROC_SPEC_AN_H__
#define __PROC_SPEC_AN_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"

#include "message.hpp"
#include "spec_an_shared.hpp"
#include "spec_an_dsp.hpp"

#include <array>
#include <cstdint>
#include <cstddef>

class SpecAnProcessor : public BasebandProcessor {
   public:
    SpecAnProcessor();
    SpecAnProcessor(const SpecAnProcessor&) = delete;
    SpecAnProcessor& operator=(const SpecAnProcessor&) = delete;

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    /* A capture handed from execute() (baseband thread) to the FFT worker
     * (event thread). `full` is the only handshake: set last by the producer,
     * cleared by the consumer once it has copied the samples out. */
    struct Slot {
        uint32_t seq;
        uint16_t slice;
        uint8_t sweep;
        uint16_t blocks;
        bool last; /* completes the slice's average */
        volatile bool full;
    };

    /* Configuration (event thread writes, baseband thread reads only while
     * `configured` is set). */
    volatile bool configured{false};
    spec_an::Shared* shared{nullptr};
    uint16_t gen{0};
    size_t log2n{8};
    size_t n{256};
    size_t blocks_per_buffer{8};
    size_t avg_blocks{1};
    int32_t bin_first{0};
    size_t bins_per_slice{0};
    size_t slices{1};
    spec_an::Window window{spec_an::Window::BlackmanHarris};

    /* Capture state, baseband thread only. */
    uint32_t armed_seq{0};
    uint16_t cap_slice{0};
    uint8_t cap_sweep{0};
    size_t settle_left{0};
    size_t blocks_left{0};
    bool capturing{false};
    bool captured_pending{false};
    uint32_t captured_seq{0};
    size_t slot_wr{0};

    /* Worker state, event thread only. */
    std::array<Slot, 2> slots{};
    size_t slot_rd{0};
    bool acc_valid{false};
    uint32_t acc_seq{0};
    spec_an::dsp::PointDetector detector{};

    void configure(const SpecAnConfigMessage& message);
    void post_captured();
    void process_pending();
    void finish_slice(uint16_t slice, uint8_t sweep);

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{20000000, this, baseband::Direction::Receive};
};

#endif /*__PROC_SPEC_AN_H__*/
