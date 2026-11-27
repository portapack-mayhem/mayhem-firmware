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

#include "call_info.hpp"
extern "C" {
#include <dmr/data.h>
}

void DmrCallInfo::reset() {
    source = destination = phase_ = 0;
    valid = private_call = false;
    boundary();
}

void DmrCallInfo::select(unsigned phase) {
    if (phase_ != phase) reset();
    phase_ = phase;
}

void DmrCallInfo::boundary() {
    dmr_embedded_lc_decoder_reset(&decoder_);
}

void DmrCallInfo::accept(const uint8_t* lc) {
    const unsigned flco = lc[0] & 0x3f;

    valid = false;

    // Other feature sets/opcodes do not necessarily carry these ID fields.

    if ((lc[0] & 0x80) || lc[1] != 0 || (flco != 0 && flco != 3)) return;

    destination = (uint32_t(lc[3]) << 16) | (uint32_t(lc[4]) << 8) | lc[5];
    source = (uint32_t(lc[6]) << 16) | (uint32_t(lc[7]) << 8) | lc[8];
    private_call = flco == 3;
    valid = true;
}

void DmrCallInfo::header(const dmr_burst_t& burst) {
    uint8_t lc[9];
    unsigned cc, type;

    if (!dmr_full_lc_decode(&burst, lc, &cc, &type)) return;

    select(burst.timeslot);
    boundary();

    if (type == DMR_DATA_TYPE_TERMINATOR_LC) {
        reset();
        return;
    }

    accept(lc);
}

void DmrCallInfo::embedded(const dmr_emb_t& emb) {
    dmr_embedded_lc_t lc;

    if (dmr_embedded_lc_decoder_process(&decoder_, &emb, &lc)) accept(lc.data);
}
