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

#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

#include <cstdint>

// consecutive # of times button input must be same (<=8)
#define DEBOUNCE_COUNT 4
#define DEBOUNCE_MASK ((1 << DEBOUNCE_COUNT) - 1)

// # of timer0 ticks before a held button starts being counted as repeated presses
#define REPEAT_INITIAL_DELAY 250
#define REPEAT_SUBSEQUENT_DELAY 92

class Debounce {
   public:
    bool feed(const uint8_t bit);

    uint8_t state() const {
        return state_;
    }

    void enable_repeat() {
        repeat_enabled_ = true;
    }

   private:
    uint8_t history_{0};
    uint8_t state_{0};
    bool repeat_enabled_{0};
    uint16_t repeat_ctr_{0};
    uint16_t held_time_{0};
};

#endif /*__DEBOUNCE_H__*/
