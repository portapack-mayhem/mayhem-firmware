/*
 * Copyright (C) 2024 HTotoo
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

/*
    For usage check the FlipperTX app. Don't forget to "close" the tx by sending 42069, 613379 values at the end of the stream.
    Only close baseband when you get the TX done msg, not on replay thread done, because, there can still data in the buffer.
*/

#ifndef __PROC_OOKSTREAMTX_HPP__
#define __PROC_OOKSTREAMTX_HPP__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "stream_output.hpp"
#include <array>
#include <memory>

#define OOK_SAMPLERATE 2280000U

class OOKProcessorStreamed : public BasebandProcessor {
   public:
    OOKProcessorStreamed();

    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    uint32_t rem_samples = 0;
    uint32_t curr_samples = 0;
    bool curr_hilow = false;

    uint32_t phase{0}, sphase{0};
    void write_sample(const buffer_c8_t& buffer, bool bit_value, size_t i);

    std::unique_ptr<StreamOutput> stream{};
    bool configured{false};
    void replay_config(const ReplayConfigMessage& message);

    int32_t endsignals[3] = {0, 42069, 613379};  // 0 is skipped, count from 1, don't ask...
    uint8_t readerrs = 0;                        // to count in the array

    TXProgressMessage txprogress_message{};
    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{OOK_SAMPLERATE, this, baseband::Direction::Transmit};
};

#endif /*__PROC_OOKSTREAMTX_HPP__*/
