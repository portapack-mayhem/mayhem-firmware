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

#include "ui_freqlist.hpp"

#include "baseband_api.hpp"
#include "utility.hpp"

#include <algorithm>

namespace ui {
FreqManUIList::FreqManUIList(
    Rect parent_rect,
    bool instant_exec)
    : Widget{parent_rect},
      instant_exec_{instant_exec} {
    this->set_focusable(true);
}

void FreqManUIList::set_highlighted_index(int index) {
    if ((unsigned)(current_index + index) >= freqlist_db.size())
        return;
    if (index < 0) {
        index = 0;
        if (current_index > 0)
            current_index--;
    }
    if (index >= freqlist_nb_lines) {
        index = freqlist_nb_lines - 1;
        if ((unsigned)(current_index + index) < freqlist_db.size())
            current_index++;
        else
            current_index = freqlist_db.size() - freqlist_nb_lines - 1;
    }
    highlighted_index = index;
}

uint8_t FreqManUIList::get_index() {
    return current_index + highlighted_index ;
}

void FreqManUIList::paint(Painter& painter) {
    freqlist_nb_lines = 0;
    const auto r = screen_rect();
    uint8_t focused = has_focus();
    const Rect r_widget_screen{r.left() + focused, r.top() + focused, r.width() - 2 * focused, r.height() - 2 * +focused};
    painter.fill_rectangle(
        r_widget_screen,
        Color::black());
    // only return after clearing the screen so previous entries are not shown anymore
    if (freqlist_db.size() == 0)
        return;
    // coloration if file is too big
    Style* text_color = &style_default;
    if (freqlist_db.size() > FREQMAN_MAX_PER_FILE)
        text_color = &style_yellow;
    uint8_t nb_lines = 0;
    for (uint8_t it = current_index; it < freqlist_db.size(); it++) {
        uint8_t line_height = (int)nb_lines * char_height;
        if (line_height < (r.height() - char_height)) {
            std::string description = freqman_item_string(freqlist_db[it], 30);
            // line is within the widget
            painter.draw_string(
                {0, r.location().y() + (int)nb_lines * char_height},
                *text_color, description);
            if (nb_lines == highlighted_index) {
                const Rect r_highlighted_freq{0, r.location().y() + (int)nb_lines * char_height, 240, char_height};
                painter.draw_rectangle(
                    r_highlighted_freq,
                    Color::white());
            }
            nb_lines++;
        } else {
            // rect is filled, we can break
            break;
        }
    }
    freqlist_nb_lines = nb_lines;
    if (has_focus() || highlighted()) {
        const Rect r_focus{r.left(), r.top(), r.width(), r.height()};
        painter.draw_rectangle(
            r_focus,
            Color::white());
    }
}

void FreqManUIList::set_db(freqman_db& db) {
    freqlist_db = db;
    if( db.size() == 0 )
    {
        current_index = 0;
        highlighted_index = 0;
    }
    else
    {
        if( (unsigned)(current_index + highlighted_index) >= db.size() )
        {
            current_index =  db.size() - 1 - highlighted_index ;
        }
        if( current_index < 0 )
        {
            current_index = 0 ;
            if( highlighted_index > 0 )
                highlighted_index -- ;
        }
    }
}

void FreqManUIList::on_focus() {
    if (on_highlight)
        on_highlight(*this);
}

bool FreqManUIList::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        if (on_select) {
            on_select(*this);
            return true;
        }
    } else {
        if (on_dir) {
            return on_dir(*this, key);
        }
    }
    return false;
}

bool FreqManUIList::on_touch(const TouchEvent event) {
    switch (event.type) {
        case TouchEvent::Type::Start:
            set_highlighted(true);
            set_dirty();
            if (on_touch_press) {
                on_touch_press(*this);
            }
            if (on_select && instant_exec_) {
                on_select(*this);
            }
            return true;
        case TouchEvent::Type::End:
            set_highlighted(false);
            set_dirty();
            if (on_touch_release) {
                on_touch_release(*this);
            }
            if (on_select && !instant_exec_) {
                on_select(*this);
            }
            return true;
        default:
            return false;
    }
}

bool FreqManUIList::on_encoder(EncoderEvent delta) {
    set_highlighted_index((int)highlighted_index + delta);
    set_dirty();
    return true;
}
} /* namespace ui */
