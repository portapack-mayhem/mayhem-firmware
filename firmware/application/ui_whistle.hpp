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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {

class WhistleView : public View {
public:
	WhistleView(NavigationView& nav);
	~WhistleView();
	
	void focus() override;
	void paint(Painter& painter) override;
	static void whistle_th(void *p);

private:
	rf::Frequency f;

	Text text_status {
		{ 172, 196, 64, 16 },
		"Ready"
	};
	
	Checkbox checkbox_am_a {
		{ 16, 68 },
		20,
		""
	};
	
	Button button_transmit {
		{ 24, 270, 48, 32 },
		"TX"
	};
	
	static Button button_scan;
	
	Button button_exit {
		{ 176, 270, 48, 32 },
		"Exit"
	};
};

} /* namespace ui */
