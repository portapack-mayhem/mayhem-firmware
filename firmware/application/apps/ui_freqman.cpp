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

static int32_t last_category_id { 0 };

FreqManBaseView::FreqManBaseView(
	NavigationView& nav
) : nav_ (nav)
{
	file_list = get_freqman_files();
	
	add_children({
		&label_category,
		&button_exit
	});
	
	if (file_list.size()) {
		add_child(&options_category);
		populate_categories();
	} else
		error_ = ERROR_NOFILES;
	
	// Default function
	on_change_category = [this](int32_t category_id) {
		change_category(category_id);
	};
	
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

void FreqManBaseView::populate_categories() {
	categories.clear();
	
	for (size_t n = 0; n < file_list.size(); n++)
		categories.emplace_back(std::make_pair(file_list[n].substr(0, 14), n));
	
	// Alphabetical sort
	std::sort(categories.begin(), categories.end(), [](auto &left, auto &right) {
		return left.first < right.first;
	});
	
	options_category.set_options(categories);
	options_category.set_selected_index(last_category_id);
	
	options_category.on_change = [this](size_t category_id, int32_t) {
		if (on_change_category)
			on_change_category(category_id);
	};
}

void FreqManBaseView::change_category(int32_t category_id) {
	
	if (!file_list.size()) return;
	
	last_category_id = current_category_id = category_id;
	
	if (!load_freqman_file(file_list[categories[current_category_id].second], database))
		error_ = ERROR_ACCESS;
	else
		refresh_list();
}

void FreqManBaseView::refresh_list() {
	if (!database.size()) {
		if (on_refresh_widgets)
			on_refresh_widgets(true);
	} else {
		if (on_refresh_widgets)
			on_refresh_widgets(false);
	
		menu_view.clear();
		
		for (size_t n = 0; n < database.size(); n++) {
			menu_view.add_item({
				freqman_item_string(database[n], 30),
				ui::Color::white(),
				nullptr,
				[this](){
					if (on_select_frequency)
						on_select_frequency();
				}
			});
		}
	
		menu_view.set_highlighted(0);	// Refresh
	}
}

void FrequencySaveView::save_current_file() {
	if (database.size() > FREQMAN_MAX_PER_FILE) {
		nav_.display_modal(
			"Error", "Too many entries, maximum is\n" FREQMAN_MAX_PER_FILE_STR ". Trim list ?",
			YESNO,
			[this](bool choice) {
				if (choice) {
					database.resize(FREQMAN_MAX_PER_FILE);
					save_freqman_file(file_list[categories[current_category_id].second], database);
				}
				nav_.pop();
			}
		);
	} else {
		save_freqman_file(file_list[categories[current_category_id].second], database);
		nav_.pop();
	}
}

void FrequencySaveView::on_save_name() {
	text_prompt(nav_, desc_buffer, 28, [this](std::string& buffer) {
		database.push_back({ value_, 0, buffer, SINGLE });
		save_current_file();
	});
}

void FrequencySaveView::on_save_timestamp() {
	database.push_back({ value_, 0, live_timestamp.string(), SINGLE });
	save_current_file();
}

FrequencySaveView::FrequencySaveView(
	NavigationView& nav,
	const rf::Frequency value
) : FreqManBaseView(nav),
	value_ (value)
{
	desc_buffer.reserve(28);
	
	// Todo: add back ?
	/*for (size_t n = 0; n < database.size(); n++) {
		if (database[n].value == value_) {
			error_ = ERROR_DUPLICATE;
			break;
		}
	}*/
	
	add_children({
		&labels,
		&big_display,
		&button_save_name,
		&button_save_timestamp,
		&live_timestamp
	});
	
	big_display.set(value);
	
	button_save_name.on_select = [this, &nav](Button&) {
		on_save_name();
	};
	button_save_timestamp.on_select = [this, &nav](Button&) {
		on_save_timestamp();
	};
}

void FrequencyLoadView::refresh_widgets(const bool v) {
	menu_view.hidden(v);
	text_empty.hidden(!v);
	//display.fill_rectangle(menu_view.screen_rect(), Color::black());
	set_dirty();
}

FrequencyLoadView::FrequencyLoadView(
	NavigationView& nav
) : FreqManBaseView(nav)
{
	on_refresh_widgets = [this](bool v) {
		refresh_widgets(v);
	};
	
	add_children({
		&menu_view,
		&text_empty
	});
	
	// Resize menu view to fill screen
	menu_view.set_parent_rect({ 0, 3 * 8, 240, 30 * 8 });
	
	// Just to allow exit on left
	menu_view.on_left = [&nav, this]() {
		nav.pop();
	};
	
	change_category(last_category_id);
	refresh_list();
	
	on_select_frequency = [&nav, this]() {
		nav_.pop();
		
		auto& entry = database[menu_view.highlighted_index()];
		
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
	};
}

void FrequencyManagerView::on_edit_freq(rf::Frequency f) {
	database[menu_view.highlighted_index()].frequency_a = f;
	save_freqman_file(file_list[categories[current_category_id].second], database);
	refresh_list();
}

void FrequencyManagerView::on_edit_desc(NavigationView& nav) {
	text_prompt(nav, desc_buffer, 28, [this](std::string& buffer) {
		database[menu_view.highlighted_index()].description = buffer;
		refresh_list();
		save_freqman_file(file_list[categories[current_category_id].second], database);
	});
}

void FrequencyManagerView::on_new_category(NavigationView& nav) {
	text_prompt(nav, desc_buffer, 12, [this](std::string& buffer) {
		File freqman_file;
		create_freqman_file(buffer, freqman_file);
	});
	populate_categories();
	refresh_list();
}

void FrequencyManagerView::on_delete() {
	database.erase(database.begin() + menu_view.highlighted_index());
	save_freqman_file(file_list[categories[current_category_id].second], database);
	refresh_list();
}

void FrequencyManagerView::refresh_widgets(const bool v) {
	button_edit_freq.hidden(v);
	button_edit_desc.hidden(v);
	button_delete.hidden(v);
	menu_view.hidden(v);
	text_empty.hidden(!v);
	//display.fill_rectangle(menu_view.screen_rect(), Color::black());
	set_dirty();
}

FrequencyManagerView::~FrequencyManagerView() {
	//save_freqman_file(file_list[categories[current_category_id].second], database);
}

FrequencyManagerView::FrequencyManagerView(
	NavigationView& nav
) : FreqManBaseView(nav)
{
	on_refresh_widgets = [this](bool v) {
		refresh_widgets(v);
	};
	
	add_children({
		&labels,
		&button_new_category,
		&menu_view,
		&text_empty,
		&button_edit_freq,
		&button_edit_desc,
		&button_delete
	});
	
	// Just to allow exit on left
	menu_view.on_left = [&nav, this]() {
		nav.pop();
	};
	
	change_category(last_category_id);
	refresh_list();
	
	on_select_frequency = [this]() {
		button_edit_freq.focus();
	};
	
	button_new_category.on_select = [this, &nav](Button&) {
		desc_buffer = "";
		on_new_category(nav);
	};
	
	button_edit_freq.on_select = [this, &nav](Button&) {
		auto new_view = nav.push<FrequencyKeypadView>(database[menu_view.highlighted_index()].frequency_a);
		new_view->on_changed = [this](rf::Frequency f) {
			on_edit_freq(f);
		};
	};
	
	button_edit_desc.on_select = [this, &nav](Button&) {
		desc_buffer = database[menu_view.highlighted_index()].description;
		on_edit_desc(nav);
	};
	
	button_delete.on_select = [this, &nav](Button&) {
		nav.push<ModalMessageView>("Confirm", "Are you sure ?", YESNO,
			[this](bool choice) {
				if (choice)
					on_delete();
			}
		);
	};
}

}
