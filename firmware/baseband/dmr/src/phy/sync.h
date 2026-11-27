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

#ifndef DMR_PHY_SYNC_H
#define DMR_PHY_SYNC_H

#include <dmr/burst.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DMR_SYNC_SYMBOL_COUNT = 24 };

uint32_t dmr_sync_candidates(const float observed[DMR_SYNC_SYMBOL_COUNT],
                             float threshold);

bool dmr_sync_word(dmr_sync_kind_t kind, uint64_t* word);

typedef struct {
    float offset_hz;
    float gain_hz_per_level;
    float residual_rms_hz;
    float score;
} dmr_sync_fit_t;

void dmr_sync_expected_levels(dmr_sync_kind_t kind,
                              float levels[DMR_SYNC_SYMBOL_COUNT]);

/* Fits observed = offset + gain * expected. A valid voice sync has positive
 * gain; score is the signed normalized correlation in [-1, +1]. */

bool dmr_sync_correlate(const float observed[DMR_SYNC_SYMBOL_COUNT],
                        dmr_sync_kind_t kind,
                        dmr_sync_fit_t* fit);

#endif
