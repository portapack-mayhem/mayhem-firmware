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
    void clear_baseband_ready() { baseband_ready = false; }
    void set_baseband_ready() { baseband_ready = true; }

    uint8_t volatile request_m4_performance_counter{0};
    uint8_t volatile m4_performance_counter{0};
    uint16_t volatile m4_stack_usage{0};
    uint32_t volatile m4_heap_usage{0};
    uint16_t volatile m4_buffer_missed{0};

    /**
     * Meteor LRPT M2-x interleaved: M4 writes 8192 B raw soft to `soft_in`, M0 runs dual
     * `MeteorDeinterleaverReader` + SD rings, then fills `deint_a` / `deint_b` (8192 B each).
     * `state`: 0 idle, 1 M4 posted soft, 2 M0 finished (M4 clears after consuming).
     * Pointers are set once by M0 (`meteor_lrpt_deint_service`); must live in M0-visible AHB SRAM.
     * `seq`: block generation. M4 increments when posting `state=1`; M0 sets `state=2` only if `seq`
     * still matches the value seen at deinterleave start; M4 IPC timeout increments `seq` to drop a stale completion.
     */
    struct MeteorLrptIpc {
        static constexpr uint32_t kMagic = 0x4D4C5254; /* 'MLRT' */
        volatile uint32_t magic{0};
        volatile uint32_t state{0};
        volatile uint32_t seq{0};
        volatile uint32_t dropped{0};
        /** M0: increments when `read_samples` returns 2 (FatFs / sector deinterleave failure). */
        volatile uint16_t sd_deint_errors{0};
        volatile uint16_t ipc_pad_reserved{0};
        /** M4 `execute()` count when `state` was set to 1 (M0 stall / timeout recovery). */
        volatile uint32_t m4_exec_at_post{0};
        volatile int8_t* soft_in{nullptr};
        volatile int8_t* deint_a{nullptr};
        volatile int8_t* deint_b{nullptr};
        volatile uint8_t flags{0}; /* NRZ-M bit = MeteorLrptRx flags bit2 */
        uint8_t pad[3]{};
    } meteor_lrpt_ipc{};

    /**
     * Meteor LRPT G4 (MSU-MR product): M0 worker demux/JPEG/BMP; M4 may push live CADU when enabled.
     * UI sets `enabled` via `meteor_lrpt_g4_configure` (lrpt_flags bit6). Counters are written by
     * the G4 worker; ring indices follow strict SPSC: M4 updates `ring_push` only, M0 `ring_pop` only.
     */
    struct MeteorLrptG4Ipc {
        static constexpr uint32_t kMagic = 0x4D344734; /* 'M4G4' */
        static constexpr size_t kInputPathUtf8Max = 96;
        /** Single slot: minimal Bank2 RAM; M4 drops CADUs if M0 has not popped yet (`live_ring_overflows`). */
        static constexpr size_t kLiveRingSlots = 1;
        volatile uint32_t magic{0};
        volatile uint8_t enabled{0};
        volatile uint8_t lrpt_flags_snapshot{0};
        uint8_t pad0[2]{};
        /** Bitfield of last error / drop reasons (see meteor_lrpt_g4_service.hpp). */
        volatile uint32_t drop_bits{0};
        volatile uint32_t cadus_processed{0};
        volatile uint32_t jpeg_ok_count{0};
        volatile uint32_t bmp_write_count{0};
        /** Last JPEG prepare/decomp JRESULT (TJpgDec), for diagnostics. */
        volatile uint8_t last_jresult{0};
        /** M0: `meteor_lrpt_g4_set_input_path_utf8` / UI. Empty → default CADU tail path in worker. */
        volatile char input_path_utf8[kInputPathUtf8Max]{};
        /** M0: set with LRPT flag bit7 when MSU-MR + live ring desired. M4 pushes post-RS CADU here (`proc_meteor_lrpt_rx`). */
        volatile uint8_t live_ring_enable{0};
        /** bit0: write `.ppm` sidecar next to BMP for debug. */
        volatile uint8_t debug_flags{0};
        uint8_t pad_dbg[2]{};
        /** Monotonic indices: occupancy = `ring_push - ring_pop` (SPSC: M4 writes `ring_push`, M0 writes `ring_pop`). */
        volatile uint32_t ring_push{0};
        volatile uint32_t ring_pop{0};
        /** Post-RS CADUs **not** copied into the ring because it was full (incoming dropped); see `meteor_lrpt_g4_ring_push_post_rs`. */
        volatile uint32_t live_ring_overflows{0};
        uint8_t live_ring_slots[kLiveRingSlots][1020]{};

        /** Last decoded BMP path (ASCII/UTF-8 as stored on SD); UI / optional paging viewer. */
        static constexpr size_t kLastBmpUtf8Max = 64;
        volatile char last_bmp_utf8[kLastBmpUtf8Max]{};

        /** Nearest-neighbor RGB888 thumbnail (worker fills after decode); UI polls `preview_seq`. */
        static constexpr size_t kPreviewWidth = 16;
        static constexpr size_t kPreviewHeight = 12;
        static constexpr size_t kPreviewRgbBytes = kPreviewWidth * kPreviewHeight * 3;
        volatile uint32_t preview_seq{0};
        volatile uint16_t preview_width{0};
        volatile uint16_t preview_height{0};
        volatile uint8_t preview_rgb[kPreviewRgbBytes]{};
    } meteor_lrpt_g4_ipc{};

#ifdef PRALINE
    // Phase 0 instrumentation counters for PRALINE radio debugging
    uint32_t volatile m4_dma_xfr_count{0};    // DMA transfer_complete() calls
    uint32_t volatile m4_dma_wait_count{0};   // wait_for_buffer() calls
    uint32_t volatile m4_baseband_loops{0};   // Main loop iterations
    uint8_t volatile m4_streaming_marker{0};  // Proves streaming_enable() called
#endif
};

extern SharedMemory& shared_memory;

#endif /*__PORTAPACK_SHARED_MEMORY_H__*/
