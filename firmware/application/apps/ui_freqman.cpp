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
#include "rtc_time.hpp"
#include "utility.hpp"

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

    refresh_categories();
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

void FreqManBaseView::refresh_categories() {
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
    auto saved_index = current_category_index;
    options_category.set_options(std::move(new_categories));
    options_category.set_selected_index(saved_index);
}

void FreqManBaseView::refresh_list(int delta_selected) {
    // Update the index and ensures in bounds.
    freqlist_view.set_index(freqlist_view.get_index() + delta_selected);
    freqlist_view.set_dirty();
}

/* FrequencySaveView *************************************/

FrequencySaveView::FrequencySaveView(
    NavigationView& nav,
    const rf::Frequency value)
    : FreqManBaseView(nav) {
    add_children(
        {&labels,
         &big_display,
         &button_clear,
         &button_edit,
         &button_save,
         &text_description});

    entry_.type = freqman_type::Single;
    entry_.frequency_a = value;
    entry_.description = to_string_timestamp(rtc_time::now());
    refresh_ui();

    button_clear.on_select = [this, &nav](Button&) {
        entry_.description = "";
        refresh_ui();
    };

    button_edit.on_select = [this, &nav](Button&) {
        temp_buffer_ = entry_.description;
        text_prompt(nav_, temp_buffer_, desc_edit_max, [this](std::string& new_desc) {
            entry_.description = new_desc;
            refresh_ui();
        });
    };

    button_save.on_select = [this, &nav](Button&) {
        db_.insert_entry(entry_, db_.entry_count());
        nav_.pop();
    };
}

void FrequencySaveView::refresh_ui() {
    big_display.set(entry_.frequency_a);
    text_description.set(entry_.description);
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

void FrequencyManagerView::on_edit_freq() {
    // TODO: range edit support.
    auto freq_edit_view = nav_.push<FrequencyKeypadView>(current_entry().frequency_a);
    freq_edit_view->on_changed = [this](rf::Frequency f) {
        auto entry = current_entry();
        entry.frequency_a = f;
        db_.replace_entry(current_index(), entry);
        freqlist_view.set_dirty();
    };
}

void FrequencyManagerView::on_edit_desc() {
    temp_buffer_ = current_entry().description;
    text_prompt(nav_, temp_buffer_, desc_edit_max, [this](std::string& new_desc) {
        auto entry = current_entry();
        entry.description = std::move(new_desc);
        db_.replace_entry(current_index(), entry);
        freqlist_view.set_dirty();
    });
}

void FrequencyManagerView::on_add_category() {
    temp_buffer_.clear();
    text_prompt(nav_, temp_buffer_, 20, [this](std::string& new_name) {
        if (!new_name.empty()) {
            create_freqman_file(new_name);
            refresh_categories();
        }
    });
}

void FrequencyManagerView::on_del_category() {
    nav_.push<ModalMessageView>(
        "Delete", "Delete " + current_category() + "\nAre you sure?", YESNO,
        [this](bool choice) {
            if (choice) {
                db_.close();  // Ensure file is closed.
                auto path = get_freqman_path(current_category());
                delete_file(path);
                refresh_categories();
            }
        });
}

void FrequencyManagerView::on_add_entry() {
    freqman_entry entry{
        .frequency_a = 100'000'000,
        .description = std::string{"Entry "} + to_string_dec_uint(db_.entry_count()),
        .type = freqman_type::Single,
    };

    // Add will insert below the currently selected item.
    db_.insert_entry(entry, current_index() + 1);
    refresh_list(1);
}

void FrequencyManagerView::on_del_entry() {
    if (db_.empty())
        return;

    nav_.push<ModalMessageView>(
        "Delete", "Delete '" + pretty_string(current_entry(), 23) + "'\nAre you sure?", YESNO,
        [this](bool choice) {
            if (choice) {
                db_.delete_entry(current_index());
                refresh_list();
            }
        });
}

FrequencyManagerView::FrequencyManagerView(
    NavigationView& nav)
    : FreqManBaseView(nav) {
    add_children(
        {&freqlist_view,
         &labels,
         &button_add_category,
         &button_del_category,
         &button_edit_freq,
         &button_edit_desc,
         &button_add_entry,
         &button_del_entry});

    freqlist_view.on_select = [this](size_t) {
        button_edit_freq.focus();
    };
    // Allows for quickly exiting control.
    freqlist_view.on_leave = [this]() {
        button_edit_freq.focus();
    };

    button_add_category.on_select = [this]() {
        on_add_category();
    };

    button_del_category.on_select = [this]() {
        on_del_category();
    };

    button_edit_freq.on_select = [this](Button&) {
        on_edit_freq();
    };

    button_edit_desc.on_select = [this](Button&) {
        on_edit_desc();
    };

    button_add_entry.on_select = [this]() {
        on_add_entry();
    };

    button_del_entry.on_select = [this]() {
        on_del_entry();
    };
}

}  // namespace ui
