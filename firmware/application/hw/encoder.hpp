/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <cstdint>

enum encoder_sensitivity {
    DIAL_SENSITIVITY_MEDIUM = 0,
    DIAL_SENSITIVITY_LOW,
    NUM_DIAL_SENSITIVITY
};

class Encoder {
   public:
    int_fast8_t update(
        const uint_fast8_t phase_0,
        const uint_fast8_t phase_1);

    void set_sensitivity(const uint8_t v) {
        if (v < NUM_DIAL_SENSITIVITY)
            sensitivity_ = v;
    };

   private:
    uint_fast8_t state{0};
    uint8_t sensitivity_{DIAL_SENSITIVITY_MEDIUM};
};

#endif /*__ENCODER_H__*/
