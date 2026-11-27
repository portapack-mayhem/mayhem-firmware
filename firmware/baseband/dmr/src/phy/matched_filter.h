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

#ifndef DMR_PHY_MATCHED_FILTER_H
#define DMR_PHY_MATCHED_FILTER_H

#include <stdbool.h>
#include <stddef.h>

enum {
    DMR_MATCHED_FILTER_SAMPLES_PER_SYMBOL = 4,
    DMR_MATCHED_FILTER_DELAY_SYMBOLS = 4,
    DMR_MATCHED_FILTER_TAP_COUNT = 33,
    DMR_MATCHED_FILTER_GROUP_DELAY_SAMPLES = 16
};

typedef struct {
    const float* taps;
    float history[41];
    unsigned int count;
    unsigned int head;
} dmr_matched_filter_t;

bool dmr_matched_filter_init(dmr_matched_filter_t* filter);
bool dmr_matched_filter_init_with_delay(dmr_matched_filter_t* filter,
                                        unsigned int delay_symbols);
void dmr_matched_filter_deinit(dmr_matched_filter_t* filter);
void dmr_matched_filter_reset(dmr_matched_filter_t* filter);

/* Input and output may point to the same buffer. */
void dmr_matched_filter_process(dmr_matched_filter_t* filter,
                                const float* input,
                                size_t count,
                                float* output);

#endif
