/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "modems.hpp"
#include "serializer.hpp"

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"

namespace ui {

class ModemSetupView : public View {
public:
	ModemSetupView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "Modem setup"; };

private:
	void update_freq(rf::Frequency f);
	
	Labels labels {
		{ { 2 * 8, 11 * 8 }, "Baudrate:", Color::light_grey() },
		{ { 2 * 8, 13 * 8 }, "Mark:      Hz", Color::light_grey() },
		{ { 2 * 8, 15 * 8 }, "Space:     Hz", Color::light_grey() },
		{ { 140, 15 * 8 }, "Repeat:", Color::light_grey() },
		{ { 1 * 8, 6 * 8 }, "Modem preset:", Color::light_grey() },
		{ { 2 * 8, 22 * 8 }, "Serial format:", Color::light_grey() }
	};

	NumberField field_baudrate {
		{ 11 * 8, 11 * 8 },
		5,
		{ 50, 9600 },
		25,
		' '
	};

	NumberField field_mark {
		{ 8 * 8, 13 * 8 },
		5,
		{ 100, 15000 },
		25,
		' '
	};
	
	NumberField field_space {
		{ 8 * 8, 15 * 8 },
		5,
		{ 100, 15000 },
		25,
		' '
	};
	
	NumberField field_repeat {
		{ 204, 15 * 8 },
		2,
		{ 1, 99 },
		1,
		' '
	};
	
	OptionsField options_modem {
		{ 15 * 8, 6 * 8 },
		7,
		{
		}
	};
	
	SymField sym_format {
		{ 16 * 8, 22 * 8 },
		4,
		SymField::SYMFIELD_DEF
	};
	
	Button button_set_modem {
		{ 23 * 8, 6 * 8 - 4, 6 * 8, 24 },
		"SET"
	};
	
	Button button_save {
		{ 9 * 8, 250, 96, 40 },
		"Save"
	};
};

} /* namespace ui */
