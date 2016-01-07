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

#ifndef __UI_ABOUT_H__
#define __UI_ABOUT_H__

#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include <cstdint>

namespace ui {

class AboutView : public View {
public:
	AboutView(NavigationView& nav);
	void focus() override;

private:
	/*bool drawn;
	uint16_t phase = 0;
	uint16_t copper_buffer[256];
	void update(void);*/

	Text text_title {
		{ 100, 96, 40, 16 },
		"About",
	};

	Text text_firmware {
		{ 0, 128, 240, 16 },
		"Git Commit hash        " GIT_REVISION,
	};

	Text text_cpld_hackrf {
		{ 0, 144, 240, 16 },
		"HackRF CPLD CRC     0x????????",
	};

	Text text_cpld_portapack {
		{ 0, 160, 240, 16 },
		"PortaPack CPLD CRC  0x????????",
	};

	Button button_ok {
		{ 72, 192, 96, 24 },
		"OK"
	};
};

} /* namespace ui */

#endif/*__UI_ABOUT_H__*/
