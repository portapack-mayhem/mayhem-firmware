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

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"

uint8_t Debounce::state() {
    uint8_t v = state_to_report_;
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
                repeat_ctr_ = portapack::persistent_memory::ui_button_repeat_speed() ? REPEAT_SUBSEQUENT_DELAY_FAST / 2 : REPEAT_SUBSEQUENT_DELAY_NORMAL / 2;
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
            if (held_time_ >= (portapack::persistent_memory::ui_button_long_press_delay() ? LONG_PRESS_DELAY_FAST : LONG_PRESS_DELAY_NORMAL)) {
                pulse_upon_release_ = false;
                long_press_occurred_ = true;
                state_to_report_ = 1;
                return true;
            }
        } else if (repeat_enabled_ && !long_press_enabled_) {
            // Repeat support -- 4 directional buttons only (unless long_press is enabled)
            if (held_time_ >= (portapack::persistent_memory::ui_button_repeat_delay() ? REPEAT_INITIAL_DELAY_FAST : REPEAT_INITIAL_DELAY_NORMAL)) {
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

uint8_t EncoderDebounce::state() {
    return state_;
}

uint8_t EncoderDebounce::rotation_rate() {
    return last_rotation_rate_;
}

// Returns TRUE if encoder position phase bits changed (after debouncing)
bool EncoderDebounce::feed(const uint8_t phase_bits) {
    // Shift in new 2-bit sample into 32-bit history (16 samples total)
    history_ = (history_ << 2) | phase_bits;

    // For very noisy encoders: require BOTH bits stable for N consecutive ticks
    // Get configurable debounce window (4-32ms)
    uint8_t debounce_samples = portapack::persistent_memory::encoder_debounce_ms();

    // Build expected pattern: phase_bits repeated debounce_samples times
    // For phase_bits=0b00: 0x00000000, 0b01: 0x55555555, 0b10: 0xAAAAAAAA, 0b11: 0xFFFFFFFF
    uint32_t expected_stable_history = 0;
    for (uint8_t i = 0; i < debounce_samples; i++) {
        expected_stable_history = (expected_stable_history << 2) | phase_bits;
    }

    // Create mask for the number of samples we're checking
    uint32_t mask = 0;
    for (uint8_t i = 0; i < debounce_samples; i++) {
        mask = (mask << 2) | 0x3;
    }

    // Require exact match - both bits must be stable for configured ms
    if ((history_ & mask) == expected_stable_history) {
        // Has the debounced input value changed?
        if (state_ != phase_bits) {
            state_ = phase_bits;

            // Rate multiplier is for larger delta increments when dial is rotated rapidly.
            last_rotation_rate_ = rotation_rate_downcounter_;
            rotation_rate_downcounter_ = portapack::persistent_memory::encoder_rate_multiplier();
            return true;
        }
    }

    // Unstable input, or no change.
    // Decrement rotation rate detector once per timer tick.
    if (rotation_rate_downcounter_ > 1)
        rotation_rate_downcounter_--;
    return false;
}
