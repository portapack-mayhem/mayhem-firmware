/*
 * Copyright (C) 2025 StarVore Labs
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

#ifndef __PROC_SSTV_RX__
#define __PROC_SSTV_RX__

#include "portapack_shared_memory.hpp"
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "sstv.hpp"

using namespace sstv;

class SSTVRXProcessor : public BasebandProcessor {
    public:
     void execute(const buffer_c8_t& buffer) override;
     void on_message(const Message* const p) override;

    private:
     enum state_t {
        STATE_CALIBRATION = 0,
        STATE_VIS,
        STATE_SYNC,
        STATE_PIXELS
     };

     state_t state{};

     bool configured{false};

     int8_t re{}, im{};

     RequestSignalMessage sig_message{RequestSignalMessage::Signal::FillRequest};

     /* NB: Threads should be the last members in the class definition. */
     BasebandThread baseband_thread{307200, this, baseband::Direction::Receive};
};

#endif