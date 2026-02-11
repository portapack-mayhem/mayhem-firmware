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

#ifndef __LED_H__
#define __LED_H__

#include "gpio.hpp"

struct LED {
    constexpr LED(const GPIO gpio)
        : _gpio{gpio} {
    }

    void setup() const {
#ifdef PRALINE
        /* PRALINE LEDs are active-low (GPIO LOW = LED ON) */
        _gpio.set(); /* Start with LED OFF (HIGH) */
#else
        _gpio.clear();
#endif
        _gpio.output();
        _gpio.configure();
    }

    void on() const {
#ifdef PRALINE
        _gpio.clear(); /* LOW = ON for PRALINE */
#else
        _gpio.set();
#endif
    }

    void off() const {
#ifdef PRALINE
        _gpio.set(); /* HIGH = OFF for PRALINE */
#else
        _gpio.clear();
#endif
    }

    void toggle() const {
        _gpio.toggle();
    }

    void write(const bool value) const {
#ifdef PRALINE
        _gpio.write(!value); /* Invert for PRALINE */
#else
        _gpio.write(value);
#endif
    }

   private:
    const GPIO _gpio;
};

#endif /*__LED_H__*/
