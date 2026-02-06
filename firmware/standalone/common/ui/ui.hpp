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

#ifndef __UI_H__
#define __UI_H__

#include <cstdint>

namespace ui {

// Escape sequences for colored text; second character is index into term_colors[]
#define STR_COLOR_BLACK "\x1B\x00"
#define STR_COLOR_DARK_BLUE "\x1B\x01"
#define STR_COLOR_DARK_GREEN "\x1B\x02"
#define STR_COLOR_DARK_CYAN "\x1B\x03"
#define STR_COLOR_DARK_RED "\x1B\x04"
#define STR_COLOR_DARK_MAGENTA "\x1B\x05"
#define STR_COLOR_DARK_YELLOW "\x1B\x06"
#define STR_COLOR_LIGHT_GREY "\x1B\x07"
#define STR_COLOR_DARK_GREY "\x1B\x08"
#define STR_COLOR_BLUE "\x1B\x09"
#define STR_COLOR_GREEN "\x1B\x0A"
#define STR_COLOR_CYAN "\x1B\x0B"
#define STR_COLOR_RED "\x1B\x0C"
#define STR_COLOR_MAGENTA "\x1B\x0D"
#define STR_COLOR_YELLOW "\x1B\x0E"
#define STR_COLOR_WHITE "\x1B\x0F"
#define STR_COLOR_FOREGROUND "\x1B\x10"

#define DEG_TO_RAD(d) (d * (2 * pi) / 360.0)

using Coord = int16_t;
using Dim = int16_t;

constexpr uint16_t screen_width = 240;
constexpr uint16_t screen_height = 320;

/* Dimensions for the default font character. */
constexpr uint16_t char_width = 8;
constexpr uint16_t char_height = 16;

struct Color {
    uint16_t v;  // rrrrrGGGGGGbbbbb

    constexpr Color()
        : v{0} {
    }

    constexpr Color(
        uint16_t v)
        : v{v} {
    }

    constexpr Color(
        uint8_t r,
        uint8_t g,
        uint8_t b)
        : v{
              static_cast<uint16_t>(
                  ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3))} {
    }

    uint8_t r() {
        return (uint8_t)((v >> 8) & 0xf8);
    }

    uint8_t g() {
        return (uint8_t)((v >> 3) & 0xfc);
    }

    uint8_t b() {
        return (uint8_t)((v << 3) & 0xf8);
    }

    uint8_t to_greyscale() {
        uint32_t r = (v >> 8) & 0xf8;
        uint32_t g = (v >> 3) & 0xfc;
        uint32_t b = (v << 3) & 0xf8;

        uint32_t grey = ((r * 306) + (g * 601) + (b * 117)) >> 10;

        return (uint8_t)grey;
    }

    uint16_t dark() {
        // stripping bits 4 & 5 from each of the colors R/G/B
        return (v & ((0xc8 << 8) | (0xcc << 3) | (0xc8 >> 3)));
    }

    Color operator-() const {
        return (v ^ 0xffff);
    }

    /* Converts a 32-bit color into a 16-bit color.
     * High byte is ignored. */
    static constexpr Color RGB(uint32_t rgb) {
        return {static_cast<uint8_t>((rgb >> 16) & 0xff),
                static_cast<uint8_t>((rgb >> 8) & 0xff),
                static_cast<uint8_t>(rgb & 0xff)};
    }

    static constexpr Color black() {
        return {0, 0, 0};
    }

    static constexpr Color red() {
        return {255, 0, 0};
    }
    static constexpr Color dark_red() {
        return {159, 0, 0};
    }

    static constexpr Color orange() {
        return {255, 175, 0};
    }
    static constexpr Color dark_orange() {
        return {191, 95, 0};
    }

    static constexpr Color yellow() {
        return {255, 255, 0};
    }
    static constexpr Color dark_yellow() {
        return {191, 191, 0};
    }

    static constexpr Color green() {
        return {0, 255, 0};
    }
    static constexpr Color dark_green() {
        return {0, 159, 0};
    }

    static constexpr Color blue() {
        return {0, 0, 255};
    }
    static constexpr Color dark_blue() {
        return {0, 0, 191};
    }

    static constexpr Color cyan() {
        return {0, 255, 255};
    }
    static constexpr Color dark_cyan() {
        return {0, 191, 191};
    }

    static constexpr Color magenta() {
        return {255, 0, 255};
    }
    static constexpr Color dark_magenta() {
        return {191, 0, 191};
    }

    static constexpr Color white() {
        return {255, 255, 255};
    }

    static constexpr Color light_grey() {
        return {191, 191, 191};
    }
    static constexpr Color grey() {
        return {127, 127, 127};
    }
    static constexpr Color dark_grey() {
        return {63, 63, 63};
    }
    static constexpr Color darker_grey() {
        return {31, 31, 31};
    }

    static constexpr Color purple() {
        return {204, 0, 102};
    }
};

extern Color term_colors[16];

struct ColorRGB888 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Point {
   private:
    Coord _x;
    Coord _y;

   public:
    constexpr Point()
        : _x{0},
          _y{0} {
    }

    constexpr Point(
        int x,
        int y)
        : _x{static_cast<Coord>(x)},
          _y{static_cast<Coord>(y)} {
    }

    constexpr int x() const {
        return _x;
    }

    constexpr int y() const {
        return _y;
    }

    constexpr Point operator-() const {
        return {-_x, -_y};
    }

    constexpr Point operator+(const Point& p) const {
        return {_x + p._x, _y + p._y};
    }

    constexpr Point operator-(const Point& p) const {
        return {_x - p._x, _y - p._y};
    }

    Point& operator+=(const Point& p) {
        _x += p._x;
        _y += p._y;
        return *this;
    }

    Point& operator-=(const Point& p) {
        _x -= p._x;
        _y -= p._y;
        return *this;
    }
};

struct Size {
   private:
    Dim _w;
    Dim _h;

   public:
    constexpr Size()
        : _w{0},
          _h{0} {
    }

    constexpr Size(
        int w,
        int h)
        : _w{static_cast<Dim>(w)},
          _h{static_cast<Dim>(h)} {
    }

    int width() const {
        return _w;
    }

    int height() const {
        return _h;
    }

    bool is_empty() const {
        return (_w < 1) || (_h < 1);
    }
};

struct Rect {
   private:
    Point _pos;
    Size _size;

   public:
    constexpr Rect()
        : _pos{},
          _size{} {
    }

    constexpr Rect(
        int x,
        int y,
        int w,
        int h)
        : _pos{x, y},
          _size{w, h} {
    }

    constexpr Rect(
        Point pos,
        Size size)
        : _pos(pos),
          _size(size) {
    }

    Point location() const {
        return _pos;
    }

    Size size() const {
        return _size;
    }

    int top() const {
        return _pos.y();
    }

    int bottom() const {
        return _pos.y() + _size.height();
    }

    int left() const {
        return _pos.x();
    }

    int right() const {
        return _pos.x() + _size.width();
    }

    int width() const {
        return _size.width();
    }

    int height() const {
        return _size.height();
    }

    Point center() const {
        return {_pos.x() + _size.width() / 2, _pos.y() + _size.height() / 2};
    }

    bool is_empty() const {
        return _size.is_empty();
    }

    bool contains(const Point p) const;

    Rect intersect(const Rect& o) const;

    Rect operator+(const Point& p) const {
        return {_pos + p, _size};
    }

    Rect& operator+=(const Rect& p);
    Rect& operator+=(const Point& p);
    Rect& operator-=(const Point& p);

    operator bool() const {
        return !_size.is_empty();
    }
};

struct Bitmap {
    const Size size;
    const uint8_t* const data;
};

enum class KeyEvent : uint8_t {
    /* Ordinals map to bit positions reported by CPLD */
    Right = 0,
    Left = 1,
    Down = 2,
    Up = 3,
    Select = 4,
    Dfu = 5,
    Back = 6, /* Left and Up together */
};

using EncoderEvent = int32_t;
using KeyboardEvent = uint8_t;

struct TouchEvent {
    enum class Type : uint32_t {
        Start = 0,
        Move = 1,
        End = 2,
    };

    Point point;
    Type type;
};

Point polar_to_point(float angle, uint32_t distance);

Point fast_polar_to_point(int32_t angle, uint32_t distance);

/* Default font glyph size. */
constexpr Size char_size{char_width, char_height};

bool key_is_long_pressed(KeyEvent key);

} /* namespace ui */

#endif /*__UI_H__*/
