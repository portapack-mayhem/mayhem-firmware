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

#include "ui_playdead.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"

using namespace portapack;

namespace ui {
	
void PlayDeadView::focus() {
	button_seq_entry.focus();
}

void PlayDeadView::paint(Painter& painter) {
	if (persistent_memory::config_login()) {
		// Blank the whole display
		painter.fill_rectangle(
			display.screen_rect(),
			style().background
		);
	}
}

PlayDeadView::PlayDeadView(NavigationView& nav) {
	rtc::RTC datetime;
	
	persistent_memory::set_playing_dead(0x5920C1DF);		// Enable
	
	add_children({
		&text_playdead1,
		&text_playdead2,
		&text_playdead3,
		&button_seq_entry,
	});
	
	// Seed from RTC
	rtcGetTime(&RTCD1, &datetime);
	text_playdead2.set("0x" + to_string_hex(lfsr_iterate(datetime.second()), 6) + "00");
	
	text_playdead3.hidden(true);
	
	button_seq_entry.on_dir = [this](Button&, KeyEvent key){
		sequence = (sequence << 3) | (static_cast<std::underlying_type<KeyEvent>::type>(key) + 1);
		return true;
	};
	
	button_seq_entry.on_select = [this, &nav](Button&){
		if (sequence == persistent_memory::playdead_sequence()) {
			persistent_memory::set_playing_dead(0x82175E23);		// Disable
			if (persistent_memory::config_login()) {
				text_playdead3.hidden(false);
			} else {
				nav.pop();
				nav.push<SystemMenuView>();
			}
		} else {
			sequence = 0;
		}
	};
}

} /* namespace ui */
