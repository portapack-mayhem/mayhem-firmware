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

void write_m4_panic_msg(const char *panic_message, struct extctx *ctxp) {
    if (ctxp == nullptr) {
        shared_memory.bb_data.data[0] = 0;
    }
    else {
        shared_memory.bb_data.data[0] = 1;
        memcpy(&shared_memory.bb_data.data[4], ctxp, sizeof(struct extctx));
    }

	for(size_t i=0; i<sizeof(shared_memory.m4_panic_msg); i++) {
		if( *panic_message == 0 ) {
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
    asm volatile ("mov     %0, lr" : "=r" (_saved_lr) : : "memory");
	CH_IRQ_PROLOGUE();

    struct extctx *ctxp;
    port_lock_from_isr();

    if ((uint32_t)_saved_lr & 0x04)
        ctxp = (struct extctx *)__get_PSP();
    else
        ctxp = (struct extctx *)__get_MSP();

    write_m4_panic_msg("Hard Fault", ctxp);

	port_disable();
    while (true);

    CH_IRQ_EPILOGUE();
#else
	chSysHalt();
#endif
}

}

