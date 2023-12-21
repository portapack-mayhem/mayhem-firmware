/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __IRQ_CONTROLS_H__
#define __IRQ_CONTROLS_H__

#include <cstdint>
#include <bitset>

#include "touch.hpp"

// Must be same values as in ui::KeyEvent
// TODO: Why not just reuse one or the other?
enum class Switch : uint8_t {
    Right = 0,
    Left = 1,
    Down = 2,
    Up = 3,
    Sel = 4,
    Dfu = 5
};

// Index with the Switch enum.
using SwitchesState = std::bitset<6>;
using EncoderPosition = uint32_t;

void controls_init();
uint8_t swizzled_switches();
SwitchesState get_switches_state();
EncoderPosition get_encoder_position();
touch::Frame get_touch_frame();

SwitchesState get_switches_long_press_config();
void set_switches_long_press_config(SwitchesState switch_config);
bool switch_is_long_pressed(Switch s);

namespace control {
namespace debug {

uint8_t switches();
void inject_switch(uint8_t);

}  // namespace debug
}  // namespace control

#endif /*__IRQ_CONTROLS_H__*/
