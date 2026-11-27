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

#ifndef DMR_PHY_H
#define DMR_PHY_H

#include <dmr/burst.h>

#include <complex.h>
#include <stddef.h>

typedef struct dmr_phy_rx dmr_phy_rx_t;

enum {
    DMR_PHY_SAMPLES_PER_SYMBOL = 4,
    DMR_PHY_SLOT_SAMPLES = 576
};

typedef void (*dmr_phy_rx_burst_cb)(void* context,
                                    const dmr_burst_t* burst);

typedef struct {
    dmr_phy_rx_burst_cb burst_callback;
    void* callback_context;
    /* Signed normalized correlation in [0, 1]. Zero selects the default. */
    float sync_threshold;
    /* RRC group delay in symbols. 3/4/5 select 25/33/41 taps; zero selects
     * the default of 4. Other values are currently rejected by create(). */
    unsigned int matched_filter_delay_symbols;
    /* After a sync-bearing burst, extract good-quality normal bursts every
     * 30 ms and label the two relative TDMA phases as timeslots 1 and 2. */
    int track_timeslots;
} dmr_phy_rx_config_t;

dmr_phy_rx_t* dmr_phy_rx_create(const dmr_phy_rx_config_t* config);
void dmr_phy_rx_destroy(dmr_phy_rx_t* rx);
void dmr_phy_rx_reset(dmr_phy_rx_t* rx);

void dmr_phy_rx_process(dmr_phy_rx_t* rx,
                        const float complex* samples,
                        size_t count);

#endif
