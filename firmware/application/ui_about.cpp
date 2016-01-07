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

#include "ui_about.hpp"
#include "touch.hpp"

#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"

#include <math.h>
#include <cstring>

using namespace lpc43xx;
using namespace portapack;

namespace ui {

/*static AboutView::update() {
	uint8_t c;
	uint16_t raster_color;
	
	for (c=0; c<=200; c++) {
		raster_color = copper_buffer[c];
		if (raster_color) painter.fill_rectangle({ { 0, (c+32) }, { 240, 1 } }, { (raster_color >> 4) & 0xF0, raster_color & 0xF0, (raster_color << 4) & 0xF0 });
	}
	
	for (c=0; c<=200; c++)
		copper_buffer[c] = 0;

	for (c=0; c<=200; c++) {
		copper_buffer[c] = (c | (c << 4)) + phase;
	}
	
	phase++;
}*/

AboutView::AboutView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_firmware,
		&text_cpld_hackrf,
		&text_cpld_portapack,
		&button_ok,
	} });

	button_ok.on_select = [this,&nav](Button&){ nav.pop(); };
}

void AboutView::focus() {
	button_ok.focus();
}

} /* namespace ui */
