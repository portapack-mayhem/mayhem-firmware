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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "m4_startup.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

class LoadModuleView : public View {
public:
	LoadModuleView(NavigationView& nav, const char * hash, View* new_view);
	void loadmodule();
	
	void on_show() override;
	void on_hide() override;
	void focus() override;
	void paint(Painter& painter) override;

private:
	const char * _hash;
	bool _mod_loaded = false;
	
	Text text_info {
		{ 8, 64, 224, 16 },
		"Searching module..."
	};
	
	Button button_ok {
		{ 88, 128, 64, 32 },
		"OK"
	};
};

} /* namespace ui */
