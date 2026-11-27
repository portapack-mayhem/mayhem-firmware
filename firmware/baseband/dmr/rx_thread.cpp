/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
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

#include "rx_thread.hpp"
#include "baseband.hpp"
#include "baseband_dma.hpp"
#include "baseband_sgpio.hpp"
#include "gpdma.hpp"
#include "portapack_dma.hpp"
#include "portapack_shared_memory.hpp"
#include <array>
#include <memory>
#include <cstring>

namespace {
WORKING_AREA(dmr_rx_thread_wa, 2048);
baseband::SGPIO sgpio;
constexpr size_t block_samples = 2048;
constexpr size_t dma_samples = 4 * block_samples;
constexpr auto& channel = lpc43xx::gpdma::channels[portapack::sgpio_gpdma_channel_number];
}  // namespace

DmrRxThread* DmrRxThread::instance = nullptr;

DmrRxThread::~DmrRxThread() {
    if (thread_) {
        chThdTerminate(thread_);
        chThdWait(thread_);
    }

    instance = nullptr;
}

void DmrRxThread::start() {
    if (!thread_) {
        instance = this;
        thread_ = chThdCreateStatic(dmr_rx_thread_wa, sizeof(dmr_rx_thread_wa),
                                    NORMALPRIO + 20, ThreadBase::fn, this);
    }
}

uint32_t DmrRxThread::active_block() const {
    // The DMA LLI register points one descriptor ahead of the active block.

    const auto next_address = channel.next_lli()->destaddr;
    const uint32_t next = (next_address - reinterpret_cast<uint32_t>(dma_buffer_)) /
                          (block_samples * sizeof(complex8_t));
    return (next + 3u) & 3u;
}
void DmrRxThread::transfer_complete() {
    // gpdma's ISR invokes handlers with the ChibiOS ISR lock held.

    instance->backlog_.complete((instance->active_block() + 3u) & 3u);
    chBSemSignalI(&instance->ready_);
#ifdef PRALINE
    ++shared_memory.m4_dma_xfr_count;
#endif
}

void DmrRxThread::dma_error() {
    instance->error_ = true;
    chBSemSignalI(&instance->ready_);
    baseband::dma::disable();
}

bool DmrRxThread::copy_next(complex8_t* snapshot) {
    while (true) {
        baseband::RxDmaBacklog::Block block{};
        chSysLock();

        // Check and wait atomically: an interrupt while execute() is busy
        // leaves both its sequence number and a semaphore notification.

        while (!error_ && !backlog_.take(block))
            chBSemWaitS(&ready_);

        if (error_) {
            chSysUnlock();
            chThdSleepMilliseconds(1);  // Permit clean shutdown after error.
            return false;
        }

        // Account for hardware advancing before its IRQ has been serviced.

        const bool safe_before = active_block() != block.index;
        chSysUnlock();

        if (safe_before)
            std::memcpy(snapshot, dma_buffer_ + block.index * block_samples,
                        block_samples * sizeof(complex8_t));

        chSysLock();

        const bool valid = safe_before && active_block() != block.index &&
                           backlog_.readable(block) && !error_;

        if (!valid) backlog_.discard_copy();

        shared_memory.m4_buffer_missed = backlog_.missed();
        chSysUnlock();

        if (valid) return true;
        // Never hand torn IQ to DSP if DMA reached the source during copying.
    }
}

void DmrRxThread::run() {
    const auto dma_buffer = std::make_unique<std::array<complex8_t, dma_samples>>();

    // UI is link-time limited to the first 4 KiB of the extended 8 KiB region.

    auto* snapshot_data = reinterpret_cast<complex8_t*>(0x10089000);

    dma_buffer_ = dma_buffer->data();
    backlog_.reset();
    chBSemInit(&ready_, true);
    shared_memory.m4_buffer_missed = 0;
    sgpio.init();
    baseband::dma::init();
    baseband::dma::configure(dma_buffer_, baseband::Direction::Receive);

    // Override only this image's SGPIO completion handlers. Other DMA
    // channels (RSSI, audio) retain their ordinary handlers.

    channel.set_handlers(transfer_complete, dma_error);
    sgpio.configure(baseband::Direction::Receive);
    baseband::dma::enable(baseband::Direction::Receive);
    sgpio.streaming_enable();
#ifdef PRALINE
    shared_memory.m4_streaming_marker = 0xaa;
#endif
    while (!chThdShouldTerminate()) {
#ifdef PRALINE
        ++shared_memory.m4_baseband_loops;
        ++shared_memory.m4_dma_wait_count;
#endif
        if (copy_next(snapshot_data)) {
            const buffer_c8_t buffer{snapshot_data, block_samples, sampling_rate_};
            processor_->execute(buffer);
        }
    }

    baseband::dma::disable();
    sgpio.streaming_disable();
    dma_buffer_ = nullptr;
}
