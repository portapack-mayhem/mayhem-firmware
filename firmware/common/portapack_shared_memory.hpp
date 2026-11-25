/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __PORTAPACK_SHARED_MEMORY_H__
#define __PORTAPACK_SHARED_MEMORY_H__

#include <cstdint>
#include <cstddef>

#include "message_queue.hpp"

struct JammerChannel {
    bool enabled;
    uint64_t center;
    uint32_t width;
    uint32_t duration;
};

struct HopperChannel {
    bool enabled;
    uint64_t center;
    uint32_t width;
    uint32_t duration;
};

struct ToneDef {
    uint32_t delta;
    uint32_t duration;
};

struct ToneData {
    ToneDef tone_defs[32];
    uint32_t silence;
    uint8_t message[256];
};

#ifdef PRALINE
/* How far the M4 got, written at fixed points from the reset vector onwards so
 * an image that never signals ready and never panics can be reported by stage
 * rather than as a bare timeout. */
enum class M4BootStage : uint32_t {
    none = 0,
    reset = 1,         // crt0 reached __early_init: the core is fetching code
    segments = 2,      // .data copied and .bss zeroed: the image can be run
    hal = 3,           // halInit() returned
    kernel = 4,        // chSysInit() returned
    main_entered = 5,  // constructors ran and main() started
    audio_ready = 6,   // init_audio_out() returned
    processor = 7,     // the BasebandProcessor was constructed
    dispatching = 8,   // EventDispatcher::run() reached; ready is set next
};
#endif

/* NOTE: These structures must be located in the same location in both M4 and M0 binaries */
struct SharedMemory {
    static constexpr size_t application_queue_k = 11;
    static constexpr size_t app_local_queue_k = 11;

    uint8_t application_queue_data[1 << application_queue_k]{0};
    uint8_t app_local_queue_data[1 << app_local_queue_k]{0};
    const Message* volatile baseband_message{nullptr};
    MessageQueue application_queue{application_queue_data, application_queue_k};
    MessageQueue app_local_queue{app_local_queue_data, app_local_queue_k};

    char m4_panic_msg[32]{0};

    union {
        ToneData tones_data;
        struct {
            JammerChannel jammer_channels[80];
            HopperChannel hopper_channels[24];
        } dummy_seperate;
        uint8_t data[512];
    } bb_data{{{{0, 0}}, 0, {0}}};

    // Set by the M4 to indicate that the baseband app is ready.
    bool volatile baseband_ready{false};
#ifdef PRALINE
    void clear_baseband_ready() {
        baseband_ready = false;
        m4_boot_stage = 0;
    }
    void set_m4_boot_stage(M4BootStage stage) { m4_boot_stage = static_cast<uint32_t>(stage); }
#else
    void clear_baseband_ready() { baseband_ready = false; }
#endif
    void set_baseband_ready() { baseband_ready = true; }

    uint8_t volatile request_m4_performance_counter{0};
    uint8_t volatile m4_performance_counter{0};
    uint16_t volatile m4_stack_usage{0};
    uint32_t volatile m4_heap_usage{0};
    uint16_t volatile m4_buffer_missed{0};
    uint8_t volatile radio_tx_drain{0};  // to indicate the baseband thread to drain the tx buffer, and wait for it, before radio::disable()
#ifdef PRALINE
    // Phase 0 instrumentation counters for PRALINE radio debugging
    uint32_t volatile m4_dma_xfr_count{0};    // DMA transfer_complete() calls
    uint32_t volatile m4_dma_wait_count{0};   // wait_for_buffer() calls
    uint32_t volatile m4_baseband_loops{0};   // Main loop iterations
    uint8_t volatile m4_streaming_marker{0};  // Proves streaming_enable() called

    /* Written only by the DAB baseband image, after every field above so the
     * other images keep their offsets. */
    uint32_t volatile m4_boot_stage{0};  // see M4BootStage

    /* Buffers whose ring slot was not the one after the last returned: the
     * stream is spliced wherever this climbs. */
    uint32_t volatile m4_slot_jumps{0};
    /* Transfers the DMA ring lapped past before the thread came back for them,
     * which the slot check cannot see: the catch-up clamp advances the slot by
     * exactly one when the backlog is a whole ring. */
    uint32_t volatile m4_transfers_dropped{0};
    uint32_t volatile m4_slot_last{0};

    /* How close the ring gets to lapping: the largest backlog at a hand-out
     * and the hand-outs past half the ring. */
    uint32_t volatile m4_backlog_max{0};
    uint32_t volatile m4_backlog_high{0};
    uint32_t volatile m4_slots_handed{0};

    /* Dropped transfers binned on whether the baseband had flagged the symbol
     * as carrying a transform, to tell bursty overruns from uniform load. */
    uint8_t volatile m4_in_burst{0};
    uint32_t volatile m4_drop_in_burst{0};
    uint32_t volatile m4_drop_elsewhere{0};
    uint32_t volatile m4_symbols_in_burst{0};
    uint32_t volatile m4_symbols_total{0};

    /* Cycles inside the processor's execute() against wall-clock cycles over
     * the same iterations, a load figure that stays true when samples are
     * lost. */
    uint32_t volatile m4_busy_cycles{0};
    uint32_t volatile m4_wall_cycles{0};
#endif
};

extern SharedMemory& shared_memory;

#endif /*__PORTAPACK_SHARED_MEMORY_H__*/
