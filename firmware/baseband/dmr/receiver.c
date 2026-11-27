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

#include "receiver.h"
#include <dmr/phy.h>
#include <dmr/data.h>
#include <dmr/embedded.h>

struct dmr_phy_rx* mayhem_dmr_create(void (*callback)(void*, const dmr_burst_t*), void* context) {
    const dmr_phy_rx_config_t config = {
        .burst_callback = callback, .callback_context = context, .track_timeslots = 1};
    return dmr_phy_rx_create(&config);
}

void mayhem_dmr_reset(struct dmr_phy_rx* rx) {
    dmr_phy_rx_reset(rx);
}

void mayhem_dmr_sample(struct dmr_phy_rx* rx, float i, float q) {
    const float complex sample = i + I * q;
    dmr_phy_rx_process(rx, &sample, 1);
}

int mayhem_dmr_color_code(const dmr_burst_t* burst) {
    if (!burst->has_sync) {
        dmr_emb_t emb;
        return dmr_emb_decode(burst, &emb) ? (int)emb.color_code : -1;
    }

    if (burst->sync_kind == DMR_SYNC_BS_DATA || burst->sync_kind == DMR_SYNC_MS_DATA) {
        uint32_t slot = 0;

        for (unsigned i = 0; i < 10; ++i)
            slot = (slot << 1) | dmr_burst_get_bit(burst->data, 98 + i);

        for (unsigned i = 0; i < 10; ++i)
            slot = (slot << 1) | dmr_burst_get_bit(burst->data, 156 + i);

        unsigned cc, type;

        if (dmr_slot_type_decode(slot, &cc, &type, NULL)) return (int)cc;
    }

    return -1;
}
