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

#ifndef DMR_PHY_SYNC_DETECTOR_H
#define DMR_PHY_SYNC_DETECTOR_H

#include "phy/sync.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    DMR_SYNC_SAMPLES_PER_SYMBOL = 4,
    DMR_SYNC_WINDOW_SAMPLE_COUNT =
        (DMR_SYNC_SYMBOL_COUNT - 1) * DMR_SYNC_SAMPLES_PER_SYMBOL + 1,
    DMR_SYNC_PEAK_SEARCH_SAMPLES = DMR_SYNC_SAMPLES_PER_SYMBOL
};

typedef struct {
    dmr_sync_kind_t kind;
    dmr_sync_fit_t fit;
    uint64_t first_symbol_sample;
    unsigned int symbol_phase;
} dmr_sync_event_t;

typedef void (*dmr_sync_event_cb)(void* context,
                                  const dmr_sync_event_t* event);

typedef struct {
    float samples[DMR_SYNC_WINDOW_SAMPLE_COUNT];
    size_t write_index;
    size_t sample_count;
    uint64_t next_sample_index;
    float threshold;

    bool peak_pending;
    dmr_sync_event_t pending;
    uint64_t pending_last_symbol_sample;
    uint64_t suppress_until_sample;
} dmr_sync_detector_t;

void dmr_sync_detector_init(dmr_sync_detector_t* detector, float threshold);
void dmr_sync_detector_reset(dmr_sync_detector_t* detector);
void dmr_sync_detector_process(dmr_sync_detector_t* detector,
                               const float* samples,
                               size_t count,
                               dmr_sync_event_cb callback,
                               void* callback_context);

#endif
