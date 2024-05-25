/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2019 Elia Yehuda (z4ziggy)
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2024 u-foka
 * Copyleft (ↄ) 2024 zxkmm with the GPL license
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

#include "ui_btngrid.hpp"
#include "rtc_time.hpp"

namespace ui {

/* BtnGridView **************************************************************/

BtnGridView::BtnGridView(
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

BtnGridView::~BtnGridView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
}

void BtnGridView::set_max_rows(int rows) {
    rows_ = rows;
}

int BtnGridView::rows() {
    return rows_;
}

void BtnGridView::set_parent_rect(const Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    displayed_max = (parent_rect().size().height() / button_h);
    arrow_more.set_parent_rect({228, (Coord)(displayed_max * button_h), 8, 8});
    displayed_max *= rows_;

    // Delete any existing buttons.
    if (!menu_item_views.empty()) {
        for (auto& item : menu_item_views)
            remove_child(item.get());

        menu_item_views.clear();
    }

    button_w = screen_width / rows_;
    for (size_t c = 0; c < displayed_max; c++) {
        auto item = std::make_unique<NewButton>();
        add_child(item.get());
        item->set_parent_rect({(int)(c % rows_) * button_w,
                               (int)(c / rows_) * button_h,
                               button_w, button_h});

        menu_item_views.push_back(std::move(item));
    }

    update_items();
}

void BtnGridView::set_arrow_enabled(bool enabled) {
    if (enabled) {
        add_child(&arrow_more);
    } else {
        remove_child(&arrow_more);
    }
};

void BtnGridView::on_tick_second() {
    if (more && blink)
        arrow_more.set_foreground(Theme::getInstance()->bg_darkest->foreground);
    else
        arrow_more.set_foreground(Theme::getInstance()->bg_darkest->background);

    blink = !blink;

    arrow_more.set_dirty();
}

void BtnGridView::clear() {
    // clear vector and release memory, not using swap since it's causing capture to glitch/fault
    menu_items.clear();

    for (auto& item : menu_item_views)
        remove_child(item.get());

    // clear vector and release memory, not using swap since it's causing capture to glitch/fault
    menu_item_views.clear();
}

void BtnGridView::add_items(std::initializer_list<GridItem> new_items) {
    for (auto item : new_items) {
        if (!blacklisted_app(item))
            menu_items.push_back(item);
    }

    update_items();
}

void BtnGridView::add_item(GridItem new_item) {
    if (!blacklisted_app(new_item)) {
        menu_items.push_back(new_item);
        update_items();
    }
}

void BtnGridView::insert_item(GridItem new_item, uint8_t position) {
    if (!blacklisted_app(new_item)) {
        if (position < menu_items.size()) {
            auto pos_iter = menu_items.begin() + position;
            menu_items.insert(pos_iter, new_item);
            update_items();
        } else {
            menu_items.push_back(new_item);
            update_items();
        }
    }
}

void BtnGridView::update_items() {
    size_t i = 0;
    Color bg_color = portapack::persistent_memory::menu_color();

    if ((menu_items.size()) > (displayed_max + offset)) {
        more = true;
        blink = true;
    } else
        more = false;

    for (auto& item : menu_item_views) {
        if ((i + offset) >= menu_items.size()) {
            item->hidden(true);
            item->set_text(" ");
            item->set_bitmap(nullptr);
            item->on_select = []() {};
            item->set_dirty();
        } else {
            // Assign item data to NewButtons according to offset
            item->hidden(false);
            item->set_text(menu_items[i + offset].text);
            item->set_bitmap(menu_items[i + offset].bitmap);
            item->set_color(menu_items[i + offset].color);
            item->set_bg_color(bg_color);
            item->on_select = menu_items[i + offset].on_select;
            item->set_dirty();
        }

        i++;
    }
}

NewButton* BtnGridView::item_view(size_t index) const {
    return menu_item_views[index].get();
}

bool BtnGridView::set_highlighted(int32_t new_value) {
    int32_t item_count = (int32_t)menu_items.size();

    if (new_value < 0)
        return false;

    if (new_value >= item_count)
        new_value = item_count - 1;

    if (((uint32_t)new_value > offset) && ((new_value - offset) >= displayed_max)) {
        // Shift BtnGridView up
        highlighted_item = new_value;
        // rounding up new offset to next multiple of rows
        offset = new_value - displayed_max + rows_;
        offset -= (offset % rows_);
        update_items();
        // refresh whole screen (display flickers) only if scrolling last row up and a blank button is needed at the bottom
        if ((new_value + rows_ >= item_count) && (item_count % rows_) != 0)
            set_dirty();
    } else if ((uint32_t)new_value < offset) {
        // Shift BtnGridView down
        highlighted_item = new_value;
        offset = (new_value / rows_) * rows_;
        update_items();
        // no need to set_dirty() here since all buttons have been repainted
    } else {
        // Just update highlight
        highlighted_item = new_value;
    }

    if (visible())
        item_view(highlighted_item - offset)->focus();

    return true;
}

uint32_t BtnGridView::highlighted_index() {
    return highlighted_item;
}

void BtnGridView::on_focus() {
    item_view(highlighted_item - offset)->focus();
}

void BtnGridView::on_blur() {
#if 0
    if (!keep_highlight)
        item_view(highlighted_item - offset)->unhighlight();
#endif
}

void BtnGridView::on_show() {
    on_populate();

    View::on_show();
}

void BtnGridView::on_hide() {
    View::on_hide();

    clear();
}

bool BtnGridView::on_key(const KeyEvent key) {
    switch (key) {
        case KeyEvent::Up:
            return set_highlighted(highlighted_item - rows_);

        case KeyEvent::Down:
            return set_highlighted(highlighted_item + rows_);

        case KeyEvent::Right:
            return set_highlighted(highlighted_item + 1);

        case KeyEvent::Left:
            return set_highlighted(highlighted_item - 1);

        case KeyEvent::Select:
            if (menu_items[highlighted_item].on_select) {
                menu_items[highlighted_item].on_select();
            }
            return true;

        default:
            return false;
    }
}

bool BtnGridView::on_encoder(const EncoderEvent event) {
    return set_highlighted(highlighted_item + event);
}

/* BlackList ******************************************************/

std::unique_ptr<char> blacklist_ptr{};
size_t blacklist_len{};

void load_blacklist() {
    File f;

    auto error = f.open(BLACKLIST);
    if (error)
        return;

    // allocating two extra bytes for leading & trailing commas
    blacklist_ptr = std::unique_ptr<char>(new char[f.size() + 2]);
    if (f.read(blacklist_ptr.get() + 1, f.size())) {
        blacklist_len = f.size() + 2;

        // replace any CR/LF characters with comma delineator, and add comma prefix/suffix, to simplify searching
        char* ptr = blacklist_ptr.get();
        *ptr = ',';
        *(ptr + blacklist_len - 1) = ',';
        for (size_t i = 0; i < blacklist_len; i++, ptr++) {
            if (*ptr == 0x0D || *ptr == 0x0A)
                *ptr = ',';
        }
    }
}

bool BtnGridView::blacklisted_app(GridItem new_item) {
    std::string app_name = "," + new_item.text + ",";

    if (blacklist_len < app_name.size())
        return false;

    return std::search(blacklist_ptr.get(), blacklist_ptr.get() + blacklist_len, app_name.begin(), app_name.end()) < blacklist_ptr.get() + blacklist_len;
}

} /* namespace ui */
