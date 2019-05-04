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

#include "debug.hpp"

#include <ch.h>
#include <hal.h>

#include "portapack_shared_memory.hpp"

#if defined(LPC43XX_M0)
static void debug_indicate_error_init() {
	// TODO: Get knowledge of LED GPIO port/bit from shared place.
	LPC_GPIO->CLR[2] = (1 << 2);
}

static void debug_indicate_error_update() {
	// Flash RX (yellow) LED to indicate baseband error.
	// TODO: Get knowledge of LED GPIO port/bit from shared place.
	LPC_GPIO->NOT[2] = (1 << 2);
}
#endif

#if defined(LPC43XX_M4)
static void debug_indicate_error_init() {
	// TODO: Get knowledge of LED GPIO port/bit from shared place.
	LPC_GPIO->CLR[2] = (1 << 8);
}

static void debug_indicate_error_update() {
	// Flash TX (red) LED to indicate baseband error.
	// TODO: Get knowledge of LED GPIO port/bit from shared place.
	LPC_GPIO->NOT[2] = (1 << 8);
}
#endif

static void runtime_error() {
	debug_indicate_error_init();

	while(true) {
		volatile size_t n = 1000000U;
		while(n--);
		debug_indicate_error_update();
	}
}

extern "C" {

void port_halt(void) {
	// Copy debug panic message to M0 region.
	const auto* p = dbg_panic_msg;
	for(size_t i=0; i<sizeof(shared_memory.m4_panic_msg); i++) {
		if( *p == 0 ) {
			shared_memory.m4_panic_msg[i] = 0;
		} else {
			shared_memory.m4_panic_msg[i] = *(p++);
		}
	}

	port_disable();
	runtime_error();
}

#if defined(LPC43XX_M4)
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
#endif

}
