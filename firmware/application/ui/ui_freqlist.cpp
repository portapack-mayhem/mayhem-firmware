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

#include "ui_freqlist.hpp"

#include "baseband_api.hpp"
#include "utility.hpp"

namespace ui {

FreqManUIList::FreqManUIList(Rect parent_rect)
    : Widget{parent_rect},
      visible_lines_{(unsigned)parent_rect.height() / char_height} {
    this->set_focusable(true);
}

void FreqManUIList::paint(Painter& painter) {
    auto rect = screen_rect();

    if (!db_ || db_->empty()) {
        auto line_position = rect.location() + Point{7 * 8, 6 * 16};
        painter.fill_rectangle(rect, Theme::getInstance()->bg_darkest->background);
        painter.draw_string(line_position, *Theme::getInstance()->bg_darkest, "Empty Category");
        return;
    }

    // Indicate when a file is too large by drawing in yellow.
    auto over_max = db_->entry_count() > freqman_default_max_entries;
    auto base_style = over_max ? Theme::getInstance()->fg_yellow : Theme::getInstance()->bg_darkest;

    // TODO: could minimize redraw/re-read if necessary
    //       with better change tracking.
    for (auto offset = 0u; offset < visible_lines_; ++offset) {
        // The whole frame needs to be cleared so every line 'slot'
        // is redrawn even when `text` just left empty.
        auto text = std::string{};
        auto index = start_index_ + offset;
        auto line_position = rect.location() + Point{4, 1 + (int)offset * char_height};
        auto is_selected = offset == selected_index_;
        auto style = base_style;

        if (index < db_->entry_count()) {
            auto entry = (*db_)[index];
            // db_ is directly backed by a file, so invalid lines cannot be
            // pre-filtered. Show an empty entry if 'Unknown'.
            if (entry.type != freqman_type::Unknown)
                text = pretty_string(entry, line_max_length);

            // Otherwise, if 'Raw' indicate an invalid entry by color.
            if (entry.type == freqman_type::Raw)
                style = Theme::getInstance()->fg_light;
        }

        // Pad right with ' ' so trailing chars are cleaned up.
        // draw_glyph has less flicker than fill_rect when drawing.
        if (text.length() < line_max_length)
            text.resize(line_max_length, ' ');

        painter.draw_string(
            line_position, (is_selected ? style->invert() : *style), text);
    }

    // Draw a bounding rectangle when focused.
    painter.draw_rectangle(rect, (has_focus() ? Theme::getInstance()->bg_darkest->foreground : Theme::getInstance()->bg_darkest->background));
}

void FreqManUIList::on_focus() {
    set_dirty();
}

void FreqManUIList::on_blur() {
    set_dirty();
}

bool FreqManUIList::on_keyboard(const KeyboardEvent key) {
    if (!db_ || db_->empty())
        return false;

    auto delta = 0;
    if (key == '-' && get_index() > 0) delta = -1;
    if (key == '+' && get_index() < db_->entry_count() - 1) delta = 1;
    if (delta != 0) {
        adjust_selected_index(delta);
        set_dirty();
        return true;
    }
    if (key == 10) {
        if (on_select) {
            on_select(get_index());
            return true;
        }
    }

    return false;
}

bool FreqManUIList::on_key(const KeyEvent key) {
    if (!db_ || db_->empty())
        return false;

    if (key == KeyEvent::Select && on_select) {
        on_select(get_index());
        return true;
    } else if (key == KeyEvent::Right && on_leave) {
        on_leave();
        return true;
    }

    auto delta = 0;
    if (key == KeyEvent::Up && get_index() > 0)
        delta = -1;
    else if (key == KeyEvent::Down && get_index() < db_->entry_count() - 1)
        delta = 1;
    else
        return false;

    adjust_selected_index(delta);
    set_dirty();
    return true;
}

bool FreqManUIList::on_encoder(EncoderEvent delta) {
    if (!db_ || db_->empty())
        return false;

    adjust_selected_index(delta);
    set_dirty();
    return true;
}

void FreqManUIList::set_parent_rect(Rect new_parent_rect) {
    visible_lines_ = new_parent_rect.height() / char_height;
    Widget::set_parent_rect(new_parent_rect);
}

void FreqManUIList::set_index(size_t index) {
    start_index_ = 0;
    selected_index_ = 0;
    adjust_selected_index(index);
}

size_t FreqManUIList::get_index() const {
    return start_index_ + selected_index_;
}

void FreqManUIList::set_db(FreqmanDB& db) {
    db_ = &db;
    start_index_ = 0;
    selected_index_ = 0;
    set_dirty();
}

void FreqManUIList::adjust_selected_index(int delta) {
    int32_t new_index = selected_index_ + delta;

    // The selection went off the top of the screen, move up.
    if (new_index < 0) {
        start_index_ = std::max<int32_t>(start_index_ + new_index, 0);
        selected_index_ = 0;
    }

    // Selection is off the bottom of the screen, move down.
    else if (new_index >= (int32_t)visible_lines_) {
        start_index_ = std::min<int32_t>(start_index_ + delta, db_->entry_count() - visible_lines_);
        selected_index_ = visible_lines_ - 1;
    }

    // Otherwise, scroll within the screen, but not past the end.
    else {
        selected_index_ = std::min<int32_t>(new_index, db_->entry_count() - 1);
    }
}

} /* namespace ui */
