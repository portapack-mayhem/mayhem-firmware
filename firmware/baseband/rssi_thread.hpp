/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __RSSI_THREAD_H__
#define __RSSI_THREAD_H__

#include "thread_base.hpp"

#include <ch.h>
#include <cstdint>

/* NB: Because ThreadBase threads start when then are initialized (by default),
 * they should be the last members in a Processor class to ensure the rest of the
 * members are fully initialized before data handling starts. If the Procressor
 * needs to do additional initialization (in its ctor), set 'auto_start' to false
 * and manually call 'start()' on the thread.
 * This isn't as relevant for RSSIThread which is entirely self-contained, but
 * it's good practice to keep all the thread-init together. */

class RSSIThread : public ThreadBase {
   public:
    RSSIThread(
        bool auto_start = true,
        tprio_t priority = (NORMALPRIO + 10));
    ~RSSIThread();

    void start() override;

   private:
    void run() override;

    const tprio_t priority_;

    static Thread* thread;
    static constexpr uint32_t sampling_rate{400000};
};

#endif /*__RSSI_THREAD_H__*/
