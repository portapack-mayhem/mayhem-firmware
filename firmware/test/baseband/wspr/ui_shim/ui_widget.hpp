/*
 * Copyright (C) 2026 Ivan
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

/* HOST TEST SHIM for the Mayhem ui::Widget / ui::Theme framework (never
 * compiled into firmware). It provides the minimal surface that
 * ui_station_list.hpp needs: a focusable Widget base with the real
 * on_key/on_encoder/on_touch/on_focus/on_blur/paint virtuals, screen_rect()
 * (no parent chain on the host: parent_rect == screen_rect), set_dirty(),
 * style() and a Theme singleton exposing the style pointers the widget
 * reads (fg_green). */

#ifndef __WSPR_UI_SHIM_UI_WIDGET_H__
#define __WSPR_UI_SHIM_UI_WIDGET_H__

#include "ui.hpp"
#include "ui_painter.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <functional>

namespace ui {

class Theme {
   public:
    static Theme* getInstance() {
        static Theme instance;
        return &instance;
    }

    Style* fg_green{nullptr};
    Style* fg_red{nullptr};
    Style* fg_orange{nullptr};
    Style* fg_yellow{nullptr};
    Style* fg_cyan{nullptr};
    Style* fg_white{nullptr};

   private:
    Theme() {
        static Style green{font::fixed_8x16, Color::black(), Color::green()};
        static Style red{font::fixed_8x16, Color::black(), Color::red()};
        static Style orange{font::fixed_8x16, Color::black(), Color::orange()};
        static Style yellow{font::fixed_8x16, Color::black(), Color::yellow()};
        static Style cyan{font::fixed_8x16, Color::black(), Color::cyan()};
        static Style white{font::fixed_8x16, Color::black(), Color::white()};
        fg_green = &green;
        fg_red = &red;
        fg_orange = &orange;
        fg_yellow = &yellow;
        fg_cyan = &cyan;
        fg_white = &white;
    }
};

class Widget {
   public:
    Widget(Rect parent_rect = {})
        : _parent_rect{parent_rect} {
    }

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;
    virtual ~Widget() = default;

    /* Host harness has no parent chain: the widget rect IS the screen rect
     * (the harness places the widget at its device coordinates instead). */
    Rect screen_rect() const { return _parent_rect; }
    void set_parent_rect(const Rect new_parent_rect) {
        _parent_rect = new_parent_rect;
    }

    void focus() {
        focused_widget_ = this;
        on_focus();
    }
    void blur() {
        if (focused_widget_ == this)
            focused_widget_ = nullptr;
        on_blur();
    }
    bool has_focus() const { return focused_widget_ == this; }
    bool focusable() const { return focusable_; }
    void set_focusable(const bool value) { focusable_ = value; }

    virtual void on_focus() {}
    virtual void on_blur() {}

    void set_dirty() { dirty_ = true; }
    bool dirty() const { return dirty_; }
    void clear_dirty() { dirty_ = false; }

    const Style& style() const { return style_; }
    /* Host default style: black background, default font - the only field
     * ui_station_list.hpp reads is .background. */
    Style style_{font::fixed_8x16, Color::black(), Color::white()};

    virtual void paint(Painter& painter) = 0;
    virtual bool on_key(const KeyEvent) { return false; }
    virtual bool on_encoder(const EncoderEvent) { return false; }
    virtual bool on_touch(const TouchEvent) { return false; }

   private:
    Rect _parent_rect;
    bool focusable_{false};
    bool dirty_{false};
    inline static Widget* focused_widget_{nullptr};
};

} /* namespace ui */

#endif /*__WSPR_UI_SHIM_UI_WIDGET_H__*/
