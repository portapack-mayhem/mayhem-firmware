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
#include "ui_font_fixed_8x16.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "rf_path.hpp"
#include "max2837.hpp"
#include "volume.hpp"
#include "receiver_model.hpp"

namespace ui {

class SIGFRXView : public View {
public:
	SIGFRXView(NavigationView& nav);
	~SIGFRXView();
	void on_channel_spectrum(const ChannelSpectrum& spectrum);
	
	void on_show() override;
	void on_hide() override;
	void focus() override;
	void paint(Painter& painter) override;

private:	
	uint8_t last_channel;
	uint8_t detect_counter = 0;
	
	const Style style_white {
		.font = font::fixed_8x16,
		.background = Color::white(),
		.foreground = Color::black()
	};
	
	const uint16_t sigfrx_marks[18] = {
		10,		8, 		0,
		60,		52,		90, 
		119, 	95,		180, 
		121, 	122,	220, 
		179, 	171,	310, 
		230, 	214,	400 };
	
	Text text_type {
		{ 1 * 8, 1 * 16, 28 * 8, 16 },
		"SIGFOX interceptor. Yap !"
	};
	
	Text text_channel {
		{ 1 * 8, 3 * 16, 28 * 8, 16 },
		"PL: "
	};
	Text text_data {
		{ 1 * 8, 4 * 16, 28 * 8, 16 },
		"??: "
	};
	
	Button button_exit {
		{ 22 * 8, 160 - 32, 56, 32 },
		"Exit"
	};
};

} /* namespace ui */
