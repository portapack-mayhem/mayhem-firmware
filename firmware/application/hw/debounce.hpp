/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2024 Mark Thompson
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
#define LONG_PRESS_DELAY 800

class Debounce {
   public:
    bool feed(const uint8_t bit);
    uint8_t state();
    void enable_repeat();
    bool get_long_press_enabled() const;
    void set_long_press_enabled(bool v);
    bool long_press_occurred();

   private:
    uint8_t history_{0};  // shift register of last 8 reads from button hardware state bit

    uint8_t state_{0};  // actual button hardware state (after debounce logic), 1=pressed

    uint8_t state_to_report_{0};  // pseudo button state reported by state() function (may be masked off or simulated presses)

    bool repeat_enabled_{false};  // TRUE if this button is enabled to auto-repeat when held down (ignored if long_press_enabled)

    uint16_t repeat_ctr_{0};  // used for timing auto-repeat simulated button presses when button is held down and repeat_enabled

    uint16_t held_time_{0};  // number of ticks that the button has been held down (compared against REPEAT and LONG_PRESS delays)

    bool pulse_upon_release_{false};  // TRUE when button is being held down when long_press_enabled and LONG_PRESS_DELAY hasn't been reached yet

    bool simulated_pulse_{false};  // TRUE if a simulated button press is active following a short button press (only when long_press_enabled)

    bool long_press_enabled_{false};  // TRUE when button is in long-press mode (takes precedence over the repeat_enabled flag)

    bool long_press_occurred_{false};  // TRUE when button is being held down and LONG_PRESS_DELAY has been reached (only when long_press_enabled)
};

class EncoderDebounce {
   public:
    bool feed(const uint8_t phase_bits);  // returns TRUE if state changed after debouncing

    uint8_t state();  // returns debounced phase bits from encoder

    uint8_t rotation_rate();  // returns last rotation rate

   private:
    uint8_t history_{0};  // shift register of previous reads from encoder

    uint8_t state_{0};  // actual encoder output state (after debounce logic)

    uint8_t last_rotation_rate_{1};
    uint8_t rotation_rate_downcounter_{1};  // down-counter to estimate rotation speed
};

#endif /*__DEBOUNCE_H__*/
