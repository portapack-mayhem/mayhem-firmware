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

void FrequencySaveView::on_save_name(NavigationView& nav) {
	textentry(nav, desc_buffer, 28);
	frequencies.push_back({ value_, "", desc_buffer });
	nav.pop();
}
void FrequencySaveView::on_save_timestamp(NavigationView& nav) {
	frequencies.push_back({ value_, "", text_timestamp.text() });
	nav.pop();
}

void FrequencySaveView::focus() {
	button_save_timestamp.focus();
	
	if (error)
		nav_.display_modal("Error", "File acces error !", ABORT, nullptr);
}

void FrequencySaveView::on_tick_second() {
	rtcGetTime(&RTCD1, &datetime);
	text_timestamp.set(to_string_dec_uint(datetime.month(), 2, '0') + "/" + to_string_dec_uint(datetime.day(), 2, '0') + " " +
						to_string_dec_uint(datetime.hour(), 2, '0') + ":" + to_string_dec_uint(datetime.minute(), 2, '0'));
}

FrequencySaveView::~FrequencySaveView() {
	time::signal_tick_second -= signal_token_tick_second;
	save_freqman_file(frequencies);
}

FrequencySaveView::FrequencySaveView(
	NavigationView& nav,
	const rf::Frequency value
) : nav_ (nav),
	value_ (value)
{
	File freqs_file;
	
	if (!load_freqman_file(frequencies)) {
		if (!create_freqman_file(freqs_file)) error = true;
	}
	
	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
	
	add_children({ {
		&big_display,
		&text_save,
		&button_save_name,
		&button_save_timestamp,
		&text_timestamp,
		&button_cancel
	} });
	
	on_tick_second();
	
	big_display.set(value);
	
	button_save_name.on_select = [this, &nav](Button&) {
		on_save_name(nav);
	};
	button_save_timestamp.on_select = [this, &nav](Button&) {
		on_save_timestamp(nav);
	};
	button_cancel.on_select = [this, &nav](Button&) {
		nav.pop();
	};
}

void FrequencyLoadView::setup_list() {
	size_t n;
	
	menu_view.clear();
	
	for (n = 0; n < frequencies.size(); n++) {
		menu_view.add_item({ freqman_item_string(frequencies[n]), ui::Color::white(), nullptr, [this](){ on_frequency_select(); } });
	}
	
	menu_view.set_parent_rect({ 0, 0, 240, 216 });
	menu_view.set_highlighted(menu_view.highlighted());		// Refresh
}

void FrequencyLoadView::on_frequency_select() {
	nav_.pop();
	if (on_changed) on_changed(frequencies[menu_view.highlighted()].value);
}

void FrequencyLoadView::focus() {
	menu_view.focus();
	
	if (error)
		nav_.display_modal("Error", "File acces error !", ABORT, nullptr);
}

FrequencyLoadView::FrequencyLoadView(
	NavigationView& nav
) : nav_ (nav)
{
	error = !load_freqman_file(frequencies);

	add_children({ {
		&menu_view,
		&button_cancel
	} });
	
	setup_list();
	
	button_cancel.on_select = [this, &nav](Button&) {
		nav.pop();
	};
}

void FreqManView::on_frequency_select() {
	button_edit_freq.focus();
}

void FreqManView::on_edit_freq(rf::Frequency f) {
	frequencies[menu_view.highlighted()].value = f;
	setup_list();
}

void FreqManView::on_edit_desc(NavigationView& nav) {
	char desc_buffer[32] = { 0 };
	
	strcpy(desc_buffer, frequencies[menu_view.highlighted()].description.c_str());
	textentry(nav, desc_buffer, 28, [this, &desc_buffer](char * buffer) {
				frequencies[menu_view.highlighted()].description = buffer;
				setup_list();
			});
}

void FreqManView::on_delete() {
	frequencies.erase(frequencies.begin() + menu_view.highlighted());
	setup_list();
}

void FreqManView::setup_list() {
	size_t n;
	
	menu_view.clear();
	
	for (n = 0; n < frequencies.size(); n++) {
		menu_view.add_item({ freqman_item_string(frequencies[n]), ui::Color::white(), nullptr, [this](){ on_frequency_select(); } });
	}
	
	menu_view.set_parent_rect({ 0, 0, 240, 168 });
	menu_view.set_highlighted(menu_view.highlighted());		// Refresh
}

void FreqManView::focus() {
	menu_view.focus();
	
	if (error)
		nav_.display_modal("Error", "File acces error !", ABORT, nullptr);
}

FreqManView::~FreqManView() {
	save_freqman_file(frequencies);
}

FreqManView::FreqManView(
	NavigationView& nav
) : nav_ (nav)
{
	error = !load_freqman_file(frequencies);
	
	add_children({ {
		&menu_view,
		&button_edit_freq,
		&button_edit_desc,
		&button_del,
		&button_exit
	} });
	
	setup_list();
	
	button_edit_freq.on_select = [this, &nav](Button&) {
		auto new_view = nav.push<FrequencyKeypadView>(frequencies[menu_view.highlighted()].value);
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
