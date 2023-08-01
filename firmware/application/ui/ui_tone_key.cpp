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

#include "ui_tone_key.hpp"
#include <utility>

using namespace ui;

namespace tonekey {

void tone_keys_populate(OptionsField& field) {
    OptionsField::options_t tone_key_options;
    std::string tone_name;

    for (size_t c = 0; c < tone_keys.size(); c++) {
        auto f = tone_keys[c].second;
        if ((c != 0) && (f < 1000 * 100))
            tone_name = "CTCSS " + fx100_string(f) + " #" + tone_keys[c].first;
        else
            tone_name = tone_keys[c].first;

        tone_key_options.emplace_back(tone_name, c);
    }

    field.set_options(std::move(tone_key_options));
}

}  // namespace tonekey
