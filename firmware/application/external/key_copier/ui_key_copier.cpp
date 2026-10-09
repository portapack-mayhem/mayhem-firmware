/*
 * Copyright (C) 2026 osahv
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
 *
 * Key Copier is a port of KeyCopier for Flipper Zero (https://github.com/zinongli/KeyCopier),
 * Copyright (c) 2024 zinongli, MIT License (see LICENSE-KeyCopier-MIT in this folder).
 * The key format table and the contour algorithm come from there.
 */

#include "ui_key_copier.hpp"

#include "ui_font_fixed_8x16.hpp"
#include "string_format.hpp"
#include "portapack.hpp"

using namespace ui;

namespace ui::external_app::key_copier {

namespace {
// Calibration for the H2 screen: visible area 50 x 65 mm over 240 x 320 px = ~0.205 mm per pixel.
constexpr int kUnitsPerPx = 81;  // 0.0081 inch per pixel, in 1/10000 inch
constexpr int kBaseY = 180;
constexpr int kBaseYLand = 128;  // landscape: blade bottom, text above and below
constexpr int kTipMargin = 22;   // landscape: space right of the drawn tip (~4.5 mm)
constexpr Color kBg = Color::white();
constexpr Color kPin = Color::red();

inline void text(Painter& p, Point at, Color fg, std::string_view s) {
    p.draw_string(at, Style{.font = ui::font::fixed_8x16, .background = kBg, .foreground = fg}, s);
}

inline int mn(int a, int b) {
    return a < b ? a : b;
}
inline int mx(int a, int b) {
    return a > b ? a : b;
}
}  // namespace

int KeyCopierView::px(int units) const {
    return (units + kUnitsPerPx / 2) / kUnitsPerPx;  // units are never negative
}

// v * tangent, rounded like the original (int)(v * tan + 0.5f)
static int tan_mul(int v, int tangent) {
    return (v * tangent + 500) / 1000;
}

int KeyCopierView::rel_depth(int pin) const {
    const auto& f = all_formats[format_index_];
    if (pin < 0 || pin >= f.pin_num) return 0;
    return depth_[pin] - f.min_depth;
}

Point KeyCopierView::rot_point(int x, int y) const {
    // Portrait (0/180): local = screen coordinates, 180 flips around the blade centre.
    // Landscape (90/270): local = coordinates of the turned device (320 wide, 240 high), mapped to the real screen.
    switch (rot_ & 3) {
        case 1:
            return {screen_width - 1 - y, x};  // device turned counter-clockwise
        case 3:
            return {y, screen_height - 1 - x};  // device turned clockwise
        case 2:
            return {2 * cx_ - x, 2 * cy_ - y};
        default:
            return {x, y};
    }
}

void KeyCopierView::text_at(Painter& p, int x, int y, Color fg, std::string_view s) {
    if (!(rot_ & 1)) {
        text(p, {x, y}, fg, s);
        return;
    }
    // Landscape: draw each glyph rotated, one screen row (16 px) at a time to keep the M0 stack usage tiny.
    const auto& font = ui::font::fixed_8x16;
    for (char c : s) {
        const auto g = font.glyph(c);
        const uint8_t* bits = g.pixels();
        auto on = [&](int gx, int gy) { return (bits[(gy * 8 + gx) >> 3] >> ((gy * 8 + gx) & 7)) & 1; };
        for (int gx = 0; gx < 8; gx++) {
            Color row[16];
            int sx, sy;
            if (rot_ == 1) {  // reading direction = screen y, glyph "down" = screen x decreasing
                sx = screen_width - 1 - (y + 15);
                sy = x + gx;
                for (int col = 0; col < 16; col++) row[col] = on(gx, 15 - col) ? fg : kBg;
            } else {  // reading direction = screen y decreasing, glyph "down" = screen x increasing
                sx = y;
                sy = screen_height - 1 - (x + gx);
                for (int col = 0; col < 16; col++) row[col] = on(gx, col) ? fg : kBg;
            }
            if (sx < 0 || sy < 0 || sx + 16 > screen_width || sy >= screen_height) continue;  // clip
            portapack::display.draw_pixels({sx, sy, 16, 1}, row, 16);
        }
        x += 8;
    }
}

void KeyCopierView::line(Painter& p, int ax, int ay, int bx, int by) {
    const Point a = rot_point(ax, ay), b = rot_point(bx, by);
    int x0 = a.x(), y0 = a.y(), x1 = b.x(), y1 = b.y();
    if (y0 == y1) {
        if (x0 > x1) {
            int t = x0;
            x0 = x1;
            x1 = t;
        }
        p.fill_rectangle({x0, y0, x1 - x0 + 1, 1}, ink_);
        return;
    }
    if (x0 == x1) {
        if (y0 > y1) {
            int t = y0;
            y0 = y1;
            y1 = t;
        }
        p.fill_rectangle({x0, y0, 1, y1 - y0 + 1}, ink_);
        return;
    }
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;  // dy is negative
    int err = dx + dy;
    for (;;) {
        p.fill_rectangle({x0, y0, 1, 1}, ink_);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void KeyCopierView::select_format(int index) {
    format_index_ = (index + FORMAT_NUM) % FORMAT_NUM;
    const auto& f = all_formats[format_index_];
    for (int i = 0; i < kMaxPins; i++) depth_[i] = f.min_depth;
    pin_ = 0;
    set_dirty();
}

KeyCopierView::KeyCopierView(NavigationView& nav)
    : nav_{nav} {
    set_focusable(true);
    select_format(0);
}

void KeyCopierView::focus() {
    View::focus();
}

bool KeyCopierView::on_key(const KeyEvent key) {
    const auto& f = all_formats[format_index_];
    enum Act { None,
               PinPrev,
               PinNext,
               FmtPrev,
               FmtNext,
               Rotate } act = None;
    // The user turns the device, so the physical buttons are mapped to what the user sees:
    //   0:   normal              180: contour flipped, device not turned (left/right swap along the key)
    //   90:  device turned counter-clockwise (screen top is on the user's left)
    //   270: device turned clockwise (screen top is on the user's right)
    switch (rot_ & 3) {
        case 0:
            act = key == KeyEvent::Left ? PinPrev : key == KeyEvent::Right ? PinNext
                                                : key == KeyEvent::Up      ? FmtPrev
                                                : key == KeyEvent::Down    ? FmtNext
                                                                           : None;
            break;
        case 2:
            act = key == KeyEvent::Left ? PinNext : key == KeyEvent::Right ? PinPrev
                                                : key == KeyEvent::Up      ? FmtPrev
                                                : key == KeyEvent::Down    ? FmtNext
                                                                           : None;
            break;
        case 1:
            act = key == KeyEvent::Up ? PinPrev : key == KeyEvent::Down ? PinNext
                                              : key == KeyEvent::Right  ? FmtPrev
                                              : key == KeyEvent::Left   ? FmtNext
                                                                        : None;
            break;
        default:
            act = key == KeyEvent::Up ? PinNext : key == KeyEvent::Down ? PinPrev
                                              : key == KeyEvent::Right  ? FmtNext
                                              : key == KeyEvent::Left   ? FmtPrev
                                                                        : None;
            break;
    }
    if (key == KeyEvent::Select) act = Rotate;
    if (act == None) return false;

    // Moving past the end of the key on the side of the title bar hands the key to the system: focus moves to the
    // back arrow and Select then exits (Left on the first pin when upright, Up in landscape).
    const KeyEvent exit_key = (rot_ & 1) ? KeyEvent::Up : KeyEvent::Left;
    if (key == exit_key && ((act == PinPrev && pin_ == 0) || (act == PinNext && pin_ == f.pin_num - 1))) return false;

    switch (act) {
        case PinPrev:
            if (pin_ > 0) pin_--;
            break;
        case PinNext:
            if (pin_ < f.pin_num - 1) pin_++;
            break;
        case FmtPrev:
            select_format(format_index_ - 1);
            return true;
        case FmtNext:
            select_format(format_index_ + 1);
            return true;
        default:
            rot_ = (rot_ + 1) & 3;
            break;
    }
    set_dirty();
    return true;
}

bool KeyCopierView::on_encoder(const EncoderEvent delta) {
    const auto& f = all_formats[format_index_];
    int d = depth_[pin_] + (delta > 0 ? 1 : -1);
    if (d >= f.min_depth && d <= f.max_depth) depth_[pin_] = d;
    set_dirty();
    return true;
}

void KeyCopierView::paint(Painter& p) {
    const auto& f = all_formats[format_index_];
    p.fill_rectangle(screen_rect(), kBg);

    const int pin_half = px(f.pin_width) / 2;
    const int pin_step = px(f.pin_increment);
    const int uncut = px(f.uncut_depth);
    const int step = px(f.depth_step);
    const int level_len = px(f.last_pin + f.elbow);
    const bool land = (rot_ & 1) != 0;
    // Landscape: the right end is the key tip, so keep only a small margin there and leave the rest for the bow.
    const bool two_sided = f.sides == 2;
    const int right_ext = level_len + (two_sided ? 0 : px(f.elbow));  // real right end of the drawn contour
    const int x0 = land ? (screen_height - kTipMargin - right_ext) : (screen_width - right_ext) / 2;
    const int base_y = land ? kBaseYLand : kBaseY;
    const int top = base_y - uncut;
    const int bottom = base_y;  // used by double-sided keys
    cx_ = screen_width / 2;
    cy_ = base_y - uncut / 2;  // 180 degree flip happens around the middle of the blade
    const bool two = f.sides == 2;
    ink_ = Color::black();

    int pre_extra = 0, post_extra = 0, b_pre_extra = 0, b_post_extra = 0;
    for (int i = 0; i < f.pin_num; i++) {
        const int cx = x0 + px(f.first_pin + i * f.pin_increment);
        const int cur = rel_depth(i);
        const int last = (i == 0) ? 0 : rel_depth(i - 1);
        const int next = (i == f.pin_num - 1) ? 0 : rel_depth(i + 1);
        // depth in px, with the same rounding as the original: round(depth * step / inch_per_px)
        const int cur_px = px(cur * f.depth_step);
        const int last_px = px(last * f.depth_step);
        (void)step;

        // pin marker and value
        line(p, cx, top - 5, cx, top);
        line(p, cx - pin_half, top + cur_px, cx + pin_half, top + cur_px);

        if (two) {
            line(p, cx - pin_half, bottom - cur_px, cx + pin_half, bottom - cur_px);
            if (i == 0) {
                line(p, x0, bottom, cx - pin_half - cur_px, bottom);
                b_pre_extra = mx(cur_px + pin_half, 0);
            }
            if ((last + cur) > f.clearance) {
                if (i != 0) b_pre_extra = mn(mx(pin_step - b_post_extra, pin_half), pin_step - pin_half);
                line(p, cx - b_pre_extra, bottom - mx(tan_mul(cur_px - (b_pre_extra - pin_half), f.tangent), 0),
                     cx - pin_half, bottom - tan_mul(cur_px, f.tangent));
            } else {
                int up_start = cx - pin_half - cur_px;
                line(p, up_start, bottom, cx - pin_half, bottom - tan_mul(cur_px, f.tangent));
                line(p, mn(cx - pin_step + pin_half + last_px, up_start), bottom, up_start, bottom);
            }
            if ((cur + next) > f.clearance) {
                int prod = cur * pin_step / (cur + next);
                b_post_extra = mn(mx(prod, pin_half), pin_step - pin_half);
                line(p, cx + pin_half, bottom - cur_px, cx + b_post_extra,
                     bottom - mx(cur_px - tan_mul(b_post_extra - pin_half, f.tangent), 0));
            } else {
                line(p, cx + pin_half, bottom - tan_mul(cur_px, f.tangent), cx + pin_half + cur_px, bottom);
            }
        }

        if (i == 0) {
            line(p, x0, top, cx - pin_half - cur_px, top);  // shoulder
            pre_extra = mx(cur_px + pin_half, 0);
            if (two)
                line(p, x0, bottom, cx - pin_half - cur_px, bottom);
            else
                line(p, x0, base_y, x0 + level_len, base_y);  // flat bottom of the blade
        }
        if ((last + cur) > f.clearance) {
            if (i != 0) pre_extra = mn(mx(pin_step - post_extra, pin_half), pin_step - pin_half);
            line(p, cx - pre_extra, top + mx(tan_mul(cur_px - (pre_extra - pin_half), f.tangent), 0),
                 cx - pin_half, top + tan_mul(cur_px, f.tangent));
        } else {
            int down_start = cx - pin_half - cur_px;
            line(p, down_start, top, cx - pin_half, top + tan_mul(cur_px, f.tangent));
            line(p, mn(cx - pin_step + pin_half + last_px, down_start), top, down_start, top);
        }
        if ((cur + next) > f.clearance) {
            int prod = cur * pin_step / (cur + next);
            post_extra = mn(mx(prod, pin_half), pin_step - pin_half);
            line(p, cx + pin_half, top + cur_px, cx + post_extra,
                 top + mx(cur_px - tan_mul(post_extra - pin_half, f.tangent), 0));
        } else {
            line(p, cx + pin_half, top + tan_mul(cur_px, f.tangent), cx + pin_half + cur_px, top);
        }
    }

    // elbow at the tip and the shoulder end
    const int elbow = px(f.elbow);
    if (!two) line(p, x0 + level_len, base_y, x0 + level_len + elbow, base_y - elbow);
    line(p, x0, top - 6, x0, top);
    if (f.stop == 2) line(p, x0 + level_len, top, x0 + level_len, base_y);

    // pin values above the blade (kept upright); the selected pin is highlighted
    for (int i = 0; i < f.pin_num; i++) {
        const int cx = x0 + px(f.first_pin + i * f.pin_increment);
        const bool sel = (i == pin_);
        const Color fg = sel ? kPin : Color::black();
        const int cw = ui::font::fixed_8x16.char_width();
        if (land) {
            // local (device turned) coordinates: text is rotated by text_at, the frame by line()
            text_at(p, cx - cw / 2, top - 24, fg, to_string_dec_uint(depth_[i]));
            if (sel) {
                ink_ = kPin;
                line(p, cx - 6, top - 26, cx + 5, top - 26);
                line(p, cx - 6, top - 7, cx + 5, top - 7);
                line(p, cx - 6, top - 26, cx - 6, top - 7);
                line(p, cx + 5, top - 26, cx + 5, top - 7);
                ink_ = Color::black();
            }
        } else {
            const Point c = rot_point(cx, top - 16);
            text(p, {c.x() - cw / 2, c.y() - 8}, fg, to_string_dec_uint(depth_[i]));
            if (sel) p.draw_rectangle({c.x() - 6, c.y() - 10, 12, 20}, kPin);
        }
    }

    // header and footer text stay far from the key (landscape: top and bottom of the turned device)
    const int hx = land ? 22 : 8;
    const int y_head = land ? 10 : 24, y_scale = land ? 28 : 42;
    const int y_bit = land ? 160 : 242, y_h1 = land ? 180 : 262, y_h2 = land ? 196 : 278, y_h3 = land ? 212 : 294;
    text_at(p, hx, y_head, Color::black(), std::string(f.manufacturer) + " " + f.name);
    // depth scale as in the manufacturer specification: smallest number = shallowest cut, read head -> tip
    text_at(p, hx, y_scale, Color::dark_grey(),
            "Depth " + to_string_dec_uint(f.min_depth) + "-" + to_string_dec_uint(f.max_depth) + ": " +
                to_string_dec_uint(f.min_depth) + "=shallow " + to_string_dec_uint(f.max_depth) + "=deep");
    text_at(p, hx, y_scale + 18, Color::dark_grey(), "Order: head > tip");
    std::string bitting = "Bitting:";
    for (int i = 0; i < f.pin_num; i++) bitting += " " + to_string_dec_uint(depth_[i]);
    text_at(p, hx, y_bit, Color::black(), bitting);
    text_at(p, hx, y_h1, Color::dark_grey(), "L/R pin    Wheel depth");
    text_at(p, hx, y_h2, Color::dark_grey(), "U/D type   Sel rotate");
    const char* exit_hint = (rot_ == 0) ? "Exit: Left on pin 1, Sel" : (rot_ == 2) ? "Exit: Left on last pin, Sel"
                                                                   : (rot_ == 1)   ? "Exit: Left on pin 1, Sel"
                                                                                   : "Exit: Right on last pin,Sel";
    text_at(p, hx, y_h3, Color::dark_grey(), exit_hint);
}

}  // namespace ui::external_app::key_copier
