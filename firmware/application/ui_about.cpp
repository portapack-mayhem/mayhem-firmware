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

#include "cpld_update.hpp"
#include "portapack.hpp"
#include "event_m0.hpp"

#include "ui_about.hpp"

#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"

#include <math.h>
#include <cstring>

using namespace lpc43xx;
using namespace portapack;

namespace ui {

void AboutView::update() {
	size_t c;
	int32_t n;
	std::string text;
	Coord y_val, x_pos = 0;
	uint32_t flag;
	
	if (scroll & 1) {
		if (!((scroll >> 1) & 15)) {
			if (line_feed) {
				line_feed = false;
			} else {
				// Find a free text widget
				for (c = 0; c < 10; c++)
					if (text_line[c].screen_pos().y() >= 200) break;
				
				if (c < 10) {
					flag = credits[credits_index].flag & 0x3F;
					line_feed = (credits[credits_index].flag & 0x40) ? true : false;
					
					if (flag == SECTION) {
						if (!second) {
							text = credits[credits_index].role;
							if (credits[credits_index].name != "")
								second = true;
							else {
								credits_index++;
								line_feed = true;
							}
						} else {
							text = credits[credits_index].name;
							second = false;
							line_feed = true;
							credits_index++;
						}
						x_pos = (240 - (text.size() * 8)) / 2;
					} else if (flag == TITLE) {
						text = credits[credits_index].role;
						x_pos = 120 - (text.size() * 8);
						if (credits[credits_index].name != "")
							text += "  " + credits[credits_index].name;
						credits_index++;
					} else if (flag == MEMBER) {
						if (!second) {
							text = credits[credits_index].role;
							if (credits[credits_index].name != "")
								second = true;
							else
								credits_index++;
						} else {
							text = credits[credits_index].name;
							second = false;
							credits_index++;
						}
						x_pos = 136;
					}
					
					if (!(flag & 0x80)) {
						text_line[c].set_parent_rect({{ x_pos, 200 - 16 }, { (Dim)text.size() * 8, 17 }});
						text_line[c].set(text);
						text_line[c].hidden(false);
					}
				}
			}
		}

		// Scroll text lines
		for (c = 0; c < 10; c++) {
			y_val = text_line[c].screen_pos().y() - 16;
			if (y_val < 32) {
				text_line[c].set_parent_rect({{ text_line[c].screen_pos().x(), 200 }, { text_line[c].size() }});
				text_line[c].hidden(true);
			} else {
				if (y_val < 200) {
					text_line[c].set_parent_rect({{ text_line[c].screen_pos().x(), y_val - 1 }, { text_line[c].size() }});
					n = (y_val - 32) >> 2;
					if (n > 19)
						n = (38 - n);
					else
						n -= 3;
					if (n > 3) n = 3;
					if (n < 0) n = 0;
					text_line[c].set_style(&styles[n]);
				}
			}
		}
	}
	scroll++;
}

AboutView::AboutView(
	NavigationView& nav
)
{
	add_children({
		&text_cpld_hackrf,
		&text_cpld_hackrf_status,
		&button_ok,
	});
	
	for (auto& text : text_line) {
		text.set("");
		text.set_parent_rect({
			static_cast<Coord>(0),
			static_cast<Coord>(200),
			0, 0
		});
		add_child(&text);
	}

	if( hackrf::cpld::verify_eeprom() ) {
		text_cpld_hackrf_status.set(" OK");
	} else {
		text_cpld_hackrf_status.set("BAD");
	}

	button_ok.on_select = [&nav](Button&){
		nav.pop();
	};
}

void AboutView::focus() {
	button_ok.focus();
}

} /* namespace ui */
