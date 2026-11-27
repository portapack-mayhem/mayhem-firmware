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

#ifndef DMR_EMBEDDED_H
#define DMR_EMBEDDED_H

#include <dmr/burst.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    unsigned int color_code;
    int privacy;
    unsigned int lcss;
    unsigned int corrected_bits;
    uint8_t fragment[4];
} dmr_emb_t;

/* Caller-owned storage avoids a separate heap allocation on the M4. */
typedef struct dmr_embedded_lc_decoder {
    uint8_t raw[128];
    unsigned int state;
    unsigned int corrected_bits;
} dmr_embedded_lc_decoder_t;

typedef struct {
    uint8_t data[9];
    unsigned int flco;
    unsigned int feature_set_id;
    unsigned int service_options;
    uint32_t target_id;
    uint32_t source_id;
    unsigned int corrected_bits;
} dmr_embedded_lc_t;

int dmr_emb_decode(const dmr_burst_t* burst, dmr_emb_t* emb);
void dmr_embedded_lc_decoder_reset(dmr_embedded_lc_decoder_t* decoder);
int dmr_embedded_lc_decoder_process(dmr_embedded_lc_decoder_t* decoder,
                                    const dmr_emb_t* emb,
                                    dmr_embedded_lc_t* lc);

#endif
