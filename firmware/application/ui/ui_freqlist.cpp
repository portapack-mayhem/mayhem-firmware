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

    uint8_t FreqManUIList::set_index( uint8_t index )
    {
        if( freqlist_db.size() == 0 )
            return 0 ;
        if( index >= freqlist_db.size() )
            index = freqlist_db.size() - 1 ;  
        current_index = index ;
        return index ;
    }

    uint8_t FreqManUIList::set_highlighted( uint8_t index )
    {
        if( freqlist_db.size() == 0 )
            return 0 ;
        if( index >= freqlist_nb_lines )
            index = freqlist_nb_lines - 1 ;
        highlighted_index = index ;
        return index ;
    }

    uint8_t FreqManUIList::get_index()
    {
        return current_index + highlighted_index ;
    }

void FreqManUIList::paint(Painter& painter) {
    const auto r = screen_rect();
    const Rect r_widget_screen{r.left(),r.top(),r.width(),r.height()};
        painter.fill_rectangle(
            r_widget_screen,
            Color::black());
    if( freqlist_db.size() == 0 )
        return ;
    uint8_t nb_lines = 0 ;
    for( uint8_t it = current_index ; it < freqlist_db.size() ; it ++ )
    {
        uint8_t line_height = (int)nb_lines * char_height ;
        if( line_height < (r.height() - char_height) )
        {
            // line is within the widget
            painter.draw_string(
                {0, r.location().y() + (int)nb_lines * char_height},
                style_default, freqman_item_string( freqlist_db[it] , 30 ));
            if( nb_lines == highlighted_index )
            {
                const Rect r_highlighted_freq{0 ,r.location().y() + (int)nb_lines * char_height , 240 , char_height};
                painter.draw_rectangle(
                r_highlighted_freq,
                Color::white());
            }
            nb_lines++ ;
        }
        else
        {
            // rect is filled, we can break
            break ;
        }
    }
    freqlist_nb_lines = nb_lines ;
    if (has_focus() || highlighted()) {
        const Rect r_focus{r.left(), r.top(), r.width(), r.height()};
        painter.draw_rectangle(
            r_focus,
            Color::white());
    }
}

void FreqManUIList::set_db( freqman_db &db )
{
   freqlist_db = db ; 
   current_index = 0 ;
   highlighted_index = 0 ;
}

void FreqManUIList::on_hide() {
}

void FreqManUIList::on_show() {
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
    if( delta > 0 )
    {
        if( highlighted_index < 10 )
            highlighted_index ++ ;
        else 
            current_index ++ ;
    }
    if( delta < 0 )
    {    
        if( highlighted_index > 0 )
            highlighted_index -- ;
        else
        {
            if( current_index > 0 )
            current_index -- ;
        }
    }
    current_index = set_index( current_index );
    highlighted_index = set_highlighted( highlighted_index );
    set_dirty();
    return true ;
}
} /* namespace ui */
