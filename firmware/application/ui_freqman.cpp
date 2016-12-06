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

#include "ch.h"
#include "ff.h"
#include "portapack.hpp"
#include "event_m0.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void FrequencySaveView::focus() {
	button_exit.focus();
}

FrequencySaveView::FrequencySaveView(
	NavigationView& nav,
	const rf::Frequency value
) {

	add_children({ {
		&button_exit
	} });
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
}

void FreqManView::focus() {
	button_exit.focus();
}

FreqManView::FreqManView(
	NavigationView& nav
) {

	add_children({ {
		&button_exit
	} });

	size_t n = 0;
	for(auto& text : text_list) {
		add_child(&text);
		text.set_parent_rect({
			static_cast<Coord>(0),
			static_cast<Coord>(16 + (n * 16)),
			240, 16
		});
		const std::string label {
			(char)(n + 0x30)
		};
		text.set(label);
		n++;
	}
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};

}

}
