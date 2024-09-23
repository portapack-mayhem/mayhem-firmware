/*
 * Copyright (C) 2024 Bernd Herzog
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

#include <memory>
#include "pacman.hpp"
#include "irq_controls.hpp"

#pragma GCC diagnostic push
// external code, so ignore warnings
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic ignored "-Weffc++"
#include "playfield.hpp"
#pragma GCC diagnostic pop

std::unique_ptr<Playfield> _playfield;

void initialize(const standalone_application_api_t& api) {
    _api = &api;
}

void on_event(const uint32_t& events) {
    static bool wait_for_button_release{false};

    if (!_playfield) {
        _playfield = std::make_unique<Playfield>();
        _playfield->Init();
    }

    if (events & 1) {
        auto switches_raw = _api->swizzled_switches() & ((1 << (int)Switch::Right) | (1 << (int)Switch::Left) | (1 << (int)Switch::Down) | (1 << (int)Switch::Up) | (1 << (int)Switch::Sel) | (1 << (int)Switch::Dfu));
        auto switches_debounced = _api->get_switches_state();

        // For the Select (Start/Pause) button, wait for release to avoid repeat
        uint8_t buttons_to_wait_for = (1 << (int)Switch::Sel);
        if (wait_for_button_release) {
            if ((switches_debounced & buttons_to_wait_for) == 0)
                wait_for_button_release = false;
            switches_debounced &= ~buttons_to_wait_for;
        } else {
            if (switches_debounced & buttons_to_wait_for)
                wait_for_button_release = true;
        }

        // For the directional buttons, use the raw inputs for fastest response time
        but_RIGHT = (switches_raw & (1 << (int)Switch::Right)) != 0;
        but_LEFT = (switches_raw & (1 << (int)Switch::Left)) != 0;
        but_DOWN = (switches_raw & (1 << (int)Switch::Down)) != 0;
        but_UP = (switches_raw & (1 << (int)Switch::Up)) != 0;

        // For the pause button, use the debounced input to avoid glitches, and OR in the value to make sure that we don't clear it before it's seen
        but_A |= (switches_debounced & (1 << (int)Switch::Sel)) != 0;

        _playfield->Step();
    }
}

void shutdown() {
    _playfield.reset();
}
