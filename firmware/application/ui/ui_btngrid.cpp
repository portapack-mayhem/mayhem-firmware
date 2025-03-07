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

    button_pgup.set_focusable(false);
    button_pgup.on_select = [this](Button&) {
        if (arrow_up_enabled) {
            if (((int64_t)highlighted_item - displayed_max) > 0)
                set_highlighted(highlighted_item - displayed_max);
            else
                set_highlighted(0);
        }
    };

    button_pgdown.set_focusable(false);
    button_pgdown.on_select = [this](Button&) {
        if (arrow_down_enabled) {
            set_highlighted(highlighted_item + displayed_max);
        }
    };

    button_pgup.set_style(Theme::getInstance()->bg_darkest_small);
    button_pgdown.set_style(Theme::getInstance()->bg_darkest_small);

    add_child(&button_pgup);
    add_child(&button_pgdown);
}

BtnGridView::~BtnGridView() {
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

    button_pgup.set_parent_rect({0, (Coord)(displayed_max * button_h), 120, 16});
    button_pgdown.set_parent_rect({120, (Coord)(displayed_max * button_h), 120, 16});

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

void BtnGridView::set_arrow_up_enabled(bool enabled) {
    if (!show_arrows)
        return;
    if (enabled) {
        if (!arrow_up_enabled) {
            arrow_up_enabled = true;
            button_pgup.set_text("< PREV");
        }
    } else if (!enabled) {
        if (arrow_up_enabled) {
            arrow_up_enabled = false;
            button_pgup.set_text("      ");
        }
    }
};

void BtnGridView::set_arrow_down_enabled(bool enabled) {
    if (!show_arrows)
        return;
    if (enabled) {
        if (!arrow_down_enabled) {
            arrow_down_enabled = true;
            button_pgdown.set_text("NEXT >");
        }
    } else if (!enabled) {
        if (arrow_down_enabled) {
            arrow_down_enabled = false;
            button_pgdown.set_text("      ");
        }
    }
};

void BtnGridView::clear() {
    // clear vector and release memory, not using swap since it's causing capture to glitch/fault
    menu_items.clear();

    for (auto& item : menu_item_views)
        remove_child(item.get());

    // clear vector and release memory, not using swap since it's causing capture to glitch/fault
    menu_item_views.clear();
}

void BtnGridView::add_items(std::initializer_list<GridItem> new_items, bool inhibit_update) {
    for (auto item : new_items) {
        if (!blacklisted_app(item))
            menu_items.push_back(item);
    }

    if (!inhibit_update) {
        update_items();
    }
}

void BtnGridView::add_item(const GridItem& new_item, bool inhibit_update) {
    if (!blacklisted_app(new_item)) {
        menu_items.push_back(new_item);
        if (!inhibit_update) {
            update_items();
        }
    }
}

void BtnGridView::insert_item(const GridItem& new_item, size_t position, bool inhibit_update) {
    if (!blacklisted_app(new_item)) {
        if (position < menu_items.size()) {
            auto pos_iter = menu_items.begin() + position;
            menu_items.insert(pos_iter, new_item);
        } else {
            menu_items.push_back(new_item);
        }

        if (!inhibit_update) {
            update_items();
        }
    }
}

void BtnGridView::show_hide_arrows() {
    if (highlighted_item == 0) {
        set_arrow_up_enabled(false);
    } else {
        set_arrow_up_enabled(true);
    }
    if (highlighted_item == (menu_items.size() - 1)) {
        set_arrow_down_enabled(false);
    } else {
        set_arrow_down_enabled(true);
    }
}

void BtnGridView::update_items() {
    size_t i = 0;

    Color bg_color = portapack::persistent_memory::menu_color();

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

void BtnGridView::show_arrows_enabled(bool enabled) {
    show_arrows = enabled;
    if (!enabled) {
        remove_child(&button_pgup);
        remove_child(&button_pgdown);
    }
}

bool BtnGridView::set_highlighted(int32_t new_value) {
    int32_t item_count = (int32_t)menu_items.size();

    if (new_value < 0)
        return false;

    if (new_value >= item_count) {
        new_value = item_count - 1;
    }

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

    show_hide_arrows();

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
    set_highlighted(highlighted_item);
}

void BtnGridView::on_hide() {
    View::on_hide();
    clear();
    set_arrow_up_enabled(false);
    set_arrow_down_enabled(false);
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
