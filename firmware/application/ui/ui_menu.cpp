/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_menu.hpp"
#include "rtc_time.hpp"

namespace ui {

/* MenuItemView **********************************************************/

void MenuItemView::set_item(MenuItem* item_) {
    item = item_;
}

void MenuItemView::highlight() {
    set_highlighted(true);
    set_dirty();
}

void MenuItemView::unhighlight() {
    set_highlighted(false);
    set_dirty();
}

void MenuItemView::paint(Painter& painter) {
    Coord offset_x{};

    if (!item) return;

    const auto r = screen_rect();

    const auto paint_style = (highlighted() && (parent()->has_focus() || keep_highlight)) ? style().invert() : style();

    const auto font_height = paint_style.font.line_height();

    ui::Color final_item_color = (highlighted() && (parent()->has_focus() || keep_highlight)) ? paint_style.foreground : item->color;
    ui::Color final_bg_color = (highlighted() && (parent()->has_focus() || keep_highlight)) ? item->color : paint_style.background;

    if (final_item_color.v == final_bg_color.v) final_item_color = paint_style.foreground;

    painter.fill_rectangle(
        r,
        final_bg_color);

    if (item->bitmap) {
        painter.draw_bitmap(
            {r.location().x() + 4, r.location().y() + 4},
            *item->bitmap,
            final_item_color,
            final_bg_color);
        offset_x = 26;
    } else
        offset_x = 0;

    Style text_style{
        .font = paint_style.font,
        .background = final_bg_color,
        .foreground = final_item_color};

    painter.draw_string(
        {r.location().x() + offset_x, r.location().y() + (r.size().height() - font_height) / 2},
        text_style,
        item->text);
}

/* MenuView **************************************************************/

MenuView::MenuView(
    Rect new_parent_rect,
    bool keep_highlight)
    : keep_highlight{keep_highlight} {
    set_parent_rect(new_parent_rect);

    set_focusable(true);

    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };

    add_child(&arrow_more);
    arrow_more.set_focusable(false);
    arrow_more.set_foreground(Theme::getInstance()->bg_darkest->background);
}

MenuView::~MenuView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
}

void MenuView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    displayed_max = (parent_rect().size().height() / item_height);
    arrow_more.set_parent_rect({228, (Coord)(displayed_max * item_height), 8, 8});

    // TODO: Clean this up :(
    if (!menu_item_views.empty()) {
        for (auto& item : menu_item_views)
            remove_child(item.get());

        menu_item_views.clear();
    }

    for (size_t c = 0; c < displayed_max; c++) {
        auto item = std::make_unique<MenuItemView>(keep_highlight);
        add_child(item.get());

        Coord y_pos = c * item_height;
        item->set_parent_rect({{0, y_pos},
                               {size().width(), (Coord)item_height}});

        menu_item_views.push_back(std::move(item));
    }

    update_items();
}

void MenuView::on_tick_second() {
    if (more && blink)
        arrow_more.set_foreground(Theme::getInstance()->bg_darkest->foreground);
    else
        arrow_more.set_foreground(Theme::getInstance()->bg_darkest->background);

    blink = !blink;

    arrow_more.set_dirty();
}

void MenuView::clear() {
    for (auto& item : menu_item_views)
        item->set_item(nullptr);

    menu_items.clear();
    highlighted_item = 0;
    offset = 0;
}

size_t MenuView::item_count() const {
    return menu_items.size();
}

void MenuView::add_item(MenuItem new_item) {
    menu_items.push_back(new_item);

    update_items();
}

void MenuView::add_items(std::initializer_list<MenuItem> new_items) {
    for (auto item : new_items) {
        add_item(item);
    }
}

void MenuView::update_items() {
    size_t i = 0;

    if (menu_items.size() > displayed_max + offset) {
        more = true;
        blink = true;
    } else
        more = false;

    for (auto& item : menu_item_views) {
        if (i >= menu_items.size()) break;

        // Assign item data to MenuItemViews according to offset
        item->set_item(&menu_items[i + offset]);
        item->set_dirty();

        if (highlighted_item == (i + offset)) {
            item->highlight();
        } else
            item->unhighlight();

        i++;
    }
}

MenuItemView* MenuView::item_view(size_t index) const {
    return menu_item_views[index].get();
}

bool MenuView::set_highlighted(int32_t new_value) {
    int32_t item_count = (int32_t)menu_items.size();

    if (new_value < 0 || item_count == 0)
        return false;

    if (new_value >= item_count)
        new_value = item_count - 1;

    if (((uint32_t)new_value > offset) && ((new_value - offset) >= displayed_max)) {
        // Shift MenuView up
        highlighted_item = new_value;
        offset = new_value - displayed_max + 1;
        update_items();
    } else if ((uint32_t)new_value < offset) {
        // Shift MenuView down
        highlighted_item = new_value;
        offset = new_value;
        update_items();
    } else {
        // Just update highlight
        item_view(highlighted_item - offset)->unhighlight();
        highlighted_item = new_value;
        item_view(highlighted_item - offset)->highlight();
    }

    if (on_highlight)
        on_highlight();

    return true;
}

uint32_t MenuView::highlighted_index() const {
    return highlighted_item;
}

void MenuView::on_focus() {
    item_view(highlighted_item - offset)->highlight();
}

void MenuView::on_blur() {
    if (!keep_highlight)
        item_view(highlighted_item - offset)->unhighlight();
}

bool MenuView::on_key(const KeyEvent key) {
    switch (key) {
        case KeyEvent::Up:
            return set_highlighted(highlighted_item - 1);

        case KeyEvent::Down:
            return set_highlighted(highlighted_item + 1);

        case KeyEvent::Right:
            if (on_right) {
                on_right();
            }
            [[fallthrough]];
        case KeyEvent::Select:
            if (menu_items[highlighted_item].on_select) {
                menu_items[highlighted_item].on_select(key);
            }
            return true;

        case KeyEvent::Left:
            if (on_left) {
                on_left();
            }
            return true;

        default:
            return false;
    }
}

bool MenuView::on_keyboard(const KeyboardEvent key) {
    if (key == '-') return set_highlighted(highlighted_item - 1);
    if (key == '+') return set_highlighted(highlighted_item + 1);
    if (key == 10) {
        if (menu_items[highlighted_item].on_select) {
            menu_items[highlighted_item].on_select(KeyEvent::Right);
        }
        return true;
    }

    return false;
}

bool MenuView::on_encoder(const EncoderEvent event) {
    set_highlighted(highlighted_item + event);
    return true;
}

/* TODO: This could be handled by default behavior, if the UI stack were to
 * transmit consumable events from the top of the hit-stack down, and each
 * MenuItem could respond to a touch and update its parent MenuView.
 */
/*
bool MenuView::on_touch(const TouchEvent event) {
        size_t i = 0;
        for(const auto child : children_) {
                if( child->screen_rect().contains(event.point) ) {
                        return set_highlighted(i);
                }
                i++;
        }

        return false;
}
*/
} /* namespace ui */
