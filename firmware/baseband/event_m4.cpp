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

#include "ch.h"
#include "debug.hpp"
#include "event_m4.hpp"
#include "meteor_lrpt_m0_configure_ipc.hpp"
#include "lpc43xx_cpp.hpp"
#include "message_queue.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <array>

using namespace lpc43xx;

extern "C" {

CH_IRQ_HANDLER(MAPP_IRQHandler) {
    CH_IRQ_PROLOGUE();

    chSysLockFromIsr();
    EventDispatcher::events_flag_isr(EVT_MASK_BASEBAND);
    chSysUnlockFromIsr();

    creg::m0apptxevent::clear();

    CH_IRQ_EPILOGUE();
}
}

Thread* EventDispatcher::thread_event_loop = nullptr;

EventDispatcher::EventDispatcher(
    std::unique_ptr<BasebandProcessor> baseband_processor)
    : owned_baseband_processor{std::move(baseband_processor)},
      baseband_processor{owned_baseband_processor.get()} {
}

EventDispatcher::EventDispatcher(BasebandProcessor& baseband_processor)
    : baseband_processor{&baseband_processor} {
}

void EventDispatcher::run() {
    thread_event_loop = chThdSelf();

    lpc43xx::creg::m0apptxevent::enable();

    shared_memory.baseband_message = nullptr;
    __DMB();

    // Indicate to the M0 thread that
    // M4 is ready to receive message events.
    shared_memory.set_baseband_ready();

    while (is_running) {
        meteor_lrpt_rx_poll_m0_configure(baseband_processor);
        const auto events = chEvtWaitAnyTimeout(ALL_EVENTS, MS2ST(5));
        dispatch(events);
        if (shared_memory.baseband_message)
            handle_baseband_queue();
    }

    lpc43xx::creg::m0apptxevent::disable();
}

void EventDispatcher::request_stop() {
    is_running = false;
}

eventmask_t EventDispatcher::wait() {
    return chEvtWaitAny(ALL_EVENTS);
}

void EventDispatcher::dispatch(const eventmask_t events) {
    if (events & EVT_MASK_BASEBAND) {
        meteor_lrpt_rx_poll_m0_configure(baseband_processor);
        handle_baseband_queue();
    }

    if (events & EVT_MASK_SPECTRUM) {
        handle_spectrum();
    }
}

void EventDispatcher::handle_baseband_queue() {
    __DMB();
    const auto message = shared_memory.baseband_message;
    if (message) {
        on_message(message);
    }
}

void EventDispatcher::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::Shutdown:
            on_message_shutdown(*reinterpret_cast<const ShutdownMessage*>(message));
#ifdef PRALINE
            shared_memory.baseband_message = nullptr;  // Must clear before M4 exits!
#endif
            break;

        default:
            on_message_default(message);
            __DMB();
            shared_memory.baseband_message = nullptr;
            break;
    }
}

void EventDispatcher::on_message_shutdown(const ShutdownMessage&) {
    request_stop();
}

void EventDispatcher::on_message_default(const Message* const message) {
    baseband_processor->on_message(message);
}

void EventDispatcher::handle_spectrum() {
    const UpdateSpectrumMessage message;
    baseband_processor->on_message(&message);
}
