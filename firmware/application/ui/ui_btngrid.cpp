/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2019 Elia Yehuda (z4ziggy)
 * Copyright (C) 2023 Mark Thompson
 * Copyright (C) 2024 u-foka
 * copyleft 2024 zxkmm AKA zix aka sommermorgentraum
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
#include "sd_card.hpp"
#include <algorithm>
#include <climits>

namespace ui {

// Maps an app display name to its config.ini icon color; grey if unlisted. Defined with the loader below.
static Color app_color_from_config(const std::string& app_name);

/* BtnGridView **************************************************************/

BtnGridView::BtnGridView(
    Rect new_parent_rect,
    bool keep_highlight)
    : keep_highlight{keep_highlight} {
    set_parent_rect(new_parent_rect);
    set_focusable(true);
    if (screen_height == 480) {
        button_h = 64;
    }
    button_pgup.set_focusable(false);
    button_pgup.on_select = [this](Button&) {
        page_up();
    };

    button_pgdown.set_focusable(false);
    button_pgdown.on_select = [this](Button&) {
        page_down();
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

    int space_available = parent_rect().size().height() - 16;  // leave space for arrows
    displayed_max = (parent_rect().size().height() / button_h);

    button_pgup.set_parent_rect({0, (Coord)(space_available), screen_width / 2, 16});
    button_pgdown.set_parent_rect({screen_width / 2, (Coord)(space_available), screen_width / 2, 16});

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
        item->set_vertical_center(true);
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
    // clear vector, not using swap since it's causing capture to glitch/fault
    menu_items.clear();

    // TODO(u-foka): Clean up my mess, move this somewhere to clear memory when the view is not visible, but not to be confused with clearing the menu items...
    for (auto& item : menu_item_views)
        remove_child(item.get());

    // clear vector, not using swap since it's causing capture to glitch/fault
    menu_item_views.clear();
}

void BtnGridView::add_items(std::initializer_list<GridItem> new_items, bool inhibit_update) {
    for (const auto& item : new_items) {
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
    // if there are no menu items, disable both arrows and avoid size-1 underflow
    if (menu_items.empty()) {
        set_arrow_up_enabled(false);
        set_arrow_down_enabled(false);
        return;
    }

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

void BtnGridView::reload_items() {
    menu_items.clear();
    on_populate();
    // Apply config.ini line order to this grid (home menu opts out to keep its fixed layout).
    if (config_reorder_enabled())
        reorder_menu_items();
    set_highlighted(highlighted_item, true);
    show_hide_arrows();

    set_dirty();  // Redraw the now potentially empty space as well
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
            item->set_color(app_color_from_config(menu_items[i + offset].text));  // icon color from config.ini
            item->set_progress_color(menu_items[i + offset].color);               // progress block keeps original color
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

bool BtnGridView::set_highlighted(int32_t new_value, bool force_update) {
    int32_t item_count = (int32_t)menu_items.size();

    // nothing to highlight when the list is empty
    if (item_count == 0) {
        highlighted_item = 0;
        offset = 0;
        show_hide_arrows();
        if (force_update) {
            update_items();
        }
        return false;
    }

    if (new_value < 0)
        return false;

    if (new_value >= item_count) {
        new_value = item_count - 1;
    }

    bool needs_update = false;

    if (((uint32_t)new_value > offset) && ((new_value - offset) >= displayed_max)) {
        // Shift BtnGridView up
        highlighted_item = new_value;
        // rounding up new offset to next multiple of rows
        offset = new_value - displayed_max + rows_;
        offset -= (offset % rows_);
        needs_update = true;
        // refresh whole screen (display flickers) only if scrolling last row up and a blank button is needed at the bottom
        if ((new_value + rows_ >= item_count) && (item_count % rows_) != 0)
            set_dirty();
    } else if ((uint32_t)new_value < offset) {
        // Shift BtnGridView down
        highlighted_item = new_value;
        offset = (new_value / rows_) * rows_;
        needs_update = true;
        // no need to set_dirty() here since all buttons have been repainted
    } else {
        // Just update highlight
        highlighted_item = new_value;
    }

    // Normalize offset to show maximum items when count decreased
    if (item_count > 0 && offset + displayed_max > static_cast<size_t>(item_count)) {
        if (static_cast<size_t>(item_count) >= displayed_max) {
            offset = item_count - displayed_max;
        } else {
            offset = 0;
        }
        needs_update = true;
    }

    if (needs_update || force_update) {
        update_items();
    }

    if (drawn()) {
        size_t idx = highlighted_item - offset;
        if (idx < menu_item_views.size())
            item_view(idx)->focus();
    }

    show_hide_arrows();

    return true;
}

uint32_t BtnGridView::highlighted_index() {
    return highlighted_item;
}

void BtnGridView::on_focus() {
    if (!menu_items.empty()) {
        size_t idx = highlighted_item - offset;
        if (idx < menu_item_views.size())
            item_view(idx)->focus();
    }
}

void BtnGridView::on_blur() {
#if 0
        if (!keep_highlight)
            item_view(highlighted_item - offset)->unhighlight();
#endif
}

void BtnGridView::on_show() {
    View::on_show();

    sd_card_status_signal_token = sd_card::status_signal += [this](const sd_card::Status /*status*/) {
        this->reload_items();
    };

    reload_items();
}

void BtnGridView::on_hide() {
    sd_card::status_signal -= sd_card_status_signal_token;

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
            if (!menu_items.empty() && highlighted_item < menu_items.size()) {
                if (menu_items[highlighted_item].on_select) {
                    menu_items[highlighted_item].on_select();
                }
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

std::string blacklist_data{};

void load_blacklist() {
    File f;
    auto error = f.open(BLACKLIST);
    if (error)
        return;
    // Resize string to fit file + 2 commas, filling it with commas by default
    blacklist_data.assign(f.size() + 2, ',');
    // Read directly into the string's buffer (offset by 1 to leave the first comma)
    if (f.read(blacklist_data.data() + 1, f.size())) {
        // Replace any CR/LF characters with commas
        for (char& c : blacklist_data) {
            if (c == '\r' || c == '\n') {
                c = ',';
            }
        }
    } else {
        blacklist_data.clear();  // Clear if read fails
    }
}

bool BtnGridView::blacklisted_app(GridItem new_item) {
    std::string app_name = "," + new_item.text + ",";
    if (blacklist_data.size() < app_name.size())
        return false;

    return blacklist_data.find(app_name) != std::string::npos;
}

/* App icon color overrides ******************************************/

std::string app_colors_data{};

// Load config.ini into a buffer bracketed by '\n' so every record reads as "\nname,c".
void load_app_colors() {
    File f;
    auto error = f.open(APP_COLORS);
    if (error)
        return;
    // Resize to fit file + 2 newlines, prefilled with '\n' (first byte stays as the leading delimiter)
    app_colors_data.assign(f.size() + 2, '\n');
    if (f.read(app_colors_data.data() + 1, f.size())) {
        // Normalize CR to '\n' so both LF and CRLF files parse the same
        for (char& c : app_colors_data)
            if (c == '\r') c = '\n';
    } else {
        app_colors_data.clear();  // Clear if read fails
    }
}

// Find "\nname," and translate the following abbrev char to a color. Unlisted/undefined -> grey.
static Color app_color_from_config(const std::string& app_name) {
    if (app_colors_data.size() < app_name.size() + 3)
        return Color::grey();

    const std::string key = "\n" + app_name + ",";
    const auto pos = app_colors_data.find(key);
    if (pos == std::string::npos)
        return Color::grey();

    const size_t ci = pos + key.size();
    if (ci >= app_colors_data.size())
        return Color::grey();

    switch (app_colors_data[ci]) {
        case 'r': return Color::red();
        case 'g': return Color::green();
        case 'b': return Color::blue();
        case 'y': return Color::yellow();
        case 'c': return Color::cyan();
        case 'm': return Color::magenta();
        case 'o': return Color::orange();
        case 'p': return Color::purple();
        case 'w': return Color::white();
        default:  return Color::grey();
    }
}

/* App menu ordering *************************************************/

// Return the 0-based line index of app_name in config.ini, or -1 if not listed.
// The line order in config.ini is what determines on-screen position, so the file
// needs no explicit position numbers. app_colors_data starts with a '\n' sentinel
// and separates records by '\n', so the count of newlines strictly before the match
// is exactly the record's 0-based index.
static int app_config_order(const std::string& app_name) {
    if (app_colors_data.empty())
        return -1;

    const std::string key = "\n" + app_name + ",";
    const auto pos = app_colors_data.find(key);
    if (pos == std::string::npos)
        return -1;

    int index = 0;
    for (size_t i = 0; i < pos; i++)
        if (app_colors_data[i] == '\n')
            index++;
    return index;
}

// Non-app control tiles that must stay pinned to the front of a grid regardless of
// config.ini. Matches the literals used to create these tiles in ui_navigation.cpp
// (".." return icon, "ExtAppErr" no-SD/read-error notice).
static bool is_pinned_tile(const std::string& text) {
    return text == ".." || text == "ExtAppErr";
}

// Sort rank for a grid item: pinned tiles first, then config-listed apps by line
// order, then unlisted apps last. Equal ranks keep original order via stable_sort.
static int item_sort_rank(const GridItem& item) {
    if (is_pinned_tile(item.text))
        return INT_MIN;
    const int order = app_config_order(item.text);
    return (order >= 0) ? order : INT_MAX;
}

// Reposition items to: [pinned control tiles] [config-listed apps in file order]
// [unlisted apps, original order]. Missing apps listed in config are ignored for
// free -- they simply have no item here, so their line index is an unused gap.
void BtnGridView::reorder_menu_items() {
    if (menu_items.empty())
        return;

    std::stable_sort(menu_items.begin(), menu_items.end(),
                     [](const GridItem& a, const GridItem& b) {
                         return item_sort_rank(a) < item_sort_rank(b);
                     });
}

void BtnGridView::page_up() {
    if (arrow_up_enabled) {
        size_t item_count = menu_items.size();
        if (item_count == 0)
            return;

        size_t new_offset;
        if (offset > displayed_max) {
            new_offset = offset - displayed_max;
        } else {
            new_offset = 0;
        }

        // If we can't move further, just move highlight to start
        if (new_offset == offset) {
            set_highlighted(0);
            return;
        }

        bool was_visible = (highlighted_item >= new_offset && highlighted_item < new_offset + displayed_max);

        offset = new_offset;
        update_items();

        if (was_visible) {
            if (drawn()) {
                size_t idx = highlighted_item - offset;
                if (idx < menu_item_views.size())
                    item_view(idx)->focus();
            }
        } else {
            // focus last item on the new page (clamp to last item overall)
            size_t last_on_page = std::min(new_offset + displayed_max, item_count) - 1;
            set_highlighted((int)last_on_page);
        }
    }
}

void BtnGridView::page_down() {
    if (arrow_down_enabled) {
        size_t item_count = menu_items.size();
        if (item_count == 0)
            return;

        size_t max_offset;
        if (item_count > displayed_max) {
            max_offset = item_count - displayed_max;
        } else {
            max_offset = 0;
        }
        size_t new_offset = std::min(offset + displayed_max, max_offset);

        // If we can't move further, just move highlight to last
        if (new_offset == offset) {
            set_highlighted((int)(item_count - 1));
            return;
        }

        bool was_visible = (highlighted_item >= new_offset && highlighted_item < new_offset + displayed_max);

        offset = new_offset;
        update_items();

        if (was_visible) {
            if (drawn()) {
                size_t idx = highlighted_item - offset;
                if (idx < menu_item_views.size())
                    item_view(idx)->focus();
            }
        } else {
            // focus first item on the new page
            set_highlighted((int)new_offset);
        }
    }
}

} /* namespace ui */
