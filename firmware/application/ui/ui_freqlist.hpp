/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef __UI_FREQLIST_H__
#define __UI_FREQLIST_H__

#include "ui.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"
#include "event_m0.hpp"
#include "freqman.hpp"
#include "freqman_db.hpp"
#include "message.hpp"
#include <cstdint>

namespace ui {

class FreqManUIList : public Widget {
   public:
    std::function<void(size_t)> on_select{};
    std::function<void()> on_leave{};  // Called when Right is pressed.

    FreqManUIList(Rect parent_rect);
    FreqManUIList(const FreqManUIList& other) = delete;
    FreqManUIList& operator=(const FreqManUIList& other) = delete;

    void paint(Painter& painter) override;
    void on_focus() override;
    void on_blur() override;
    bool on_key(const KeyEvent key) override;
    bool on_encoder(EncoderEvent delta) override;
    bool on_keyboard(const KeyboardEvent event) override;

    void set_parent_rect(Rect new_parent_rect) override;

    void set_index(size_t index);
    size_t get_index() const;
    void set_db(FreqmanDB& db);

   private:
    void adjust_selected_index(int index);

    static constexpr int8_t char_height = 16;
    static constexpr int8_t char_width = 8;
    static constexpr int8_t line_max_length = 29;
    size_t visible_lines_{0};

    FreqmanDB* db_{nullptr};
    size_t start_index_{0};
    size_t selected_index_{0};
};

}  // namespace ui

#endif /*__UI_FREQLIST_H__*/
