/*
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

// "HAL" layer for Tetris code to run on PortaPack without its original mbed OS
// (the dream here was to avoid modifying the original code)

#ifndef __UI_mbed_H__
#define __UI_mbed_H__

using Callback = void (*)(void);

#define wait_us(x) (void)0
#define wait(x) chThdSleepMilliseconds(x * 1000)
#define PullUp 1

enum {
    dp0,
    dp1,
    dp2,
    dp3,
    dp4,
    dp5,
    dp6,
    dp7,
    dp8,
    dp9,
    dp10,
    dp11,
    dp12,
    dp13,
    dp14,
    dp15,
    dp16,
    dp17,
    dp18,
    dp19,
    dp20,
    dp21,
    dp22,
    dp23,
    dp24,
    dp25,
};

static bool but_RIGHT;
static bool but_LEFT;
static bool but_UP;
static bool but_DOWN;
static bool but_SELECT;

//
// AnalogIn Class -- DID NOT WORK BECAUSE INITIALIZER CODE WON'T EXECUTE -- hacked original code module instead
//
// dp9 = joystick rotate button --> select button
// dp10 = joystick y --> up & down buttons
// dp11 = joystick x --> left & right buttons
// dp13 = random number generator
//
//
// class AnalogIn {
//    public:
//     AnalogIn(uint32_t analog_input) {
//         // FIXME - THIS CODE NEVER GETS EXECUTED!
//         analog_input_ = analog_input;
//     };
//
//     // Tetris code only uses this function for dp13 - supposed to be a random number
//     uint16_t read_u16() {
//         return std::rand();
//     };
//
//     // Tetris code uses read() function for direction buttons only
//     float read() {
//         float retval = 0.5;
//         switch (analog_input_) {
//             case dp11:
//                 if (but_LEFT)
//                     retval = 0.0;
//                 else if (but_RIGHT)
//                     retval = 1.0;
//                 break;
//
//             case dp10:
//                 if (but_UP)
//                     retval = 0.0;
//                 else if (but_DOWN)
//                     retval = 1.0;
//                 break;
//         }
//         return retval;
//     };
//
//     operator float() {
//         return read();
//     };
//
//    private:
//     uint32_t analog_input_{INT_MAX};
// };

//
// Timer Class
// (Timer object was used for unneeded button debouncing, so just returning 1000ms to indicate we've waited long enough)
//
class Timer {
   public:
    // NOTE: INITIALIZER CODE WON'T RUN
    Timer() { (void)0; };
    void reset() { (void)0; };
    void start() { (void)0; }
    uint32_t read_ms() { return 1000; };

   private:
};

//
// Ticker Class
// (Ticker is timed callback, used for checking "joystick" directional switches and when time to move piece down a row)
//
// NB: Only one callback is supported per Ticker class instantiation
static Callback fall_timer_callback;
static uint32_t fall_timer_timeout;
static uint32_t fall_timer_counter;

static Callback dir_button_callback;

static void check_fall_timer() {
    if (fall_timer_callback) {
        if (++fall_timer_counter >= fall_timer_timeout) {
            fall_timer_counter = 0;
            fall_timer_callback();
        }
    }
}

class Ticker {
   public:
    // NOTE: INITIALIZER CODE WON'T RUN
    Ticker() { (void)0; };

    void attach(Callback func, double delay_sec) {
        // 0.3 sec is requested only for button check -- kludge to use on_key callback for this one instead of timer
        if (delay_sec == 0.3) {
            dir_button_callback = func;
        } else {
            fall_timer_callback = func;
            fall_timer_timeout = delay_sec * 60;  // timer interrupts at 60 Hz
        }
    }

    void detach() {
        // shouldn't detach both, but don't know how to tell which object is which
        dir_button_callback = nullptr;
        fall_timer_callback = nullptr;
    }

   private:
};

//
// InterruptIn Class
// (just used for the Select button)
//
static Callback sel_button_callback;

static bool check_encoder(const EncoderEvent delta) {
    (void)delta;
    // TODO: consider adding ability to rotate Tetronimo via encoder too
    return false;
}

static bool check_key(const KeyEvent key) {
    auto switches_debounced = get_switches_state().to_ulong();
    but_RIGHT = (switches_debounced & 0x01) != 0;
    but_LEFT = (switches_debounced & 0x02) != 0;
    but_DOWN = (switches_debounced & 0x04) != 0;
    but_UP = (switches_debounced & 0x08) != 0;
    but_SELECT = (switches_debounced & 0x10) != 0;

    if (key == KeyEvent::Select) {
        if (sel_button_callback)
            sel_button_callback();
    } else {
        if (dir_button_callback)
            dir_button_callback();
    }
    return true;
}

class InterruptIn {
   public:
    InterruptIn(int reg) {
        // NOTE: INITIALIZER CODE WON'T RUN
        (void)reg;
    };
    void fall(Callback func) { sel_button_callback = func; };
    void mode(int v) { (void)v; };

   private:
};

#endif /*__UI_mbed_H__*/
