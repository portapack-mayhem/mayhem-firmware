/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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
    Creator: @htotoo
*/

#ifndef __PROC_PROTOVIEW_H__
#define __PROC_PROTOVIEW_H__

#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"

class ProtoViewProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    size_t baseband_fs = 0;  // will be set later by configure message.
    uint32_t nsPerDecSamp = 0;

    /* Array Buffer aux. used in decim0 and decim1 IQ c16 signed  data ; (decim0 defines the max length of the array) */
    std::array<complex16_t, 512> dst{};  // decim0 /4 ,  2048/4 = 512 complex I,Q
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    /* Decimates */
    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    dsp::decimate::FIRC16xR16x16Decim2 decim_1{};

    uint32_t currentDuration = 0;
    uint32_t threshold = 0x0630;  // will overwrite after the first iteration

    bool currentHiLow = false;
    bool configured{false};

    // for threshold
    uint32_t cnt = 0;
    uint32_t tm = 0;

    void configure(const SubGhzFPRxConfigureMessage& message);
    void on_beep_message(const AudioBeepMessage& message);

    ProtoViewDataMessage message = {};

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_PROTOVIEW_H__*/
