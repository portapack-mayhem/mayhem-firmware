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
#include <algorithm>

namespace ui::external_app::epirb_rx {

BeaconUIList::BeaconUIList(Rect parent_rect)
    : View{parent_rect} {
    this->set_focusable(true);
}

void BeaconUIList::paint(Painter& painter) {
    auto rect = screen_rect();
    // Clear view
    painter.fill_rectangle(rect, Theme::getInstance()->bg_darkest->background);

    if (!db_ || (db_->empty())) {
        return;
    }

    auto base_style = Theme::getInstance()->bg_darkest;

    for (auto offset = 0u; offset < BEACON_HISTORY_SIZE; ++offset) {
        // The whole frame needs to be cleared so every line 'slot'
        // is redrawn even when `text` just left empty.
        auto text = std::string{};
        auto index = start_index_ + offset;
        auto line_position = rect.location() + Point{0, 1 + (int)offset * char_height};
        auto is_selected = offset == selected_index_;
        auto style = base_style;

        if (index < db_->size()) {
            // Get beacon entry and format it's summary
            auto& entry = db_->get_beacon(index);
            char buffer[64];
            entry.formatSummary(buffer, true);
            text = std::string(buffer);
        }

        if (index == db_->get_current_beacon_index())
            // If this is the currently displayed beacon change color
            style = Theme::getInstance()->bg_medium;

        // Draw entry line
        painter.draw_string(
            line_position, (is_selected ? style->invert() : *style), text);
    }

    // Draw a bounding rectangle when focused.
    painter.draw_rectangle(rect, (has_focus() ? Theme::getInstance()->bg_darkest->foreground : Theme::getInstance()->bg_darkest->background));
}

void BeaconUIList::on_show() {
    set_dirty();
}

bool BeaconUIList::on_key(const KeyEvent key) {
    if (!db_ || db_->empty())
        return false;

    if (key == KeyEvent::Select && on_select) {
        // Select this beacon
        on_select(get_index());
        set_dirty();
        return true;
    }

    auto delta = 0;
    // Change position in the list
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
    // Change position in the list according to encoder
    adjust_selected_index(delta);
    set_dirty();
    return true;
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
    else if (new_index >= (int32_t)BEACON_HISTORY_SIZE) {
        start_index_ = std::min<int32_t>(start_index_ + delta, db_->size() - BEACON_HISTORY_SIZE);
        selected_index_ = BEACON_HISTORY_SIZE - 1;
    }

    // Otherwise, scroll within the screen, but not past the end.
    else {
        selected_index_ = std::min<int32_t>(new_index, db_->size() - 1);
    }
}

}  // namespace ui::external_app::epirb_rx
