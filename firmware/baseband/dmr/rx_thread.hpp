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

#ifndef DMR_RX_THREAD_HPP
#define DMR_RX_THREAD_HPP

#include "thread_base.hpp"
#include "baseband_processor.hpp"
#include "rx_dma_backlog.hpp"
#include <ch.h>

// DMR-only buffered RX worker. Ordinary Mayhem RX/TX threads are unchanged.

class DmrRxThread : public ThreadBase {
   public:
    DmrRxThread(uint32_t sampling_rate, BasebandProcessor* processor)
        : sampling_rate_{sampling_rate}, processor_{processor} {}
    ~DmrRxThread();
    DmrRxThread(const DmrRxThread&) = delete;
    DmrRxThread& operator=(const DmrRxThread&) = delete;
    void start() override;

   private:
    void run() override;
    bool copy_next(complex8_t* snapshot);
    uint32_t active_block() const;
    static void transfer_complete();
    static void dma_error();

    static DmrRxThread* instance;
    const uint32_t sampling_rate_;
    BasebandProcessor* const processor_;
    Thread* thread_ = nullptr;
    complex8_t* dma_buffer_ = nullptr;
    baseband::RxDmaBacklog backlog_{};
    BinarySemaphore ready_{};
    bool error_ = false;
};
#endif
