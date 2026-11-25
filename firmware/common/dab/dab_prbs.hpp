/*
 * Copyright (C) 2026 Mayhem firmware contributors
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

/* DAB energy dispersal (EN 300 401 section 10): PRBS x^9 + x^5 + 1, register
 * initialised to all ones. Scrambling and descrambling are the same XOR.
 * The sequence begins 0x07 0xBE 0x2E 0x64. */

#ifndef __DAB_PRBS_H__
#define __DAB_PRBS_H__

#include <cstddef>
#include <cstdint>

namespace dab {

class PrbsGenerator {
   public:
    PrbsGenerator() { reset(); }

    void reset() { register_ = 0x1FF; }

    uint8_t next_bit() {
        /* Output the feedback bit: the shifted-out bit is the same m-sequence
         * delayed by nine, which passes structural tests and descrambles to noise. */
        const uint8_t feedback = ((register_ >> 8) ^ (register_ >> 4)) & 1;
        register_ = static_cast<uint16_t>(((register_ << 1) | feedback) & 0x1FF);
        return feedback;
    }

    uint8_t next_byte() {
        uint8_t v = 0;
        for (int i = 0; i < 8; ++i)
            v = static_cast<uint8_t>((v << 1) | next_bit());
        return v;
    }

   private:
    uint16_t register_{0x1FF};
};

/* In place, from the sequence's initial state. */
void prbs_descramble(uint8_t* data, size_t length);

} /* namespace dab */

#endif /*__DAB_PRBS_H__*/
