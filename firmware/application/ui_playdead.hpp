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

#ifndef __UI_PLAYDEAD_H__
#define __UI_PLAYDEAD_H__

#include "ui.hpp"
#include "ui_navigation.hpp"

namespace ui {

class PlayDeadView : public View {
public:
	PlayDeadView(NavigationView& nav);
	
	void focus() override;
	void paint(Painter& painter) override;

private:
	uint32_t sequence = 0;
	
	Text text_playdead1 {
		{ 6 * 8, 7 * 16, 14 * 8, 16 },
		"\x46irmwa" "re " "er\x72o\x72"
	};
	Text text_playdead2 {
		{ 6 * 8, 9 * 16, 16 * 8, 16 },
		""
	};
	Text text_playdead3 {
		{ 6 * 8, 12 * 16, 16 * 8, 16 },
		"Please reset"
	};
	
	Button button_seq_entry {
		{ 240, 0, 1, 1 },
		""
	};
};

} /* namespace ui */

#endif/*__UI_PLAYDEAD_H__*/
