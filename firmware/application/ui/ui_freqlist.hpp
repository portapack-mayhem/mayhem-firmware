/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_FREQLIST_H__
#define __UI_FREQLIST_H__

#include "ui.hpp"
#include "ui_font_fixed_5x8.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "event_m0.hpp"
#include "message.hpp"
#include "freqman.hpp"
#include <cstdint>

namespace ui {

class FreqManUIList : public Widget {
   public:
    std::function<void(FreqManUIList&)> on_select{};
    std::function<void(FreqManUIList&)> on_touch_release{};  // Executed when releasing touch, after on_select.
    std::function<void(FreqManUIList&)> on_touch_press{};    // Executed when touching, before on_select.
    std::function<bool(FreqManUIList&, KeyEvent)> on_dir{};
    std::function<void(FreqManUIList&)> on_highlight{};

    FreqManUIList(Rect parent_rect, bool instant_exec);  // instant_exec: Execute on_select when you touching instead of releasing
    FreqManUIList(
        Rect parent_rect)
        : FreqManUIList{parent_rect, false} {
    }
    FreqManUIList()
        : FreqManUIList{{}, {}} {
    }

    void paint(Painter& painter) override;
    void on_focus() override;
    bool on_key(const KeyEvent key) override;
    bool on_touch(const TouchEvent event) override;
    bool on_encoder(EncoderEvent delta) override;

    void set_highlighted_index(int index);  // set highlighted_index and return capped highlighted_index value to set in caller
    uint8_t get_index();                    // return highlighed + index
    void set_db(freqman_db& db);

   private:
    Style style_default{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::white(),
    };
    Style style_yellow{
        .font = font::fixed_8x16,
        .background = Color::black(),
        .foreground = Color::yellow(),
    };
    static constexpr int8_t char_height = 16;
    bool instant_exec_{false};
    freqman_db freqlist_db{};
    int current_index{0};
    int highlighted_index{0};
    int freqlist_nb_lines{0};
};

}  // namespace ui

#endif /*__UI_FREQLIST_H__*/
