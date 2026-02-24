/*
 * Copyright (C) 2023 Kyle Reed
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

#include "ui_font_fixed_5x8.hpp"
#include <cstdint>
#include "standalone_app.hpp"

namespace ui {
namespace font {
ui::Font* fixed_5x8_font = nullptr;

ui::Font& fixed_5x8() {
    if (fixed_5x8_font == nullptr) {
        fixed_5x8_font = new ui::Font{
            5,
            8,
            _api->fixed_5x8_glyph_data,
            0x20,
            95,
        };
    }

    return *fixed_5x8_font;
}

} /* namespace font */
} /* namespace ui */
