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

#include <cstring>

using namespace portapack;

namespace ui {

void FrequencySaveView::on_save_name(char * name) {
	database.entries.push_back({ value_, "", name, (int32_t)options_category.selected_index_value() });
	nav_.pop();
}

void FrequencySaveView::on_save_timestamp() {
	database.entries.push_back({ value_, "", str_timestamp, (int32_t)options_category.selected_index_value() });
	nav_.pop();
}

void FrequencySaveView::focus() {
	if (error == ERROR_ACCESS) {
		nav_.display_modal("Error", "File acces error", ABORT, nullptr);
	} else if (error == ERROR_DUPLICATE) {
		nav_.display_modal("Error", "Frequency already saved", INFO, nullptr);
		error = NO_ERROR;
	} else {
		button_save_timestamp.focus();
	}
}

void FrequencySaveView::on_tick_second() {
	rtcGetTime(&RTCD1, &datetime);
	str_timestamp = to_string_dec_uint(datetime.month(), 2, '0') + "/" + to_string_dec_uint(datetime.day(), 2, '0') + " " +
						to_string_dec_uint(datetime.hour(), 2, '0') + ":" + to_string_dec_uint(datetime.minute(), 2, '0');
	text_timestamp.set(str_timestamp);
}

FrequencySaveView::~FrequencySaveView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
	save_freqman_file(database);
}

FrequencySaveView::FrequencySaveView(
	NavigationView& nav,
	const rf::Frequency value
) : nav_ (nav),
	value_ (value)
{
	using name_t = std::string;
	using value_t = int32_t;
	using option_t = std::pair<name_t, value_t>;
	using options_t = std::vector<option_t>;
	options_t categories;
	File freqs_file;
	size_t n;
	
	if (!load_freqman_file(database)) {
		if (!create_freqman_file(freqs_file)) {
			error = ERROR_ACCESS;
			return;
		}
	}
	
	for (n = 0; n < database.entries.size(); n++) {
		if (database.entries[n].value == value_) {
			error = ERROR_DUPLICATE;
			break;
		}
	}
	
	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	
	add_children({
		&big_display,
		&text_save,
		&button_save_name,
		&button_save_timestamp,
		&text_timestamp,
		&text_category,
		&options_category,
		&button_cancel
	});

	// Populate categories OptionsField
	categories.emplace_back(std::make_pair("No cat.", -1));
	for (n = 0; n < database.categories.size(); n++)
		categories.emplace_back(std::make_pair(database.categories[n], n));
	options_category.set_options(categories);
	options_category.set_selected_index(0);
	
	on_tick_second();
	
	big_display.set(value);
	
	button_save_name.on_select = [this, &nav](Button&) {
		textentry(nav, desc_buffer, 28, [this](char * buffer) {
				on_save_name(buffer);
			});
	};
	button_save_timestamp.on_select = [this, &nav](Button&) {
		on_save_timestamp();
	};
	
	button_cancel.on_select = [this, &nav](Button&) {
		nav.pop();
	};
}

void FrequencyLoadView::setup_list() {
	size_t n;
	
	menu_view.clear();
	
	for (n = 0; n < database.entries.size(); n++) {
		menu_view.add_item({
			freqman_item_string(database.entries[n]),
			ui::Color::white(),
			nullptr,
			[this](){ 
				on_frequency_select();
			}
		});
	}
	
	menu_view.set_highlighted(menu_view.highlighted());		// Refresh
}

void FrequencyLoadView::on_frequency_select() {
	nav_.pop();
	if (on_changed) on_changed(database.entries[menu_view.highlighted()].value);
}

void FrequencyLoadView::focus() {
	if (error == ERROR_ACCESS)
		nav_.display_modal("Error", "File acces error", ABORT, nullptr);
	else if (error == ERROR_EMPTY)
		nav_.display_modal("Error", "Frequency DB empty", ABORT, nullptr);
	else
		menu_view.focus();
}

FrequencyLoadView::FrequencyLoadView(
	NavigationView& nav
) : nav_ (nav)
{
	if (!load_freqman_file(database)) {
		error = ERROR_ACCESS;
		return;
	}
	
	if (database.entries.size() == 0) {
		error = ERROR_EMPTY;
		return;
	}

	add_children({
		&menu_view,
		&button_cancel
	});
	
	setup_list();
	
	// Just to allow exit on left
	menu_view.on_left = [this]() {
		on_frequency_select();
	};
	
	button_cancel.on_select = [this, &nav](Button&) {
		nav.pop();
	};
}

void FreqManView::on_frequency_select() {
	options_category.set_selected_index(database.entries[menu_view.highlighted()].category_id + 1);
	button_edit_freq.focus();
}

void FreqManView::on_edit_freq(rf::Frequency f) {
	database.entries[menu_view.highlighted()].value = f;
	//setup_list();
}

void FreqManView::on_edit_desc(NavigationView& nav) {
	char desc_buffer[32] = { 0 };
	
	strcpy(desc_buffer, database.entries[menu_view.highlighted()].description.c_str());
	textentry(nav, desc_buffer, 28, [this, &desc_buffer](char * buffer) {
				database.entries[menu_view.highlighted()].description = buffer;
				//setup_list();
			});
}

void FreqManView::on_delete() {
	database.entries.erase(database.entries.begin() + menu_view.highlighted());
	//setup_list();
}

void FreqManView::on_edit_category(int32_t category_id) {
	database.entries[menu_view.highlighted()].category_id = category_id;
}

void FreqManView::setup_list() {
	size_t n;
	
	menu_view.clear();
	
	for (n = 0; n < database.entries.size(); n++) {
		menu_view.add_item({
			freqman_item_string(database.entries[n]),
			ui::Color::white(),
			nullptr,
			[this](){
				on_frequency_select();
			}
		});
	}
	
	menu_view.set_highlighted(0);		// Refresh
}

void FreqManView::focus() {
	if (error == ERROR_ACCESS)
		nav_.display_modal("Error", "File acces error", ABORT, nullptr);
	else if (error == ERROR_EMPTY)
		nav_.display_modal("Error", "Frequency DB empty", ABORT, nullptr);
	else
		menu_view.focus();
}

FreqManView::~FreqManView() {
	save_freqman_file(database);
}

FreqManView::FreqManView(
	NavigationView& nav
) : nav_ (nav)
{
	using name_t = std::string;
	using value_t = int32_t;
	using option_t = std::pair<name_t, value_t>;
	using options_t = std::vector<option_t>;
	options_t categories;
	size_t n;
	
	if (!load_freqman_file(database)) {
		error = ERROR_ACCESS;
		return;
	}
	
	if (database.entries.size() == 0) {
		error = ERROR_EMPTY;
		return;
	}
	
	add_children({
		&menu_view,
		&text_edit,
		&button_edit_freq,
		&button_edit_desc,
		&text_category,
		&options_category,
		&button_del,
		&button_exit
	});
	
	setup_list();
	
	// Populate categories OptionsField
	categories.emplace_back(std::make_pair("No cat.", -1));
	for (n = 0; n < database.categories.size(); n++)
		categories.emplace_back(std::make_pair(database.categories[n], n));
	options_category.set_options(categories);
	options_category.set_selected_index(0);
	
	options_category.on_change = [this](size_t, int32_t category_id) {
		on_edit_category(category_id);
	};
	
	// Just to allow exit on left
	menu_view.on_left = [this]() {
		on_frequency_select();
	};
	
	button_edit_freq.on_select = [this, &nav](Button&) {
		auto new_view = nav.push<FrequencyKeypadView>(database.entries[menu_view.highlighted()].value);
		new_view->on_changed = [this](rf::Frequency f) {
			on_edit_freq(f);
		};
	};
	
	button_edit_desc.on_select = [this, &nav](Button&) {
		on_edit_desc(nav);
	};
	
	button_del.on_select = [this, &nav](Button&) {
		nav.push<ModalMessageView>("Confirm", "Are you sure ?", YESNO,
			[this](bool choice) {
				if (choice) {
					on_delete();
				}
			}
		);
	};
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
}

}
