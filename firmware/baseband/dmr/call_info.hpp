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

#ifndef DMR_CALL_INFO_HPP
#define DMR_CALL_INFO_HPP

extern "C" {
#include <dmr/embedded.h>
}

// RX-thread owned. Only checksum-validated standard group/private voice LC.

class DmrCallInfo {
   public:
    void reset();
    void select(unsigned phase);
    void boundary();
    void header(const dmr_burst_t& burst);
    void embedded(const dmr_emb_t& emb);

    uint32_t source = 0, destination = 0;
    bool valid = false, private_call = false;

   private:
    void accept(const uint8_t* lc);

    unsigned phase_ = 0;
    dmr_embedded_lc_decoder_t decoder_{};
};
#endif
