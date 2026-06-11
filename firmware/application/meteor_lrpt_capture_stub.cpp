/*
 * Copyright (C) 2026
 *
 * 1 MiB SPI (H4M): stubs for Meteor offline M0 deint/G4 — capture uses PMLR + RecordView only.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "meteor_lrpt_deint_service.hpp"
#include "meteor_lrpt_g4_service.hpp"

namespace meteor_lrpt {

void deint_service_configure(uint8_t) {}
void deint_service_start_thread() {}
void deint_service_stop_thread() {}
void deint_service_signal_from_isr(void) {}
bool deint_service_last_alloc_failed() { return false; }
size_t deint_service_arena_bytes() { return 0; }
bool deint_service_heap_budget_ok() { return true; }

void meteor_lrpt_g4_init() {}
void meteor_lrpt_g4_configure(uint8_t) {}
void meteor_lrpt_g4_set_input_path_utf8(const char*) {}
void meteor_lrpt_g4_set_trace_flags(uint8_t) {}

}  // namespace meteor_lrpt
