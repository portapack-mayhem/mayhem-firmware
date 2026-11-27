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

#include "lpc43xx_cpp.hpp"

#ifdef BASEBAND_dab_rx
#include "memory_map.hpp"
#endif
#include "portapack_shared_memory.hpp"
#include "portapack_dma.hpp"

#include "gpdma.hpp"
#include "audio_dma.hpp"

static void init() {
    // Audio DMA initialization was moved to baseband proc's that actually use DMA audio, to save memory.
    nvicEnableVector(DMA_IRQn, CORTEX_PRIORITY_MASK(LPC_DMA_IRQ_PRIORITY));
}

static void halt() {
    port_disable();
    while (true) {
        port_wait_for_interrupt();
    }
}

extern "C" {

#ifdef BASEBAND_dab_rx
void __early_init(void) {
    /* First thing the M4 runs, before crt0 copies .data, so the shared_memory
     * reference (itself in .data) cannot be used; the region base is
     * constexpr. A stage still at 0 means the M4 never executed at all. */
    reinterpret_cast<SharedMemory*>(portapack::memory::map::shared_memory.base())
        ->set_m4_boot_stage(M4BootStage::reset);
}
#endif

void __late_init(void) {
#ifdef BASEBAND_dab_rx
    shared_memory.set_m4_boot_stage(M4BootStage::segments);
#endif

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */
    halInit();
#ifdef BASEBAND_dab_rx
    shared_memory.set_m4_boot_stage(M4BootStage::hal);
#endif

    /* After this call, scheduler, systick, heap, etc. are available. */
    /* By doing chSysInit() here, it runs before C++ constructors, which may
     * require the heap.
     */
    chSysInit();
#ifdef BASEBAND_dab_rx
    shared_memory.set_m4_boot_stage(M4BootStage::kernel);
#endif

    /* Baseband initialization */
    init();
}

void _default_exit(void) {
    // TODO: Is this complete?

    audio::dma::disable();

    nvicDisableVector(DMA_IRQn);

    chSysDisable();

    systick_stop();

    ShutdownMessage shutdown_message;
    shared_memory.application_queue.push(shutdown_message);

    shared_memory.baseband_message = nullptr;

    halt();
}
}
