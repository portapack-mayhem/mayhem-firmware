/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Mark Thompson
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

uint8_t Debounce::state() {
    bool v = state_to_report_;
    simulated_pulse_ = false;
    return v;
}

void Debounce::enable_repeat() {
    repeat_enabled_ = true;
}

bool Debounce::get_long_press_enabled() const {
    return long_press_enabled_;
}

void Debounce::set_long_press_enabled(bool v) {
    long_press_enabled_ = v;
}

bool Debounce::long_press_occurred() {
    bool v = long_press_occurred_;
    long_press_occurred_ = false;
    return v;
}

// Returns TRUE if button state changed (after debouncing)
bool Debounce::feed(const uint8_t bit) {
    history_ = (history_ << 1) | (bit & 1);

    // Has button been held for DEBOUNCE_COUNT ticks?
    if ((history_ & DEBOUNCE_MASK) == DEBOUNCE_MASK) {
        //
        // Button is currently pressed;
        // Was previous button state 0 (released)?
        //
        if (state_ == 0) {
            //
            // Button has been pressed (after filtering glitches), transition 0->1
            //
            state_ = 1;

            // If long_press_enabled_, state() function masks the button press until it's released
            // or until LONG_PRESS_DELAY is reached
            if (long_press_enabled_) {
                pulse_upon_release_ = true;
                state_to_report_ = 0;
                return false;
            }
            state_to_report_ = 1;
            return true;
        }

        // "Repeat" handling - simulated button release
        if (repeat_ctr_ && !long_press_enabled_) {
            // Simulate button press every REPEAT_SUBSEQUENT_DELAY ticks
            // (by toggling reported state every 1/2 of the delay time)
            if (--repeat_ctr_ == 0) {
                state_to_report_ = !state_to_report_;
                repeat_ctr_ = REPEAT_SUBSEQUENT_DELAY / 2;
                return true;
            }
        }

        // Keep track of how long button has been held
        held_time_++;
        if (pulse_upon_release_) {
            // Button is being held down and long_press support is enabled for this key:
            // if LONG_PRESS_DELAY is reached then finally report that switch is pressed and set flag
            // indicating it was a LONG press
            // (note that repeat_support and long_press support are mutually exclusive)
            if (held_time_ >= LONG_PRESS_DELAY) {
                pulse_upon_release_ = false;
                long_press_occurred_ = true;
                state_to_report_ = 1;
                return true;
            }
        } else if (repeat_enabled_ && !long_press_enabled_) {
            // Repeat support -- 4 directional buttons only (unless long_press is enabled)
            if (held_time_ >= REPEAT_INITIAL_DELAY) {
                // Delay reached; trigger repeat code on NEXT tick
                repeat_ctr_ = 1;
                held_time_ = 0;
            }
        }
    } else if ((history_ & DEBOUNCE_MASK) == 0) {  // Has button been released for at least DEBOUNCE_COUNT ticks?
        //
        // Button is released;
        // Was previous button state 1 (pressed)?
        //
        if (state_ == 1) {
            //
            // Button has been released (after filtering glitches), transition 1->0
            //
            state_ = 0;
            long_press_occurred_ = false;

            // If button released when long_press_enabled_ and before LONG_PRESS_DELAY was reached;
            // allow state() function to finally return a single press indication (simulated pulse).
            // Note: In long press mode, apps won't see button press until the button is released.
            if (pulse_upon_release_) {
                pulse_upon_release_ = false;
                simulated_pulse_ = true;
                state_to_report_ = 1;
                return true;
            }

            state_to_report_ = 0;
            return true;
        }

        // Reset reported state after application/event has cleared simulated_pulse_
        if (state_to_report_ == 1 && !simulated_pulse_) {
            state_to_report_ = 0;
            return true;
        }
    } else {
        //
        // Button is in transition between states;
        // Reset counters until button inputs are stable.
        //
        held_time_ = 0;
        repeat_ctr_ = 0;
    }

    return false;
}
