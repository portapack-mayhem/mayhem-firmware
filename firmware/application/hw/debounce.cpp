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

#include "debounce.hpp"

#include "utility.hpp"

// Returns TRUE if button state changed (after debouncing)
bool Debounce::feed(const uint8_t bit) {
    history_ = (history_ << 1) | (bit & 1);

    if (state_ == 0) {
        // Previous button state was 0 (released);
        // Has button been held for DEBOUNCE_COUNT ticks?
        if ((history_ & DEBOUNCE_MASK) == DEBOUNCE_MASK) {
            state_ = 1;
            return true;
        }
    } else {
        // Previous button state was 1 (pressed);
        // Has button been released for DEBOUNCE_COUNT ticks?
        if ((history_ & DEBOUNCE_MASK) == 0) {
            state_ = 0;
            return true;
        }
    }
    return false;
}
