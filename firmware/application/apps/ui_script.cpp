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

#include "ui_script.hpp"

#include "portapack.hpp"
#include "event_m0.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void ScriptView::on_frequency_select() {
	//button_edit_freq.focus();
}

void ScriptView::on_edit_freq(rf::Frequency f) {
	(void)f;
	//frequencies[menu_view.highlighted()].value = f;
	setup_list();
}

void ScriptView::on_edit_desc(NavigationView& nav) {
	(void)nav;
}

void ScriptView::on_delete() {
	//frequencies.erase(frequencies.begin() + menu_view.highlighted());
	setup_list();
}

void ScriptView::setup_list() {
	//size_t n;
	
	menu_view.clear();
	
	/*for (n = 0; n < frequencies.size(); n++) {
		menu_view.add_item({ freqman_item_string(frequencies[n]), ui::Color::white(), nullptr, [this](){ on_frequency_select(); } });
	}*/
	
	menu_view.set_parent_rect({ 0, 0, 240, 168 });
	menu_view.set_highlighted(menu_view.highlighted());		// Refresh
}

void ScriptView::focus() {
	menu_view.focus();
}

ScriptView::ScriptView(
	NavigationView& nav
) {
	
	add_children({
		&menu_view,
		&text_edit,
		&button_edit_freq,
		&button_edit_desc,
		&button_del,
		&button_exit
	});
	
	setup_list();
	
	button_edit_freq.on_select = [this, &nav](Button&) {
		/*auto new_view = nav.push<FrequencyKeypadView>(frequencies[menu_view.highlighted()].value);
		new_view->on_changed = [this](rf::Frequency f) {
			on_edit_freq(f);
		};*/
	};
	
	button_edit_desc.on_select = [this, &nav](Button&) {
		on_edit_desc(nav);
	};
	
	button_del.on_select = [this, &nav](Button&) {
		nav.push<ModalMessageView>("Confirm", "Are you sure?", YESNO,
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
