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

FreqManBaseView::FreqManBaseView(
	NavigationView& nav,
	Widget& default_focus_widget
) : nav_ (nav),
	default_focus_widget_ (default_focus_widget)
{
	file_list = get_freqman_files();
	
	if (!file_list.size()) {
		error_ = ERROR_NOFILES;
		return;
	}
	
	add_children({
		&label_category,
		&options_category,
		&button_exit
	});
	
	// Populate categories OptionsField
	populate_categories();
	
	on_change_category = [this](int32_t category_id) {
		change_category(category_id);
	};
	//change_category(0);
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
};

void FreqManBaseView::focus() {
	/*if (error_ == ERROR_ACCESS) {
		nav_.display_modal("Error", "File acces error", ABORT, nullptr);
	} else if (error_ == ERROR_DUPLICATE) {
		nav_.display_modal("Error", "Frequency already saved", INFO, nullptr);
		error_ = NO_ERROR;
	}*/
	
	if (error_ == ERROR_NOFILES) {
		nav_.display_modal("Error", "No database files", ABORT, nullptr);
	} else {
		default_focus_widget_.focus();
	}
}

bool FreqManBaseView::populate_categories() {
	size_t n;

	categories.clear();
	
	for (n = 0; n < file_list.size(); n++)
		categories.emplace_back(std::make_pair(file_list[n], n));
	
	options_category.set_options(categories);
	options_category.set_selected_index(0);
	
	options_category.on_change = [this](size_t, int32_t category_id) {
		if (on_change_category)
			on_change_category(category_id);
	};
	
	return true;
}

void FreqManBaseView::change_category(int32_t category_id) {
	current_category_id = category_id;
	
	if (!load_freqman_file(file_list[current_category_id], database))
		error_ = ERROR_ACCESS;	// Todo
	else
		refresh_list();
}

void FreqManBaseView::refresh_list() {
	if (!database.entries.size()) {
		menu_view.hidden(true);
		text_empty.hidden(false);
		display.fill_rectangle(menu_view.screen_rect(), Color::black());
		return;
	} else {
		menu_view.hidden(false);
		text_empty.hidden(true);
	
		menu_view.clear();
		
		for (size_t n = 0; n < database.entries.size(); n++) {
			menu_view.add_item({
				freqman_item_string(database.entries[n], 26),
				ui::Color::light_grey(),
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

void FrequencySaveView::on_save_name() {
	text_prompt(nav_, &desc_buffer, 28, [this](std::string * buffer) {
		database.entries.push_back({ value_, "", *buffer });
		save_freqman_file(file_list[current_category_id], database);
		nav_.pop();
	});
}

void FrequencySaveView::on_save_timestamp() {
	database.entries.push_back({ value_, "", str_timestamp });
	nav_.pop();
	save_freqman_file(file_list[current_category_id], database);
}

void FrequencySaveView::on_tick_second() {
	rtcGetTime(&RTCD1, &datetime);
	str_timestamp = to_string_dec_uint(datetime.month(), 2, '0') + "/" + to_string_dec_uint(datetime.day(), 2, '0') + " " +
						to_string_dec_uint(datetime.hour(), 2, '0') + ":" + to_string_dec_uint(datetime.minute(), 2, '0');
	text_timestamp.set(str_timestamp);
}

FrequencySaveView::~FrequencySaveView() {
	rtc_time::signal_tick_second -= signal_token_tick_second;
}

FrequencySaveView::FrequencySaveView(
	NavigationView& nav,
	const rf::Frequency value
) : FreqManBaseView(nav, options_category),
	value_ (value)
{
	desc_buffer.reserve(28);
	
	// Todo: add back ?
	/*for (size_t n = 0; n < database.entries.size(); n++) {
		if (database.entries[n].value == value_) {
			error_ = ERROR_DUPLICATE;
			break;
		}
	}*/
	
	signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	
	add_children({
		&big_display,
		&text_save,
		&button_save_name,
		&button_save_timestamp,
		&text_timestamp
	});
	
	on_tick_second();
	
	big_display.set(value);
	
	button_save_name.on_select = [this, &nav](Button&) {
		on_save_name();
	};
	button_save_timestamp.on_select = [this, &nav](Button&) {
		on_save_timestamp();
	};
}

FrequencyLoadView::FrequencyLoadView(
	NavigationView& nav
) : FreqManBaseView(nav, options_category)
{
	add_children({
		&menu_view,
		&text_empty
	});
	
	// Just to allow exit on left
	menu_view.on_left = [&nav, this]() {
		nav.pop();
	};
	
	text_empty.hidden(true);
	change_category(0);
	refresh_list();
	
	on_select_frequency = [&nav, this]() {
		nav_.pop();
		if (on_changed)
			on_changed(database.entries[menu_view.highlighted()].value);
	};
}

void FrequencyManagerView::on_edit_freq(rf::Frequency f) {
	database.entries[menu_view.highlighted()].value = f;
	save_freqman_file(file_list[current_category_id], database);
	refresh_list();
}

void FrequencyManagerView::on_edit_desc(NavigationView& nav) {
	text_prompt(nav, &desc_buffer, 28, [this](std::string * buffer) {
		database.entries[menu_view.highlighted()].description = *buffer;
		refresh_list();
		save_freqman_file(file_list[current_category_id], database);
	});
}

void FrequencyManagerView::on_new_category(NavigationView& nav) {
	text_prompt(nav, &desc_buffer, 8, [this](std::string * buffer) {
		File freqman_file;
		create_freqman_file(*buffer, freqman_file);
	});
	populate_categories();
	refresh_list();
}

void FrequencyManagerView::on_delete() {
	database.entries.erase(database.entries.begin() + menu_view.highlighted());
	save_freqman_file(file_list[current_category_id], database);
	refresh_list();
}

FrequencyManagerView::~FrequencyManagerView() {
	//save_freqman_file(file_list[current_category_id], database);
}

FrequencyManagerView::FrequencyManagerView(
	NavigationView& nav
) : FreqManBaseView(nav, options_category)
{
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
	
	text_empty.hidden(true);
	change_category(0);
	refresh_list();
	
	on_select_frequency = [this]() {
		button_edit_freq.focus();
	};
	
	button_new_category.on_select = [this, &nav](Button&) {
		on_new_category(nav);
	};
	
	button_edit_freq.on_select = [this, &nav](Button&) {
		auto new_view = nav.push<FrequencyKeypadView>(database.entries[menu_view.highlighted()].value);
		new_view->on_changed = [this](rf::Frequency f) {
			on_edit_freq(f);
		};
	};
	
	button_edit_desc.on_select = [this, &nav](Button&) {
		desc_buffer = database.entries[menu_view.highlighted()].description;
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
