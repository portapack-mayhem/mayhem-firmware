/*
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#include "ui_beaconlist.hpp"

#include "baseband_api.hpp"
#include "utility.hpp"

namespace ui::external_app::epirb_rx {

BeaconUIList::BeaconUIList(Rect parent_rect)
    : Widget{parent_rect},
      visible_lines_{(unsigned)parent_rect.height() / char_height} {
    line_max_length = (parent_rect.width() - 8) / char_width;
    this->set_focusable(true);
}

void BeaconUIList::paint(Painter& painter) {
    auto rect = screen_rect();

    if (!db_ || (db_->empty())) {
        auto line_position = rect.location() + Point{7 * 8, 6 * 16};
        painter.fill_rectangle(rect, Theme::getInstance()->bg_darkest->background);
        painter.draw_string(line_position, *Theme::getInstance()->bg_darkest, "No beacons");
        return;
    }

    auto base_style = Theme::getInstance()->bg_darkest;

    // TODO: could minimize redraw/re-read if necessary
    //       with better change tracking.
    for (auto offset = 0u; offset < visible_lines_; ++offset) {
        // The whole frame needs to be cleared so every line 'slot'
        // is redrawn even when `text` just left empty.
        auto text = std::string{};
        auto index = start_index_ + offset;
        auto line_position = rect.location() + Point{1, 1 + (int)offset * char_height};
        auto is_selected = offset == selected_index_;
        auto style = base_style;

        if (index < db_->size()) {
            auto& entry = db_->get_beacon(index);
            text = entry.formatSummary(true);
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

void BeaconUIList::on_focus() {
    set_dirty();
}

void BeaconUIList::on_blur() {
    set_dirty();
}

bool BeaconUIList::on_touch(const TouchEvent event) {
    if (!db_ || db_->empty())
        return false;
    if (event.type == TouchEvent::Type::Start) {
        focus();
        set_dirty();
        int16_t rel_y = event.point.y() - screen_rect().top();
        size_t new_selected = rel_y / char_height;
        if (new_selected + start_index_ >= db_->size()) {
            return true;  // clicked, where there is no entry, skip it
        }
        // selected_index_ is the current
        if (selected_index_ == new_selected) {
            // already selected, trigger on_select
            if (on_select) {
                // on_select(new_selected); //causes strange behavior, not so confident to use it
            }
            return true;
        } else {
            // just change selection
            selected_index_ = new_selected;
        }
        return true;
    }
    return true;
}

bool BeaconUIList::on_keyboard(const KeyboardEvent key) {
    if (!db_ || db_->empty())
        return false;

    auto delta = 0;
    if (key == '-' && get_index() > 0) delta = -1;
    if (key == '+' && get_index() < db_->size() - 1) delta = 1;
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

bool BeaconUIList::on_key(const KeyEvent key) {
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
    else if (key == KeyEvent::Down && get_index() < db_->size() - 1)
        delta = 1;
    else
        return false;

    adjust_selected_index(delta);
    set_dirty();
    return true;
}

bool BeaconUIList::on_encoder(EncoderEvent delta) {
    if (!db_ || db_->empty())
        return false;

    adjust_selected_index(delta);
    set_dirty();
    return true;
}

void BeaconUIList::set_parent_rect(Rect new_parent_rect) {
    visible_lines_ = new_parent_rect.height() / char_height;
    Widget::set_parent_rect(new_parent_rect);
}

void BeaconUIList::set_index(size_t index) {
    start_index_ = 0;
    selected_index_ = 0;
    adjust_selected_index(index);
}

size_t BeaconUIList::get_index() const {
    return start_index_ + selected_index_;
}

void BeaconUIList::set_db(BeaconDB& db) {
    db_ = &db;
    start_index_ = 0;
    selected_index_ = 0;
    set_dirty();
}

void BeaconUIList::adjust_selected_index(int delta) {
    int32_t new_index = selected_index_ + delta;

    // The selection went off the top of the screen, move up.
    if (new_index < 0) {
        start_index_ = std::max<int32_t>(start_index_ + new_index, 0);
        selected_index_ = 0;
    }

    // Selection is off the bottom of the screen, move down.
    else if (new_index >= (int32_t)visible_lines_) {
        start_index_ = std::min<int32_t>(start_index_ + delta, db_->size() - visible_lines_);
        selected_index_ = visible_lines_ - 1;
    }

    // Otherwise, scroll within the screen, but not past the end.
    else {
        selected_index_ = std::min<int32_t>(new_index, db_->size() - 1);
    }
}

}  // namespace ui::external_app::epirb_rx
