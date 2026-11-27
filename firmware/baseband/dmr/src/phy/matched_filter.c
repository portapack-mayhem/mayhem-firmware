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

#include "phy/matched_filter.h"
#include <string.h>
#include "rrc_taps.inc"

bool dmr_matched_filter_init(dmr_matched_filter_t* filter) {
    return dmr_matched_filter_init_with_delay(filter, 4);
}

bool dmr_matched_filter_init_with_delay(dmr_matched_filter_t* filter,
                                        unsigned int delay_symbols) {
    if (delay_symbols < 3 || delay_symbols > 5)
        return false;
    filter->taps = delay_symbols == 3 ? rrc_3 : delay_symbols == 4 ? rrc_4
                                                                   : rrc_5;
    filter->count = 8 * delay_symbols + 1;
    dmr_matched_filter_reset(filter);
    return true;
}

void dmr_matched_filter_deinit(dmr_matched_filter_t* filter) {
    filter->taps = NULL;
}

void dmr_matched_filter_reset(dmr_matched_filter_t* filter) {
    memset(filter->history, 0, sizeof(filter->history));
    filter->head = 0;
}

void dmr_matched_filter_process(dmr_matched_filter_t* filter,
                                const float* input,
                                size_t count,
                                float* output) {
    for (size_t i = 0; i < count; ++i) {
        unsigned int index = filter->head;
        filter->history[index] = input[i];
        float sum = 0;
        for (unsigned int t = 0; t < filter->count; ++t) {
            sum += filter->taps[t] * filter->history[index];
            index = index == 0 ? filter->count - 1 : index - 1;
        }
        output[i] = sum;
        if (++filter->head == filter->count)
            filter->head = 0;
    }
}
