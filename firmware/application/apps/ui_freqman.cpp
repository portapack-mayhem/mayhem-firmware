/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2023 Kyle Reed
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

#include "ui_freqman.hpp"

#include "event_m0.hpp"
#include "portapack.hpp"
#include "utility.hpp"

#include "debug.hpp"

#include <memory>

using namespace portapack;
namespace fs = std::filesystem;

namespace ui {

/* FreqManBaseView ***************************************/

size_t FreqManBaseView::current_category_index = 0;

FreqManBaseView::FreqManBaseView(
    NavigationView& nav)
    : nav_(nav) {
    add_children(
        {&label_category,
         &options_category,
         &button_exit});

    options_category.on_change = [this](size_t new_index, int32_t) {
        change_category(new_index);
    };

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };

    refresh_list();
};

void FreqManBaseView::focus() {
    button_exit.focus();

    // TODO: Shouldn't be on focus.
    if (error_ == ERROR_ACCESS) {
        nav_.display_modal("Error", "File access error", ABORT, nullptr);
    } else if (error_ == ERROR_NOFILES) {
        nav_.display_modal("Error", "No database files\nin /FREQMAN", ABORT, nullptr);
    } else {
        options_category.focus();
    }
}

void FreqManBaseView::change_category(size_t new_index) {
    if (categories().empty())
        return;

    current_category_index = new_index;
    if (!db_.open(get_freqman_path(current_category()))) {
        error_ = ERROR_ACCESS;
    }

    freqlist_view.set_db(db_);
}

void FreqManBaseView::refresh_list() {
    OptionsField::options_t new_categories;

    scan_root_files(
        freqman_dir, u"*.TXT", [&new_categories](const fs::path& path) {
            // Skip temp/hidden files.
            if (path.empty() || path.native()[0] == u'.')
                return;

            // The UI layer will truncate long file names when displaying.
            new_categories.emplace_back(path.stem().string(), new_categories.size());
        });

    // Alphabetically sort the categories.
    std::sort(new_categories.begin(), new_categories.end(), [](auto& left, auto& right) {
        return left.first < right.first;
    });

    // Preserve last selection; ensure in range.
    current_category_index = clip(current_category_index, 0u, new_categories.size());
    options_category.set_options(std::move(new_categories));
    options_category.set_selected_index(current_category_index);
}

/* FrequencySaveView *************************************/

void FrequencySaveView::save_current_file() {
    // save_freqman_file(current_category(), database);
    nav_.pop();
}

void FrequencySaveView::on_save_name() {
    text_prompt(nav_, desc_buffer, 28, [this](std::string&) {
        // database.push_back(std::make_unique<freqman_entry>(freqman_entry{value_, 0, buffer, freqman_type::Single}));
        save_current_file();
    });
}

void FrequencySaveView::on_save_timestamp() {
    // database.push_back(std::make_unique<freqman_entry>(freqman_entry{value_, 0, live_timestamp.string(), freqman_type::Single}));
    save_current_file();
}

FrequencySaveView::FrequencySaveView(
    NavigationView& nav,
    const rf::Frequency value)
    : FreqManBaseView(nav),
      value_(value) {
    desc_buffer.reserve(28);

    add_children(
        {&labels,
         &big_display,
         &button_save_name,
         &button_save_timestamp,
         &live_timestamp});

    big_display.set(value);

    button_save_name.on_select = [this, &nav](Button&) {
        on_save_name();
    };
    button_save_timestamp.on_select = [this, &nav](Button&) {
        on_save_timestamp();
    };
    options_category.on_change = [this, value](size_t category_id, int32_t) {
        change_category(category_id);
        big_display.set(value);  // ???
    };
}

/* FrequencyLoadView *************************************/

FrequencyLoadView::FrequencyLoadView(
    NavigationView& nav)
    : FreqManBaseView(nav) {
    add_children({&freqlist_view});

    // Resize to fill screen. +2 keeps text out of border.
    freqlist_view.set_parent_rect({0, 3 * 8, screen_width, 15 * 16 + 2});

    freqlist_view.on_select = [&nav, this](size_t index) {
        auto entry = db_[index];
        // TODO: Maybe return center of range if user choses a range when the app
        // needs a unique frequency, instead of frequency_a?
        auto has_range = entry.type == freqman_type::Range ||
                         entry.type == freqman_type::HamRadio;

        if (on_range_loaded && has_range)
            on_range_loaded(entry.frequency_a, entry.frequency_b);
        else if (on_frequency_loaded)
            on_frequency_loaded(entry.frequency_a);

        nav_.pop();  // NB: this will call dtor.
    };
    freqlist_view.on_leave = [this]() {
        button_exit.focus();
    };
}

/* FrequencyManagerView **********************************/
// TODO: Why are all saves immediate?

void FrequencyManagerView::on_edit_freq(rf::Frequency) {
    // database[freqlist_view.get_index()]->frequency_a = f;
    //  Save every time? Seems expensive.
    // save_freqman_file(file_list[categories[current_category_index].second], database);
    // change_category(current_category_index); ??? Refresh
}

void FrequencyManagerView::on_edit_desc(NavigationView& nav) {
    text_prompt(nav, desc_buffer, 28, [this](std::string&) {
        // database[freqlist_view.get_index()]->description = std::move(buffer);
        //  Save every time? Seems expensive.
        // save_freqman_file(file_list[categories[current_category_index].second], database);
        // change_category(current_category_index);  ??? Refresh
    });
}

void FrequencyManagerView::on_new_category(NavigationView& nav) {
    text_prompt(nav, desc_buffer, 12, [this](std::string& new_name) {
        create_freqman_file(new_name);
        refresh_list();
    });
}

void FrequencyManagerView::on_delete() {
    // if (db_.empty()) {
    //     delete_freqman_file(current_category());
    //     refresh_list();
    // } else {
    //     // TODO: clear
    //     // database.erase(database.begin() + freqlist_view.get_index());
    //     // save_freqman_file(file_list[categories[current_category_id].second], database);
    // }
    // change_category(current_category_index);
}

FrequencyManagerView::FrequencyManagerView(
    NavigationView& nav)
    : FreqManBaseView(nav) {
    add_children(
        {&freqlist_view,
         &labels,
         &button_new_category,
         &button_edit_freq,
         &button_edit_desc,
         &button_delete});

    freqlist_view.on_select = [this](size_t) {
        button_edit_freq.focus();
    };
    // Allows for quickly exiting control.
    freqlist_view.on_leave = [this]() {
        button_edit_freq.focus();
    };

    button_new_category.on_select = [this, &nav](Button&) {
        desc_buffer.clear();
        on_new_category(nav);
    };

    button_edit_freq.on_select = [this, &nav](Button&) {
        // TODO: Init, if empty.

        auto new_view = nav.push<FrequencyKeypadView>(current_entry().frequency_a);
        new_view->on_changed = [this](rf::Frequency f) {
            on_edit_freq(f);
        };
    };

    button_edit_desc.on_select = [this, &nav](Button&) {
        // TODO: Init, if empty.

        desc_buffer = current_entry().description;
        on_edit_desc(nav);
    };

    button_delete.on_select = [this, &nav](Button&) {
        on_delete();
    };
}

}  // namespace ui
