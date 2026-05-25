/*
 * Copyright (C) 2026
 *
 * M0 worker: dual `MeteorDeintFileRing` + `MeteorDeinterleaverReader` for Meteor LRPT interleaved
 * mode; cooperates with M4 via `SharedMemory::meteor_lrpt_ipc`.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef METEOR_LRPT_DEINT_SERVICE_HPP
#define METEOR_LRPT_DEINT_SERVICE_HPP

#include <cstddef>
#include <cstdint>

namespace meteor_lrpt {

void deint_service_configure(uint8_t meteor_flags);
void deint_service_start_thread();
void deint_service_stop_thread();

/** M0 ISR (M4Core_IRQHandler): wake deinterleave thread when IPC has a soft block ready. */
void deint_service_signal_from_isr(void);

/** True if the last interleaved enable failed `chHeapAlloc` for the deint arena. */
bool deint_service_last_alloc_failed();

/** Bytes required for one interleaved deinterleave arena allocation. */
size_t deint_service_arena_bytes();

/** True if the M0 heap can likely satisfy @ref deint_service_arena_bytes() (plus headroom). */
bool deint_service_heap_budget_ok();

}  // namespace meteor_lrpt

#endif
