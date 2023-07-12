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

#include "ui_text.hpp"

namespace ui {

Glyph Font::glyph(const char c) const {
    size_t index;

    if (c < c_start) {
        // Non-display C0 Control characters - map to blank (index 0)
        return {w, h, data};
    }

    // Handle gap in glyphs table for C1 Control characters 0x80-0x9F
    if (c < C1_CONTROL_CHARS_START) {
        // ASCII chars <0x80:
        index = c - c_start;
    } else if (c >= C1_CONTROL_CHARS_START + C1_CONTROL_CHARS_COUNT) {
        // Latin 1 chars 0xA0-0xFF
        index = c - c_start - C1_CONTROL_CHARS_COUNT;
    } else {
        // C1 Control characters - map to blank
        return {w, h, data};
    }

    if (index >= c_count) {  // Latin Extended characters > 0xFF - not supported
        return {w, h, data};
    } else {
        return {w, h, &data[index * data_stride]};
    }
}

Dim Font::line_height() const {
    return h;
}

Dim Font::char_width() const {
    return w;
}

Size Font::size_of(const std::string s) const {
    Size size;

    for (const auto c : s) {
        const auto glyph_data = glyph(c);
        size = {
            size.width() + glyph_data.w(),
            std::max(size.height(), glyph_data.h())};
    }

    return size;
}

} /* namespace ui */
