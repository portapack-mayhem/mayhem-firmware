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

#include "ui_font_fixed_8x16.hpp"

#include <cstdint>

namespace ui {
namespace font {
ui::Font* fixed_8x16_font = nullptr;

ui::Font& fixed_8x16() {
    if (fixed_8x16_font == nullptr) {
        fixed_8x16_font = new ui::Font{
            8,
            16,
            _api->fixed_8x16_glyph_data,
            0x20,
            223,
        };
    }

    return *fixed_8x16_font;
}

} /* namespace font */
} /* namespace ui */
