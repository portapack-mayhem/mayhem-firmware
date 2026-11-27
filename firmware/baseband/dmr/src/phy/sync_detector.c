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

#include "phy/sync_detector.h"

#include <string.h>

static void read_symbol_window(const dmr_sync_detector_t* detector,
                               float symbols[DMR_SYNC_SYMBOL_COUNT]) {
    /* write_index points at the oldest sample once the ring is full. */
    for (size_t i = 0; i < DMR_SYNC_SYMBOL_COUNT; ++i) {
        const size_t offset = i * DMR_SYNC_SAMPLES_PER_SYMBOL;
        const size_t index =
            (detector->write_index + offset) % DMR_SYNC_WINDOW_SAMPLE_COUNT;
        symbols[i] = detector->samples[index];
    }
}

static void emit_pending(dmr_sync_detector_t* detector,
                         dmr_sync_event_cb callback,
                         void* callback_context) {
    if (callback != NULL)
        callback(callback_context, &detector->pending);

    /* One standard sync cannot overlap another. Suppression prevents shifted
     * views of the same 24-symbol sequence from producing duplicate events. */
    detector->suppress_until_sample = detector->pending_last_symbol_sample +
                                      DMR_SYNC_WINDOW_SAMPLE_COUNT - 1;
    detector->peak_pending = false;
}

void dmr_sync_detector_init(dmr_sync_detector_t* detector, float threshold) {
    memset(detector, 0, sizeof(*detector));
    detector->threshold = threshold;
}

void dmr_sync_detector_reset(dmr_sync_detector_t* detector) {
    const float threshold = detector->threshold;
    memset(detector, 0, sizeof(*detector));
    detector->threshold = threshold;
}

void dmr_sync_detector_process(dmr_sync_detector_t* detector,
                               const float* samples,
                               size_t count,
                               dmr_sync_event_cb callback,
                               void* callback_context) {
    for (size_t sample = 0; sample < count; ++sample) {
        const uint64_t absolute_index = detector->next_sample_index++;

        detector->samples[detector->write_index] = samples[sample];
        detector->write_index =
            (detector->write_index + 1) % DMR_SYNC_WINDOW_SAMPLE_COUNT;

        if (detector->sample_count < DMR_SYNC_WINDOW_SAMPLE_COUNT)
            ++detector->sample_count;

        if (detector->peak_pending &&
            absolute_index > detector->pending_last_symbol_sample +
                                 DMR_SYNC_PEAK_SEARCH_SAMPLES) {
            emit_pending(detector, callback, callback_context);
        }

        if (detector->sample_count < DMR_SYNC_WINDOW_SAMPLE_COUNT ||
            absolute_index <= detector->suppress_until_sample) {
            continue;
        }

        float symbols[DMR_SYNC_SYMBOL_COUNT];
        read_symbol_window(detector, symbols);

        const uint32_t candidates = dmr_sync_candidates(symbols, detector->threshold);

        for (dmr_sync_kind_t kind = DMR_SYNC_BS_VOICE;
             kind < DMR_SYNC_KIND_COUNT; ++kind) {
            if (!(candidates & (1u << kind))) continue;

            dmr_sync_fit_t fit;

            if (!dmr_sync_correlate(symbols, kind, &fit) ||
                fit.score < detector->threshold) {
                continue;
            }

            if (!detector->peak_pending ||
                fit.score > detector->pending.fit.score) {
                detector->pending.kind = kind;
                detector->pending.fit = fit;
                detector->pending_last_symbol_sample = absolute_index;
                detector->pending.first_symbol_sample =
                    absolute_index - (DMR_SYNC_WINDOW_SAMPLE_COUNT - 1);
                detector->pending.symbol_phase =
                    (unsigned int)(absolute_index %
                                   DMR_SYNC_SAMPLES_PER_SYMBOL);
                detector->peak_pending = true;
            }
        }
    }
}
