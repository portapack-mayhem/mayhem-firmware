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
        if ((history_ == 0xFF) && !long_press_enabled_) {
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

            // If long_press_enabled_, state() function masks the button press until it's released
            // or until LONG_PRESS_DELAY is reached
            if (long_press_enabled_) {
                pulse_upon_release_ = true;
                return false;
            }
            return true;
        }
    } else {
        // Previous button state was 1 (pressed);
        // Has button been released for DEBOUNCE_COUNT ticks?
        if ((history_ & DEBOUNCE_MASK) == 0) {
            // Button has been released when long_press_enabled_ and before LONG_PRESS_DELAY was reached;
            // allow state() function to finally return a single press indication
            // (in long press mode, apps won't see button press until the button is released)
            if (pulse_upon_release_) {
                // leaving state_==1 for one cycle
                pulse_upon_release_ = 0;
            } else {
                state_ = 0;
            }

            // Reset long_press_occurred_ flag after button is released
            long_press_occurred_ = false;
            return true;
        }

        // Has button been held continuously?
        if (history_ == 0xFF) {
            held_time_++;
            if (pulse_upon_release_) {
                // Button is being held down and long_press support is enabled for this key:
                // if LONG_PRESS_DELAY is reached then finally report that switch is pressed and set flag
                // indicating it was a LONG press
                // (note that repeat_support and long_press support are mutually exclusive)
                if (held_time_ >= LONG_PRESS_DELAY) {
                    long_press_occurred_ = true;
                    pulse_upon_release_ = 0;
                    held_time_ = 0;
                    return true;
                }
            } else if (repeat_enabled_ && !long_press_enabled_) {
                // Repeat support -- 4 directional buttons only (unless long_press is enabled)
                if (held_time_ == REPEAT_INITIAL_DELAY) {
                    // Delay reached; trigger repeat code on NEXT tick
                    repeat_ctr_ = 1;
                    held_time_ = 0;
                }
            }
        } else {
            // Button not continuously pressed; reset counter
            held_time_ = 0;
        }
    }
    return false;
}
