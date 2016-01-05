/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_loadmodule.hpp"

#include "ch.h"

#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"

#include "hackrf_hal.hpp"

#include <cstring>
#include <stdio.h>

using namespace hackrf::one;

namespace ui {

void LoadModuleView::focus() {
	button_ok.focus();
}

void LoadModuleView::paint(Painter& painter) {
	(void)painter;
}

void LoadModuleView::on_hide() {
	auto& message_map = context().message_map();
	message_map.unregister_handler(Message::ID::ReadyForSwitch);
}

void LoadModuleView::on_show() {
	auto& message_map = context().message_map();
	message_map.register_handler(Message::ID::ReadyForSwitch,
		[this](Message* const p) {
			(void)p;
			// const auto message = static_cast<const ReadyForSwitchMessage*>(p);
			if (m4_load_image()) {
				text_info.set("Module loaded :)");
				_mod_loaded = true;
			} else {
				text_info.set("Module not found :(");
				_mod_loaded = false;
			}
		}
	);
	
	m4_switch(_hash);
}

LoadModuleView::LoadModuleView(
	NavigationView& nav,
	const char * hash,
	View* new_view
)
{
	add_children({ {
		&text_info,
		&button_ok
	} });
	
	_hash = hash;
	
	button_ok.on_select = [this,&nav,new_view](Button&){
		nav.pop();
		if (_mod_loaded == true) nav.push(new_view);
	};
}

} /* namespace ui */
