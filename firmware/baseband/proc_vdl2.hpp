/*
 * Copyright (C) 2026 Xmoo26
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

#ifndef __PROC_VDL2_H__
#define __PROC_VDL2_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"

#include "dsp_decimate.hpp"
#include "vdl2_decoder.hpp"

#include "message.hpp"

// VHF Data Link mode 2: one 25 kHz channel, AVLC frames out.
class VDL2Processor : public BasebandProcessor {
   public:
    VDL2Processor();

    void execute(const buffer_c8_t& buffer) override;

   private:
    static constexpr size_t baseband_fs = 3072000;

    std::array<complex16_t, 512> dst{};
    const buffer_c16_t dst_buffer{dst.data(), dst.size()};
    std::array<complex16_t, 64> channel{};
    const buffer_c16_t channel_buffer{channel.data(), channel.size()};

    // 3072 -> 384 -> 48 kHz, then the channel: flat to 10.5 kHz (8.4 kHz of signal and room
    // for a tuning error), down 70 dB from 16 kHz.
    dsp::decimate::FIRC8xR16x24FS4Decim8 decim_0{};
    dsp::decimate::FIRC16xR16x32Decim8 decim_1{};
    dsp::decimate::FIRAndDecimateComplex channel_filter{};

    vdl2::BurstDecoder burst{on_frame, this};
    vdl2::Demodulator demod{burst};
    VDL2FrameMessage frame_message{};

    static void on_frame(void* context, const uint8_t* frame, size_t length);

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_VDL2_H__*/
