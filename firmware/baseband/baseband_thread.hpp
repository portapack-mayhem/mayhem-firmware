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

#ifndef __BASEBAND_THREAD_H__
#define __BASEBAND_THREAD_H__

#include "thread_base.hpp"
#include "message.hpp"
#include "baseband_processor.hpp"

#include <ch.h>

/* NB: Because ThreadBase threads start when then are initialized (by default),
 * they should be the last members in a Processor class to ensure the rest of the
 * members are fully initialized before data handling starts. If the Procressor
 * needs to do additional initialization (in its ctor), set 'auto_start' to false
 * and manually call 'start()' on the thread. */

class BasebandThread : public ThreadBase {
   public:
    BasebandThread(
        uint32_t sampling_rate,
        BasebandProcessor* const baseband_processor,
        baseband::Direction direction,
        bool auto_start = true,
        tprio_t priority = (NORMALPRIO + 20));
    ~BasebandThread();

    BasebandThread(const BasebandThread&) = delete;
    BasebandThread(BasebandThread&&) = delete;
    BasebandThread& operator=(const BasebandThread&) = delete;
    BasebandThread& operator=(BasebandThread&&) = delete;

    void start() override;

    // This getter should die, it's just here to leak information to code that
    // isn't in the right place to begin with.
    baseband::Direction direction() const {
        return direction_;
    }

    void set_sampling_rate(uint32_t new_sampling_rate);

   private:
    static Thread* thread;

    BasebandProcessor* baseband_processor_;
    baseband::Direction direction_;
    uint32_t sampling_rate_;
    const tprio_t priority_;

    void run() override;
};

#endif /*__BASEBAND_THREAD_H__*/
