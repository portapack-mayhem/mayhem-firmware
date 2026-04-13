/*
 * Copyright (C) 2024 Bernd Herzog
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

#include "standalone_heap_thread.hpp"

#include "standalone_app.hpp"

#include "ch.h"

#include <cstdint>

extern const standalone_application_api_t* _api;

namespace {

constexpr cnt_t standalone_reap_queue_depth = 32;
msg_t standalone_reap_mb_buffer[standalone_reap_queue_depth];
MAILBOX_DECL(standalone_reap_mb, standalone_reap_mb_buffer, standalone_reap_queue_depth);

WORKING_AREA(standalone_reaper_wa, 512);
MUTEX_DECL(standalone_reaper_mtx);

bool standalone_reaper_started = false;

msg_t standalone_reaper_fn(void* arg) {
    (void)arg;
    chRegSetThreadName("standalone_reap");
    while (true) {
        msg_t m;
        if (chMBFetch(&standalone_reap_mb, &m, TIME_INFINITE) != RDY_OK)
            continue;
        void* tp = reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(m)));
        _api->thread_wait(tp);
    }
}

void ensure_standalone_reaper(void) {
    chMtxLock(&standalone_reaper_mtx);
    if (!standalone_reaper_started) {
        chThdCreateStatic(standalone_reaper_wa, sizeof(standalone_reaper_wa),
                          NORMALPRIO - 1, standalone_reaper_fn, nullptr);
        standalone_reaper_started = true;
    }
    chMtxUnlock();
}

}  // namespace

void standalone_create_heap_thread(int32_t (*fn)(void*), void* arg, size_t stack_size, int priority) {
    ensure_standalone_reaper();
    void* tp = _api->thread_create_heap(fn, arg, stack_size, priority);
    if (chMBPost(&standalone_reap_mb, static_cast<msg_t>(reinterpret_cast<uintptr_t>(tp)), TIME_INFINITE) != RDY_OK)
        _api->panic("standalone reap queue");
}
