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
#include "tone_key.hpp"
#include "ui_receiver.hpp"
#include "ui_styles.hpp"
#include "utility.hpp"

#include <memory>

using namespace portapack;
using namespace ui;
namespace fs = std::filesystem;

// TODO: Clean up after moving to better lookup tables.
using options_t = OptionsField::options_t;
extern options_t freqman_modulations;
extern options_t freqman_bandwidths[4];
extern options_t freqman_steps;
extern options_t freqman_steps_short;

/* Set options. */
void freqman_set_modulation_option(OptionsField& option) {
    option.set_options(freqman_modulations);
}

void freqman_set_bandwidth_option(freqman_index_t modulation, OptionsField& option) {
    if (is_valid(modulation))
        option.set_options(freqman_bandwidths[modulation]);
}

void freqman_set_step_option(OptionsField& option) {
    option.set_options(freqman_steps);
}

void freqman_set_step_option_short(OptionsField& option) {
    option.set_options(freqman_steps_short);
}

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
        db_.insert_entry(db_.entry_count(), entry_);
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

void FrequencyManagerView::on_edit_entry() {
    auto edit_view = nav_.push<FrequencyEditView>(current_entry());
    edit_view->on_save = [this](const freqman_entry& entry) {
        db_.replace_entry(current_index(), entry);
        freqlist_view.set_dirty();
    };
}

void FrequencyManagerView::on_edit_freq() {
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
    db_.insert_entry(current_index() + 1, entry);
    refresh_list(1);
}

void FrequencyManagerView::on_del_entry() {
    if (db_.empty())
        return;

    nav_.push<ModalMessageView>(
        "Delete", "Delete " + trim(pretty_string(current_entry(), 23)) + "\nAre you sure?", YESNO,
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
         &button_add_category,
         &button_del_category,
         &button_edit_entry,
         &rect_padding,
         &button_edit_freq,
         &button_edit_desc,
         &button_add_entry,
         &button_del_entry});

    freqlist_view.on_select = [this](size_t) {
        button_edit_entry.focus();
    };
    // Allows for quickly exiting control.
    freqlist_view.on_leave = [this]() {
        button_edit_entry.focus();
    };

    button_add_category.on_select = [this]() {
        on_add_category();
    };

    button_del_category.on_select = [this]() {
        on_del_category();
    };

    button_edit_entry.on_select = [this](Button&) {
        on_edit_entry();
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

/* FrequencyEditView *************************************/

FrequencyEditView::FrequencyEditView(
    NavigationView& nav,
    freqman_entry entry)
    : nav_{nav},
      entry_{std::move(entry)} {
    add_children({&labels,
                  &field_type,
                  &field_freq_a,
                  &field_freq_b,
                  &field_modulation,
                  &field_bandwidth,
                  &field_step,
                  &field_tone,
                  &field_description,
                  &text_validation,
                  &button_save,
                  &button_cancel});

    freqman_set_modulation_option(field_modulation);
    populate_bandwidth_options();
    populate_step_options();
    populate_tone_options();

    // Add the "invalid/unset" option. Kind of hacky, but...
    field_modulation.options().insert(
        field_modulation.options().begin(), {"None", -1});
    field_step.options().insert(
        field_step.options().begin(), {"None", -1});

    field_type.set_by_value((int32_t)entry_.type);
    field_type.on_change = [this](size_t, auto value) {
        entry_.type = static_cast<freqman_type>(value);
        refresh_ui();
    };

    // TODO: this pattern should be able to be wrapped up.
    field_freq_a.set_value(entry_.frequency_a);
    field_freq_a.on_change = [this](rf::Frequency f) {
        entry_.frequency_a = f;
        refresh_ui();
    };
    field_freq_a.on_edit = [this]() {
        auto freq_view = nav_.push<FrequencyKeypadView>(field_freq_a.value());
        freq_view->on_changed = [this](rf::Frequency f) {
            field_freq_a.set_value(f);
        };
    };

    field_freq_b.set_value(entry_.frequency_b);
    field_freq_b.on_change = [this](rf::Frequency f) {
        entry_.frequency_b = f;
        refresh_ui();
    };
    field_freq_b.on_edit = [this]() {
        auto freq_view = nav_.push<FrequencyKeypadView>(field_freq_b.value());
        freq_view->on_changed = [this](rf::Frequency f) {
            field_freq_b.set_value(f);
        };
    };

    field_modulation.set_by_value((int32_t)entry_.modulation);
    field_modulation.on_change = [this](size_t, auto value) {
        entry_.modulation = static_cast<freqman_index_t>(value);
        populate_bandwidth_options();
    };

    field_bandwidth.set_by_value((int32_t)entry_.bandwidth);
    field_bandwidth.on_change = [this](size_t, auto value) {
        entry_.bandwidth = static_cast<freqman_index_t>(value);
    };

    field_step.set_by_value((int32_t)entry_.step);
    field_step.on_change = [this](size_t, auto value) {
        entry_.step = static_cast<freqman_index_t>(value);
    };

    field_tone.set_by_value((int32_t)entry_.tone);
    field_tone.on_change = [this](size_t, auto value) {
        entry_.tone = static_cast<freqman_index_t>(value);
    };

    field_description.set_text(entry_.description);
    field_description.on_change = [this](TextField& tf) {
        entry_.description = tf.get_text();
    };
    field_description.on_select = [this](TextField& tf) {
        temp_buffer_ = tf.get_text();
        text_prompt(nav_, temp_buffer_, FreqManBaseView::desc_edit_max,
                    [this, &tf](std::string& new_desc) {
                        tf.set_text(new_desc);
                    });
    };

    button_save.on_select = [this](Button&) {
        if (on_save)
            on_save(std::move(entry_));
        nav_.pop();
    };

    button_cancel.on_select = [this](Button&) {
        nav_.pop();
    };

    refresh_ui();
}

void FrequencyEditView::focus() {
    button_cancel.focus();
}

void FrequencyEditView::refresh_ui() {
    // This needs to be called whenever a UI option is changed
    // in a way that causes fields to be unused or would make the
    // entry fail validation.

    auto is_range = entry_.type == freqman_type::Range;
    auto is_ham = entry_.type == freqman_type::HamRadio;
    auto has_freq_b = is_range || is_ham;

    field_freq_b.set_style(has_freq_b ? &Styles::white : &Styles::grey);
    field_step.set_style(is_range ? &Styles::white : &Styles::grey);
    field_tone.set_style(is_ham ? &Styles::white : &Styles::grey);

    if (is_valid(entry_)) {
        text_validation.set("Valid");
        text_validation.set_style(&Styles::green);
    } else {
        text_validation.set("Error");
        text_validation.set_style(&Styles::red);
    }
}

// TODO: The following functions shouldn't be needed once
// freqman_db lookup tables are complete.
void FrequencyEditView::populate_bandwidth_options() {
    OptionsField::options_t options;
    options.push_back({"None", -1});

    if (entry_.modulation < std::size(freqman_bandwidths)) {
        auto& bandwidths = freqman_bandwidths[entry_.modulation];
        for (auto i = 0u; i < bandwidths.size(); ++i) {
            auto& item = bandwidths[i];
            options.push_back({item.first, (OptionsField::value_t)i});
        }
    }

    field_bandwidth.set_options(std::move(options));
}

void FrequencyEditView::populate_step_options() {
    OptionsField::options_t options;
    options.push_back({"None", -1});

    for (auto i = 0u; i < freqman_steps.size(); ++i) {
        auto& item = freqman_steps[i];
        options.push_back({item.first, (OptionsField::value_t)i});
    }

    field_step.set_options(std::move(options));
}

void FrequencyEditView::populate_tone_options() {
    using namespace tonekey;
    OptionsField::options_t options;
    options.push_back({"None", -1});

    for (auto i = 0u; i < tone_keys.size(); ++i) {
        auto& item = tone_keys[i];
        options.push_back({fx100_string(item.second), (OptionsField::value_t)i});
    }

    field_tone.set_options(std::move(options));
}

}  // namespace ui
