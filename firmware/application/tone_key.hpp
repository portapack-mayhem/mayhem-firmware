/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __TONE_KEY_H_
#define __TONE_KEY_H_

#include "ui.hpp"
#include "ui_widget.hpp"

using namespace ui;

namespace tonekey {

using tone_key_t = std::vector<std::pair<std::string, float>>;

extern const tone_key_t tone_keys;

void tone_keys_populate(OptionsField& field);
float tone_key_frequency(const uint32_t index);

}

#endif/*__TONE_KEY_H_*/
