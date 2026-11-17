/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "buffer_exchange.hpp"

BufferExchange* BufferExchange::obj{nullptr};

BufferExchange::BufferExchange(
    CaptureConfig* const config, BufferExchange*& owner)
    : owner{owner} {
    // In capture mode, baseband wants empty buffers, app waits for full buffers
    fifo_buffers_for_baseband = config->fifo_buffers_empty;
    fifo_buffers_for_application = config->fifo_buffers_full;
    chSysLock();
    obj = this;
    owner = this;
    chSysUnlock();
}

BufferExchange::BufferExchange(
    ReplayConfig* const config, BufferExchange*& owner)
    : owner{owner} {
    // In replay mode, baseband wants full buffers, app waits for empty buffers
    fifo_buffers_for_baseband = config->fifo_buffers_full;
    fifo_buffers_for_application = config->fifo_buffers_empty;
    chSysLock();
    obj = this;
    owner = this;
    chSysUnlock();
}

BufferExchange::~BufferExchange() {
    chSysLock();
    owner = nullptr;
    if (obj == this) {
        obj = nullptr;
    }
    chSysUnlock();
}

StreamBuffer* BufferExchange::get(FIFO<StreamBuffer*>* fifo, bool stop_on_terminate) {
    while (true) {
        StreamBuffer* p{nullptr};
        fifo->out(p);

        if (p) {
            return p;
        }

        // Check again under the lock before sleeping, so an IRQ cannot be lost.
        chSysLock();
        if (stop_on_terminate && chThdShouldTerminate()) {
            chSysUnlock();
            return nullptr;
        }
        if (!fifo->is_empty()) {
            chSysUnlock();
            continue;
        }
        thread = chThdSelf();
        chSchGoSleepS(THD_STATE_SUSPENDED);
        chSysUnlock();
    }
}

#if defined(LPC43XX_M0)
void BufferExchange::terminate_and_wake(Thread* target, BufferExchange*& owner) {
    chThdTerminate(target);
    chSysLock();
    if (owner && owner->thread == target) {
        owner->wakeup_isr();
        chSchRescheduleS();
    }
    chSysUnlock();
}
#endif

StreamBuffer* BufferExchange::get_prefill(FIFO<StreamBuffer*>* fifo) {
    StreamBuffer* p{nullptr};
    fifo->out(p);

    return p;
}
