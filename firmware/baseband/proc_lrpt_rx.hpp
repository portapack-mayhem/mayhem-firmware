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

#ifndef __PROC_LRPT_RX_H__
#define __PROC_LRPT_RX_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "lrpt_demod.hpp"
#include "lrpt_shared.hpp"

#include "message.hpp"

// Meteor-M LRPT, 72k or 80k: CADUs out to the M0 through a ring of slots in M4 RAM. The M0
// asks for the mode through the shared buffer; the demodulator is built again for it.
class LrptRxProcessor : public BasebandProcessor {
   public:
    LrptRxProcessor();
    LrptRxProcessor(const LrptRxProcessor&) = delete;
    LrptRxProcessor& operator=(const LrptRxProcessor&) = delete;

    void execute(const buffer_c8_t& buffer) override;

   private:
    // 2304 (2560) kS/s in, 288 (320) kS/s to the demodulator, 4 samples a symbol
    static constexpr size_t baseband_fs = lrpt::baseband_rate(lrpt::Mode::M72K);

    // a buffer of 2048 samples, decimated by 8
    std::array<complex16_t, 256> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};

    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    lrpt::Mode mode_{lrpt::Mode::M72K};
    alignas(lrpt::Demodulator) uint8_t demod_storage_[sizeof(lrpt::Demodulator)];
    lrpt::Demodulator* demod_{nullptr};

    uint8_t next_slot_{0};
    uint32_t frames_{0};
    uint32_t overruns_{0};
    size_t status_count_{0};
    LrptRxStatusMessage status_message_{};

    static void on_frame(void* context, const uint8_t* frame);
    static void on_block(void* context);
    void start_demod();
    void send_status();

    /* NB: Threads should be the last members in the class definition. */
    // Started by the constructor once the filter is configured.
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive, false};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_LRPT_RX_H__*/
