/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_freqman.hpp"

#include "portapack.hpp"
#include "event_m0.hpp"

using namespace portapack;

namespace ui {

static int32_t current_category_id = 0;

FreqManBaseView::FreqManBaseView(
    NavigationView& nav)
    : nav_(nav) {
    add_children({&options_category,
                  &label_category,
                  &button_exit});

    // initialize
    refresh_list();
    options_category.on_change = [this](size_t category_id, int32_t) {
        change_category(category_id);
    };
    options_category.set_selected_index(current_category_id);

    button_exit.on_select = [this, &nav](Button&) {
        nav.pop();
    };
};

void FreqManBaseView::focus() {
    button_exit.focus();

    if (error_ == ERROR_ACCESS) {
        nav_.display_modal("Error", "File acces error", ABORT, nullptr);
    } else if (error_ == ERROR_NOFILES) {
        nav_.display_modal("Error", "No database files\nin /freqman", ABORT, nullptr);
    } else {
        options_category.focus();
    }
}

void FreqManBaseView::get_freqman_files() {
    std::vector<std::string>().swap(file_list);

    auto files = scan_root_files(u"FREQMAN", u"*.TXT");

    for (auto file : files) {
        std::string file_name = file.stem().string();
        // don't propose tmp / hidden files in freqman's list
        if (file_name.length() && file_name[0] != '.') {
            file_list.emplace_back(file_name);
        }
    }
};

void FreqManBaseView::change_category(int32_t category_id) {
    current_category_id = category_id;

    if (file_list.empty()) return;

    if (!load_freqman_file(file_list[categories[category_id].second], database)) {
        error_ = ERROR_ACCESS;
    }
    freqlist_view.set_db(database);
    text_empty.hidden(!database.empty());
    set_dirty();
}

void FreqManBaseView::refresh_list() {
    categories.clear();
    get_freqman_files();

    for (size_t n = 0; n < file_list.size(); n++)
        categories.emplace_back(std::make_pair(file_list[n].substr(0, 14), n));

    // Alphabetical sort
    std::sort(categories.begin(), categories.end(), [](auto& left, auto& right) {
        return left.first < right.first;
    });

    options_category.set_options(categories);
    if ((unsigned)current_category_id >= categories.size())
        current_category_id = categories.size() - 1;
}

void FrequencySaveView::save_current_file() {
    save_freqman_file(file_list[categories[current_category_id].second], database);
    nav_.pop();
}

void FrequencySaveView::on_save_name() {
    text_prompt(nav_, desc_buffer, 28, [this](std::string& buffer) {
        database.push_back({value_, 0, buffer, SINGLE});
        save_current_file();
    });
}

void FrequencySaveView::on_save_timestamp() {
    database.push_back({value_, 0, live_timestamp.string(), SINGLE});
    save_current_file();
}

FrequencySaveView::FrequencySaveView(
    NavigationView& nav,
    const rf::Frequency value)
    : FreqManBaseView(nav),
      value_(value) {
    desc_buffer.reserve(28);

    // Todo: add back ?
    /*for (size_t n = 0; n < database.size(); n++) {
      if (database[n].value == value_) {
      error_ = ERROR_DUPLICATE;
      break;
      }
      }*/

    add_children({&labels,
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
        big_display.set(value);
    };
}

FrequencyLoadView::~FrequencyLoadView() {
    std::vector<freqman_entry>().swap(database);
}

void FrequencyLoadView::refresh_widgets(const bool v) {
    freqlist_view.hidden(v);
    text_empty.hidden(!v);
    // display.fill_rectangle(freqlist_view.screen_rect(), Color::black());
    set_dirty();
}

FrequencyLoadView::FrequencyLoadView(
    NavigationView& nav)
    : FreqManBaseView(nav) {
    on_refresh_widgets = [this](bool v) {
        refresh_widgets(v);
    };

    add_children({&freqlist_view,
                  &text_empty});

    // Resize menu view to fill screen
    freqlist_view.set_parent_rect({0, 3 * 8, 240, 30 * 8});

    freqlist_view.on_select = [&nav, this](FreqManUIList&) {
        auto& entry = database[freqlist_view.get_index()];
        if (entry.type == RANGE) {
            // User chose a frequency range entry
            if (on_range_loaded)
                on_range_loaded(entry.frequency_a, entry.frequency_b);
            else if (on_frequency_loaded)
                on_frequency_loaded(entry.frequency_a);
            // TODO: Maybe return center of range if user choses a range when the app needs a unique frequency, instead of frequency_a ?
        } else {
            // User chose an unique frequency entry
            if (on_frequency_loaded)
                on_frequency_loaded(entry.frequency_a);
        }
        // swap with empty vector to ensure memory is immediately released
        std::vector<freqman_entry>().swap(database);
        nav_.pop();
    };
}

void FrequencyManagerView::on_edit_freq(rf::Frequency f) {
    database[freqlist_view.get_index()].frequency_a = f;
    save_freqman_file(file_list[categories[current_category_id].second], database);
    change_category(current_category_id);
}

void FrequencyManagerView::on_edit_desc(NavigationView& nav) {
    text_prompt(nav, desc_buffer, 28, [this](std::string& buffer) {
        database[freqlist_view.get_index()].description = buffer;
        save_freqman_file(file_list[categories[current_category_id].second], database);
        change_category(current_category_id);
    });
}

void FrequencyManagerView::on_new_category(NavigationView& nav) {
    text_prompt(nav, desc_buffer, 12, [this](std::string& buffer) {
        File freqman_file;
        create_freqman_file(buffer, freqman_file);
        refresh_list();
        change_category(current_category_id);
    });
}

void FrequencyManagerView::on_delete() {
    if (database.empty()) {
        delete_freqman_file(file_list[categories[current_category_id].second]);
        refresh_list();
    } else {
        database.erase(database.begin() + freqlist_view.get_index());
        save_freqman_file(file_list[categories[current_category_id].second], database);
    }
    change_category(current_category_id);
}

void FrequencyManagerView::refresh_widgets(const bool v) {
    button_edit_freq.hidden(v);
    button_edit_desc.hidden(v);
    button_delete.hidden(v);
    text_empty.hidden(!v);
    freqlist_view.hidden(v);
    labels.hidden(v);
    // display.fill_rectangle(freqlist_view.screen_rect(), Color::black());
    set_dirty();
}

FrequencyManagerView::~FrequencyManagerView() {
    // save_freqman_file(file_list[categories[current_category_id].second], database);
}

FrequencyManagerView::FrequencyManagerView(
    NavigationView& nav)
    : FreqManBaseView(nav) {
    on_refresh_widgets = [this](bool v) {
        refresh_widgets(v);
    };

    add_children({&labels,
                  &button_new_category,
                  &freqlist_view,
                  &text_empty,
                  &button_edit_freq,
                  &button_edit_desc,
                  &button_delete});

    freqlist_view.on_select = [this](FreqManUIList&) {
        button_edit_freq.focus();
    };

    button_new_category.on_select = [this, &nav](Button&) {
        desc_buffer = "";
        on_new_category(nav);
    };

    button_edit_freq.on_select = [this, &nav](Button&) {
        if (database.empty()) {
            database.push_back({0, 0, "", SINGLE});
        }
        auto new_view = nav.push<FrequencyKeypadView>(database[freqlist_view.get_index()].frequency_a);
        new_view->on_changed = [this](rf::Frequency f) {
            on_edit_freq(f);
        };
    };

    button_edit_desc.on_select = [this, &nav](Button&) {
        if (database.empty()) {
            database.push_back({0, 0, "", SINGLE});
        }
        desc_buffer = database[freqlist_view.get_index()].description;
        on_edit_desc(nav);
    };

    button_delete.on_select = [this, &nav](Button&) {
        on_delete();
    };
}

}  // namespace ui
