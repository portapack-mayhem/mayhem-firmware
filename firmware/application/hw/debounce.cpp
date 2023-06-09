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

    // "Repeat" handling - simulated button release
    if (repeat_ctr_) {
        // Make sure the button is still being held continuously
        if (history_ == 0xFF) {
            // Simulate button press every REPEAT_SUBSEQUENT_DELAY ticks
            if (--repeat_ctr_ == 0) {
                state_ = !state_;
                repeat_ctr_ = REPEAT_SUBSEQUENT_DELAY / 2;
                return true;
            }
        } else {
            // It's a real button release; stop simulating
            repeat_ctr_ = 0;
        }
    }

    if (state_ == 0) {
        // Previous button state was 0 (released);
        // Has button been held for DEBOUNCE_COUNT ticks?
        if ((history_ & DEBOUNCE_MASK) == DEBOUNCE_MASK) {
            state_ = 1;
            held_time_ = 0;
            return true;
        }
    } else {
        // Previous button state was 1 (pressed);
        // Has button been released for DEBOUNCE_COUNT ticks?
        if ((history_ & DEBOUNCE_MASK) == 0) {
            state_ = 0;
            return true;
        }

        // Repeat support is limited to the 4 directional buttons
        if (repeat_enabled_) {
            // Has button been held continuously for DEBOUNCE_REPEAT_DELAY?
            if (history_ == 0xFF) {
                if (++held_time_ == REPEAT_INITIAL_DELAY) {
                    // Delay reached; trigger repeat code on NEXT tick
                    repeat_ctr_ = 1;
                    held_time_ = 0;
                }
            } else {
                // Button not continuously pressed; reset counter
                held_time_ = 0;
            }
        }
    }
    return false;
}
