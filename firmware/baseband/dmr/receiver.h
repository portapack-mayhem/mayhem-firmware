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

#ifndef MAYHEM_DMR_RECEIVER_H
#define MAYHEM_DMR_RECEIVER_H

#include <stdint.h>
#include "include/dmr/burst.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dmr_phy_rx;

struct dmr_phy_rx* mayhem_dmr_create(void (*callback)(void*, const dmr_burst_t*), void* context);
void mayhem_dmr_reset(struct dmr_phy_rx* rx);
void mayhem_dmr_sample(struct dmr_phy_rx* rx, float i, float q);

/* Returns -1 if no protected Color Code is available in this burst. */

int mayhem_dmr_color_code(const dmr_burst_t* burst);

#ifdef __cplusplus
}
#endif
#endif
