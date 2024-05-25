/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_tabview.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "portapack.hpp"
using namespace portapack;

namespace ui {

Tab::Tab() {
    set_focusable(true);
};

void Tab::set(
    uint32_t index,
    Dim width,
    std::string text,
    Color text_color) {
    set_parent_rect({(Coord)(index * width), 0, width, 24});

    text_ = text.substr(0, (width - 8) / 8);
    text_color_ = text_color;

    index_ = index;
}

void Tab::paint(Painter& painter) {
    const auto rect = screen_rect();
    const Color color = highlighted() ? Theme::getInstance()->bg_darkest->background : Theme::getInstance()->bg_medium->background;

    painter.fill_rectangle({rect.left(), rect.top(), rect.width() - 8, rect.height()}, color);

    if (!highlighted())
        painter.draw_hline({rect.left(), rect.top()}, rect.width() - 9, Theme::getInstance()->fg_light->foreground);

    painter.draw_bitmap(
        {rect.right() - 8, rect.top()},
        bitmap_tab_edge,
        color,
        Theme::getInstance()->bg_dark->background);

    auto text_point = rect.center() - Point(4, 0) - Point(text_.size() * 8 / 2, 16 / 2);

    painter.draw_string(
        text_point,
        {ui::font::fixed_8x16, color, text_color_},
        text_);

    if (has_focus())
        painter.draw_hline(text_point + Point(0, 16), text_.size() * 8, Theme::getInstance()->bg_darkest->foreground);
}

bool Tab::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select) {
        static_cast<TabView*>(parent())->set_selected(index_);
        return true;
    }

    return false;
}

bool Tab::on_touch(const TouchEvent event) {
    switch (event.type) {
        case TouchEvent::Type::Start:
            focus();
            set_dirty();
            return true;

        case TouchEvent::Type::End:
            static_cast<TabView*>(parent())->set_selected(index_);
            return true;

        default:
            return false;
    }
}

void TabView::set_selected(uint32_t index) {
    Tab* tab;

    if (index >= n_tabs)
        return;

    // Hide previous view
    views[current_tab]->hidden(true);

    tab = &tabs[current_tab];
    tab->set_highlighted(false);
    tab->set_focusable(true);
    tab->set_dirty();

    // Show new view
    views[index]->hidden(false);

    tab = &tabs[index];
    current_tab = index;
    tab->set_highlighted(true);
    tab->set_focusable(false);
    tab->set_dirty();
}

void TabView::on_show() {
    set_selected(current_tab);
}

void TabView::focus() {
    views[current_tab]->focus();
}

TabView::TabView(std::initializer_list<TabDef> tab_definitions) {
    size_t i = 0;

    n_tabs = tab_definitions.size();
    if (n_tabs > MAX_TABS)
        n_tabs = MAX_TABS;

    size_t tab_width = 240 / n_tabs;

    set_parent_rect({0, 0, 30 * 8, 3 * 8});

    for (auto& tab_definition : tab_definitions) {
        tabs[i].set(i, tab_width, tab_definition.text, tab_definition.color);
        views[i] = tab_definition.view;
        add_child(&tabs[i]);
        i++;
        if (i == MAX_TABS) break;
    }
}

TabView::~TabView() {
}

} /* namespace ui */
