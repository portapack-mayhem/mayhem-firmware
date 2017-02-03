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
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_textentry.hpp"
#include "rtc_time.hpp"

namespace ui {
	
enum script_keyword {
	STOP = 0,
	WAIT_N,
	WAIT_RTC,
	IF,
	LOOP,
	END,
	TX,
	RX
};

struct script_line {
	script_keyword keyword;
};

class ScriptView : public View {
public:
	ScriptView(NavigationView& nav);
	
	void focus() override;

	std::string title() const override { return "Script editor"; };

private:
	void on_frequency_select();
	void on_edit_freq(rf::Frequency f);
	void on_edit_desc(NavigationView& nav);
	void on_delete();
	void setup_list();
	
	std::vector<script_line> script { };

	MenuView menu_view {
		{ 0, 0, 240, 168 },
		true
	};

	Text text_edit {
		{ 16, 194, 5 * 8, 16 },
		"Edit:"
	};
	Button button_edit_freq {
		{ 16, 194 + 16, 88, 32 },
		"Frequency"
	};
	Button button_edit_desc {
		{ 16, 194 + 16 + 34, 88, 32 },
		"Description"
	};
	Button button_del {
		{ 160, 192, 72, 64 },
		"Delete"
	};
	
	Button button_exit {
		{ 160, 264, 72, 32 },
		"Exit"
	};
};

} /* namespace ui */
