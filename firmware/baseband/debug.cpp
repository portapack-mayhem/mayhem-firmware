/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
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

#include "debug.hpp"

#include <ch.h>
#include <hal.h>

#include "portapack_shared_memory.hpp"
#include "performance_counter.hpp"

void write_m4_panic_msg(const char* panic_message, struct extctx* ctxp) {
    if (ctxp == nullptr) {
        shared_memory.bb_data.data[0] = 0;
    } else {
        shared_memory.bb_data.data[0] = 1;
        *((uint32_t*)&shared_memory.bb_data.data[4]) = SCB->CFSR;
        memcpy(&shared_memory.bb_data.data[8], ctxp, sizeof(struct extctx));
    }

    for (size_t i = 0; i < sizeof(shared_memory.m4_panic_msg); i++) {
        if (*panic_message == 0) {
            shared_memory.m4_panic_msg[i] = 0;
        } else {
            shared_memory.m4_panic_msg[i] = *(panic_message++);
        }
    }
}

extern "C" {
#if CH_DBG_ENABLED
void port_halt(void) {
    port_disable();

    if (dbg_panic_msg == nullptr)
        dbg_panic_msg = "system halted";

    write_m4_panic_msg(dbg_panic_msg, nullptr);

    while (true) {
        HALT_IF_DEBUGGING();
    }
}
#endif

CH_IRQ_HANDLER(MemManageVector) {
#if CH_DBG_ENABLED
    chDbgPanic("MemManage");
#else
    chSysHalt();
#endif
}

CH_IRQ_HANDLER(BusFaultVector) {
#if CH_DBG_ENABLED
    chDbgPanic("BusFault");
#else
    chSysHalt();
#endif
}

CH_IRQ_HANDLER(UsageFaultVector) {
#if CH_DBG_ENABLED
    chDbgPanic("UsageFault");
#else
    chSysHalt();
#endif
}

CH_IRQ_HANDLER(HardFaultVector) {
#if CH_DBG_ENABLED
    regarm_t _saved_lr;
    asm volatile("mov     %0, lr"
                 : "=r"(_saved_lr)
                 :
                 : "memory");

    struct extctx* ctxp;
    port_lock_from_isr();

    if ((uint32_t)_saved_lr & 0x04)
        ctxp = (struct extctx*)__get_PSP();
    else
        ctxp = (struct extctx*)__get_MSP();

    volatile uint32_t stack_space_left = get_free_stack_space();
    if (stack_space_left < 16)
        write_m4_panic_msg("Stack Overflow", ctxp);

    else
        write_m4_panic_msg("Hard Fault", ctxp);

    port_disable();

    while (true) {
        HALT_IF_DEBUGGING();
    }

#else
    chSysHalt();
#endif
}

void update_performance_counters() {
    auto performance_counter_active = shared_memory.request_m4_performance_counter;
    if (performance_counter_active == 0x00)
        return;

    static bool last_paint_state = false;
    if ((((chTimeNow() >> 10) & 0x01) == 0x01) == last_paint_state)
        return;

    // Idle thread state is sometimes unuseable
    if (chThdGetTicks(chSysGetIdleThread()) > 0x10000000)
        return;

    last_paint_state = !last_paint_state;

    if (performance_counter_active == 0x01) {
        auto utilisation = get_cpu_utilisation_in_percent();
        auto free_stack = (uint32_t)get_free_stack_space();
        auto free_heap = chCoreStatus();

        shared_memory.m4_performance_counter = utilisation;
        shared_memory.m4_stack_usage = free_stack;
        shared_memory.m4_heap_usage = free_heap;
    } else if (performance_counter_active == 0x02) {
        shared_memory.m4_performance_counter = 0;
    }
}

} /* extern "C" */
