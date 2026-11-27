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

#ifndef DMR_BURST_H
#define DMR_BURST_H

#include <stdint.h>
#include <stddef.h>

enum {
    DMR_BURST_DIBIT_COUNT = 132,
    DMR_BURST_BIT_COUNT = 264,
    DMR_BURST_BYTE_COUNT = 33
};

typedef enum {
    DMR_SYNC_BS_VOICE,
    DMR_SYNC_MS_VOICE,
    DMR_SYNC_BS_DATA,
    DMR_SYNC_MS_DATA,
    DMR_SYNC_KIND_COUNT
} dmr_sync_kind_t;

typedef struct {
    /* On-air order, four dibits per byte, first dibit in bits 7..6. */
    uint8_t data[DMR_BURST_BYTE_COUNT];
    /* Sampling point of the first dibit in the filtered 19.2 kHz stream.
     * Index zero is the first discriminator output after RX reset. */
    uint64_t first_sample_index;
    /* Relative TDMA phase: 1/2 when tracking is enabled, otherwise zero. */
    unsigned int timeslot;
    /* False for a burst extracted from the acquired TDMA grid. */
    int has_sync;
    dmr_sync_kind_t sync_kind;
    float cfo_hz;
    float deviation_scale;
    float sync_score;
    /* Burst-local timing slope selected on held-out payload symbols. */
    float sample_rate_error_ppm;
} dmr_burst_t;

/* Packed burst helpers. Get returns UINT8_MAX for an invalid index. */
int dmr_burst_set_dibit(
    uint8_t data[DMR_BURST_BYTE_COUNT],
    size_t index,
    uint8_t dibit);
uint8_t dmr_burst_get_bit(
    const uint8_t data[DMR_BURST_BYTE_COUNT],
    size_t index);

#endif
