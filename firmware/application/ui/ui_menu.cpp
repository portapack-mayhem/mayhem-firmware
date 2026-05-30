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
#include <string_view>

namespace ui {

/* MenuItemView **********************************************************/

void MenuItemView::set_item(MenuItem* item_) {
    item = item_;
    scroll_offset = 0;
    can_scroll = false;
}

void MenuItemView::highlight() {
    set_highlighted(true);
    scroll_offset = 0;
    set_dirty();
}

void MenuItemView::unhighlight() {
    set_highlighted(false);
    scroll_offset = 0;
    set_dirty();
}

void MenuItemView::set_scroll_offset(size_t offset) {
    if (can_scroll && scroll_offset != offset) {
        scroll_offset = offset;
        set_dirty();
    }
}

void MenuItemView::paint(Painter& painter) {
    if (!item) return;

    const auto rect = screen_rect();
    const auto paint_style = (highlighted() && (parent()->has_focus() || keep_highlight)) ? style().invert() : style();

    const int char_width = paint_style.font.char_width();
    const int line_height = paint_style.font.line_height();

    if (char_width == 0 || line_height == 0) return;

    const int margin_x = char_width / 2;

    ui::Color final_item_color = (highlighted() && (parent()->has_focus() || keep_highlight)) ? paint_style.foreground : item->color;
    ui::Color final_bg_color = (highlighted() && (parent()->has_focus() || keep_highlight)) ? item->color : paint_style.background;

    if (final_item_color.v == final_bg_color.v) final_item_color = paint_style.foreground;

    painter.fill_rectangle(rect, final_bg_color);

    Coord offset_x = 0;

    if (item->bitmap) {
        painter.draw_bitmap(
            {rect.location().x() + 4, rect.location().y() + 4},
            *item->bitmap,
            final_item_color,
            final_bg_color);
        offset_x = 26;
    } else {
        offset_x = 0;
    }

    Style text_style{.font = paint_style.font, .background = final_bg_color, .foreground = final_item_color};

    std::string_view full_text = item->text;
    std::string_view file_name = full_text;
    std::string_view file_size_text = "";

    auto tab_pos = full_text.find('\t');
    if (tab_pos != std::string_view::npos) {
        file_size_text = full_text.substr(tab_pos + 1);
        file_name = full_text.substr(0, tab_pos);
    }

    int available_width_px = rect.width() - offset_x;
    if (available_width_px <= 0) return;

    size_t max_name_chars = available_width_px / char_width;
    if (max_name_chars == 0) return;

    if (!file_size_text.empty() && max_name_chars > file_size_text.length()) {
        max_name_chars = max_name_chars - file_size_text.length() - 1;
    }

    if (max_name_chars == 0) return;

    can_scroll = (file_name.length() > max_name_chars);

    std::string_view display_name = file_name;
    if (file_name.length() > max_name_chars) {
        if (highlighted()) {
            size_t max_scroll = file_name.length() - max_name_chars;
            size_t actual_offset = scroll_offset % (max_scroll + 1);
            display_name = file_name.substr(actual_offset, max_name_chars);
        } else {
            display_name = file_name.substr(0, max_name_chars);
        }
    }

    Coord text_y = rect.location().y() + (rect.height() - line_height) / 2;

    painter.draw_string({rect.location().x() + offset_x, text_y}, text_style, display_name);

    if (!file_size_text.empty()) {
        int file_size_width = static_cast<int>(file_size_text.length()) * char_width;
        int file_size_x = rect.width() - file_size_width - margin_x;

        // Csak akkor rajzoljuk ki, ha pozitív koordinátára esik és nem takarja el az ikont/nevet
        if (file_size_x > offset_x) {
            painter.draw_string({rect.location().x() + static_cast<Coord>(file_size_x), text_y}, text_style, file_size_text);
        }
    }
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
        menu_item_views.shrink_to_fit();
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

void MenuView::increment_scroll() {
    if (menu_items.empty()) return;
    const size_t view_index = highlighted_item - offset;
    if (view_index >= menu_item_views.size()) {
        return;
    }
    // The MenuView checks if the currently highlighted item needs scrolling
    scroll_offset++;
    auto* view = item_view(view_index);
    if (view) {
        view->set_scroll_offset(scroll_offset);
    }
}

void MenuView::on_tick_second() {
    if (more && blink)
        arrow_more.set_foreground(Theme::getInstance()->bg_darkest->foreground);
    else
        arrow_more.set_foreground(Theme::getInstance()->bg_darkest->background);

    blink = !blink;
    arrow_more.set_dirty();

    increment_scroll();
}

void MenuView::clear() {
    for (auto& item : menu_item_views)
        item->set_item(nullptr);

    menu_items.clear();
    menu_items.shrink_to_fit();
    highlighted_item = 0;
    offset = 0;
    reset_scroll();
}

size_t MenuView::item_count() const {
    return menu_items.size();
}

void MenuView::add_item(MenuItem&& new_item) {
    menu_items.push_back(std::move(new_item));

    update_items();
}

void MenuView::add_items(std::initializer_list<MenuItem> new_items) {
    menu_items.insert(menu_items.end(), new_items);
    update_items();
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

    reset_scroll();

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

bool MenuView::on_touch(const TouchEvent event) {
    size_t i = 0;
    for (const auto child : children_) {
        if (i >= menu_item_views.size()) break;
        if (child->screen_rect().contains(event.point)) {
            return set_highlighted(i + offset);
        }
        i++;
    }
    return false;
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
