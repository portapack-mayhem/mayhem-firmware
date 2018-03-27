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

#include "receiver_model.hpp"

#include "spectrum_color_lut.hpp"

#include "ui_receiver.hpp"
#include "ui_font_fixed_8x16.hpp"

namespace ui {

class ScannerView : public View {
public:
	ScannerView(NavigationView& nav);
	~ScannerView();
	
	void focus() override;
	
	std::string title() const override { return "Scanner"; };

private:
	Labels labels {
		{ { 1 * 8, 0 }, "Work in progress...", Color::light_grey() }
	};
	
	Button button {
		{ 60, 64, 120, 32 },
		"Exit"
	};
};

} /* namespace ui */
