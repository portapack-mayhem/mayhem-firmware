/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#ifndef __TONE_KEY_H_
#define __TONE_KEY_H_

#include <cstdint>
#include <string>
#include <vector>

namespace tonekey {

#define TONE_FREQ_TOLERANCE_CENTIHZ (4 * 100)
#define TONE_DISPLAY_TOGGLE_COUNTER 3
#define F2Ix100(x) (int32_t)(x * 100.0 + 0.5)  // add 0.5f to round vs truncate during FP->int conversion

using tone_index = int32_t;
using tone_key_t = std::vector<std::pair<std::string, uint32_t>>;

extern const tone_key_t tone_keys;

float tone_key_frequency(tone_index index);

std::string fx100_string(uint32_t f);
std::string tone_key_string(tone_index index);
std::string tone_key_value_string(tone_index index);
std::string tone_key_string_by_value(uint32_t value, size_t max_length);
tone_index tone_key_index_by_value(uint32_t value);

}  // namespace tonekey

#endif /*__TONE_KEY_H_*/
