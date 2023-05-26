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

void FreqmanUIList::paint(Painter& painter) {
    const auto r = screen_rect();
    if( freqlist_db.size() == 0 )
        return ;
    for( uint8_t n = 0 ; n < 10 ; n ++ )
    {
        if( current_index + n < freqlist_db.size() && freqlist_db[n].description.length() > 0 )
        {
            painter.draw_string(
                {0, r.location().y() + (int)n * char_height},
                style_default, freqlist_db[n].description);
        }
    }
}

void FreqmanUIList::set_db( freqman_db &db )
{
   freqlist_db = db ; 
}

void FreqmanUIList::on_hide() {
}

void FreqmanUIList::on_show() {
}

void FreqmanUIList::on_focus() {
    if (on_highlight)
        on_highlight(*this);
}

bool FreqmanUIList::on_key(const KeyEvent key) {
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

bool FreqmanUIList::on_touch(const TouchEvent event) {
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
} /* namespace ui */
