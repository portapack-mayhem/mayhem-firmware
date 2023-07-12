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

#ifndef __UI_TEXT_H__
#define __UI_TEXT_H__

#include <cstdint>
#include <cstddef>
#include <string>

#include "ui.hpp"

// C1 Control Characters are an unprintable range of glyphs missing from the font files
#define C1_CONTROL_CHARS_START 0x80
#define C1_CONTROL_CHARS_COUNT 32

namespace ui {

class Glyph {
   public:
    constexpr Glyph(
        Dim w,
        Dim h,
        const uint8_t* const pixels)
        : w_{static_cast<uint8_t>(w)},
          h_{static_cast<uint8_t>(h)},
          pixels_{pixels} {
    }

    int w() const {
        return w_;
    }

    int h() const {
        return h_;
    }

    Size size() const {
        return {w_, h_};
    }

    Point advance() const {
        return {w_, 0};
    }

    const uint8_t* pixels() const {
        return pixels_;
    }

   private:
    const uint8_t w_;
    const uint8_t h_;
    const uint8_t* const pixels_;
};

class Font {
   public:
    constexpr Font(
        Dim w,
        Dim h,
        const uint8_t* data,
        char c_start,
        size_t c_count)
        : w{w},
          h{h},
          data{data},
          c_start{c_start},
          c_count{c_count},
          data_stride{(w * h + 7U) >> 3} {
    }

    Glyph glyph(const char c) const;

    Dim line_height() const;
    Dim char_width() const;

    Size size_of(const std::string s) const;

   private:
    const Dim w;
    const Dim h;
    const uint8_t* const data;
    const char c_start;
    const size_t c_count;
    const size_t data_stride;
};

} /* namespace ui */

#endif /*__UI_TEXT_H__*/
