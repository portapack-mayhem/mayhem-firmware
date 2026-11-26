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

#include "baseband_dma.hpp"
#include "portapack_shared_memory.hpp"

#include <cstdint>
#include <cstddef>
#include <array>

#include "hal.h"
#include "gpdma.hpp"

using namespace lpc43xx;

#include "portapack_dma.hpp"

#include "thread_wait.hpp"

namespace baseband {
namespace dma {

constexpr uint32_t gpdma_ahb_master_sgpio = 0;
constexpr uint32_t gpdma_ahb_master_memory = 1;
constexpr uint32_t gpdma_ahb_master_lli_fetch = 0;

constexpr uint32_t gpdma_src_peripheral = 0x0;
constexpr uint32_t gpdma_dest_peripheral = 0x0;

constexpr gpdma::channel::LLIPointer lli_pointer(const void* lli) {
    return {
        .lm = gpdma_ahb_master_lli_fetch,
        .r = 0,
        .lli = reinterpret_cast<uint32_t>(lli),
    };
}

constexpr gpdma::channel::Control control(const baseband::Direction direction, const size_t buffer_words) {
    return {
        .transfersize = buffer_words,
        .sbsize = 0, /* Burst size: 1 */
        .dbsize = 0, /* Burst size: 1 */
        .swidth = 2, /* Source transfer width: word (32 bits) */
        .dwidth = 2, /* Destination transfer width: word (32 bits) */
        .s = (direction == baseband::Direction::Transmit) ? gpdma_ahb_master_memory : gpdma_ahb_master_sgpio,
        .d = (direction == baseband::Direction::Transmit) ? gpdma_ahb_master_sgpio : gpdma_ahb_master_memory,
        .si = (direction == baseband::Direction::Transmit) ? 1U : 0U,
        .di = (direction == baseband::Direction::Transmit) ? 0U : 1U,
        .prot1 = 0,
        .prot2 = 0,
        .prot3 = 0,
        .i = 1,
    };
}

constexpr gpdma::channel::Config config(const baseband::Direction direction) {
    return {
        .e = 0,
        .srcperipheral = gpdma_src_peripheral,
        .destperipheral = gpdma_dest_peripheral,
        .flowcntrl = (direction == baseband::Direction::Transmit)
                         ? gpdma::FlowControl::MemoryToPeripheral_DMAControl
                         : gpdma::FlowControl::PeripheralToMemory_DMAControl,
        .ie = 1,
        .itc = 1,
        .l = 0,
        .a = 0,
        .h = 0,
    };
}

/* 8192 samples is 4 ms at 2.048 Msps, which work between buffers must fit in;
 * DAB's symbol-boundary bursts overrun it, so the value is set per image. */
#ifndef BASEBAND_BUFFER_LOG2N
#define BASEBAND_BUFFER_LOG2N 13
#endif

constexpr size_t buffer_samples_log2n = BASEBAND_BUFFER_LOG2N;
constexpr size_t buffer_samples = (1 << buffer_samples_log2n);
/* Deepen by adding transfers, not enlarging them: 4096 samples is the GPDMA
 * count limit and doubles what each miss costs. Keep in step with
 * proc_dab.hpp's fallback; nothing links the two translation units. */
#ifndef BASEBAND_TRANSFERS_LOG2N
#define BASEBAND_TRANSFERS_LOG2N 2
#endif

constexpr size_t transfers_per_buffer_log2n = BASEBAND_TRANSFERS_LOG2N;
constexpr size_t transfers_per_buffer = (1 << transfers_per_buffer_log2n);
constexpr size_t transfer_samples = buffer_samples / transfers_per_buffer;
constexpr size_t transfers_mask = transfers_per_buffer - 1;

constexpr size_t buffer_bytes = buffer_samples * sizeof(baseband::sample_t);
constexpr size_t transfer_bytes = transfer_samples * sizeof(baseband::sample_t);

constexpr size_t msg_count = transfers_per_buffer - 1;

static std::array<gpdma::channel::LLI, transfers_per_buffer> lli_loop;
static constexpr auto& gpdma_channel_sgpio = gpdma::channels[portapack::sgpio_gpdma_channel_number];

static ThreadWait thread_wait;

volatile uint32_t buffer_transfered = 0;
volatile uint32_t buffer_handled = 0;

static void transfer_complete() {
    const auto next_lli_index = gpdma_channel_sgpio.next_lli() - &lli_loop[0];
    buffer_transfered++;
#ifdef PRALINE
    shared_memory.m4_dma_xfr_count++;  // Phase 0 instrumentation
#endif
    thread_wait.wake_from_interrupt(next_lli_index);
}

static void dma_error() {
    thread_wait.wake_from_interrupt(-1);
    disable();
}

void init() {
    gpdma_channel_sgpio.set_handlers(transfer_complete, dma_error);
#if defined(PORTAPACK_BASEBAND_DMA_NO_SYNC)
    /* Disable synchronization logic to improve(?) DMA response time.
     * SGPIO (peripheral) must be on same clock as GPDMA peripheral.
     * SGPIO runs from BASE_PERIPH_CLK, which is set to PLL1 in normal
     * operation, same as the M4 and M0 cores. Memory, of course, is
     * running from the same clock as the cores.
     */
    LPC_GPDMA->SYNC |= (1 << gpdma_src_peripheral);
    LPC_GPDMA->SYNC |= (1 << gpdma_dest_peripheral);
#endif
}

void configure(
    baseband::sample_t* const buffer_base,
    const baseband::Direction direction) {
    const auto peripheral = reinterpret_cast<uint32_t>(&LPC_SGPIO->REG_SS[0]);
    const auto control_value = control(direction, gpdma::buffer_words(transfer_bytes, 4));
    for (size_t i = 0; i < lli_loop.size(); i++) {
        const auto memory = reinterpret_cast<uint32_t>(&buffer_base[i * transfer_samples]);
        lli_loop[i].srcaddr = (direction == Direction::Transmit) ? memory : peripheral;
        lli_loop[i].destaddr = (direction == Direction::Transmit) ? peripheral : memory;
        lli_loop[i].lli = lli_pointer(&lli_loop[(i + 1) % lli_loop.size()]);
        lli_loop[i].control = control_value;
    }
}

void enable(const baseband::Direction direction) {
    const auto gpdma_config = config(direction);
    gpdma_channel_sgpio.configure(lli_loop[0], gpdma_config);
    gpdma_channel_sgpio.enable();
}

bool is_enabled() {
    return gpdma_channel_sgpio.is_enabled();
}

void disable() {
    gpdma_channel_sgpio.disable();
}

#ifdef BASEBAND_dab_rx
/* The next slot to hand out, one counter for the sleep and catch-up paths:
 * deriving it from the LLI pointer and the handled count separately splices
 * the stream when the thread falls behind. Skips only when the ring laps. */
static uint32_t data_index = 0;

/* The first wake is dropped once, so the slot handed out trails the writer by
 * two transfers, not one; one behind hands back a slot the DMA has only just
 * completed and breaks the stream at every transfer boundary. */
static bool primed = false;

static size_t next_data_slot() {
    const uint32_t completed = buffer_transfered;
    /* The near misses, which the drop counter is blind to by construction. */
    {
        const int32_t backlog = static_cast<int32_t>(completed - data_index);
        if (backlog > 0) {
            if (static_cast<uint32_t>(backlog) > shared_memory.m4_backlog_max)
                shared_memory.m4_backlog_max = static_cast<uint32_t>(backlog);
            if (static_cast<uint32_t>(backlog) * 2 >= transfers_per_buffer)
                shared_memory.m4_backlog_high++;
        }
        shared_memory.m4_slots_handed++;
    }
    if (static_cast<int32_t>(completed - data_index) > static_cast<int32_t>(transfers_per_buffer - 1)) {
        /* The ring has lapped and this data is gone. Counted here: the clamp
         * looks like normal progress to note_slot(), and buffer_handled counts
         * calls, not data. */
        const uint32_t lost = completed - (transfers_per_buffer - 1) - data_index;
        shared_memory.m4_transfers_dropped += lost;
        if (shared_memory.m4_in_burst)
            shared_memory.m4_drop_in_burst += lost;
        else
            shared_memory.m4_drop_elsewhere += lost;
        data_index = completed - (transfers_per_buffer - 1);
    }
    const size_t slot = data_index & transfers_mask;
    data_index++;
    return slot;
}

/* Records which ring slot each returned buffer came from, so a splice shows
 * up as a number rather than as a signal that will not decode. */
static void note_slot(size_t index) {
    const uint32_t expected = (shared_memory.m4_slot_last + 1) & transfers_mask;
    if (index != expected)
        shared_memory.m4_slot_jumps++;
    shared_memory.m4_slot_last = index;
}
#endif

baseband::buffer_t wait_for_buffer() {
#ifdef PRALINE
    shared_memory.m4_dma_wait_count++;  // Phase 0 instrumentation
#endif

#ifdef BASEBAND_dab_rx
    /* Take a buffer already waiting rather than sleep: ThreadWait drops a wake
     * that arrives while the thread runs, so transfers completed during long
     * work would never be collected. Tested on data_index, not buffer_handled:
     * a lap clamp advances data_index alone, and the handled count then ran
     * the reader ahead of the writer for good, splicing the stream by a whole
     * ring. Same margin as the handled test without a lap. */
    if (static_cast<int32_t>(buffer_transfered - data_index) > 2) {
        const size_t index = next_data_slot();
        buffer_handled++;
        shared_memory.m4_buffer_missed = buffer_transfered - data_index - 1;

        note_slot(index);

        const auto src = lli_loop[index].srcaddr;
        const auto dst = lli_loop[index].destaddr;
        const auto p = (src == reinterpret_cast<uint32_t>(&LPC_SGPIO->REG_SS[0])) ? dst : src;
        return {reinterpret_cast<sample_t*>(p), transfer_samples};
    }
#endif

    const auto next_index = thread_wait.sleep();
    buffer_handled++;

    auto buffer_missed = buffer_transfered - buffer_handled;
    shared_memory.m4_buffer_missed = buffer_missed;

    if (next_index >= 0) {
#ifdef BASEBAND_dab_rx
        if (!primed) {
            /* Give the writer a transfer of head start, once. */
            primed = true;
            return {};
        }

        /* Same counter as the catch-up path, so the two cannot disagree. */
        const size_t free_index = next_data_slot();
        shared_memory.m4_buffer_missed = buffer_transfered - data_index - 1;
        note_slot(free_index);
#else
        const size_t free_index = (next_index + transfers_per_buffer - 2) & transfers_mask;
#endif
        const auto src = lli_loop[free_index].srcaddr;
        const auto dst = lli_loop[free_index].destaddr;
        const auto p = (src == reinterpret_cast<uint32_t>(&LPC_SGPIO->REG_SS[0])) ? dst : src;
        return {reinterpret_cast<sample_t*>(p), transfer_samples};
    } else {
#ifdef BASEBAND_dab_rx
        /* A wake with no LLI index drops a whole transfer that neither the
         * loss figure nor note_slot() would see. */
        shared_memory.m4_slot_jumps++;
#endif
        return {};
    }
}

} /* namespace dma */
} /* namespace baseband */
