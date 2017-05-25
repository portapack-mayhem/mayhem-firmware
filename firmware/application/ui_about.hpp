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

#ifndef __UI_ABOUT_H__
#define __UI_ABOUT_H__

#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"

#include <cstdint>

namespace ui {

class AboutView : public View {
public:
	AboutView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "About"; };

private:
	void update();

	uint8_t credits_index = 0;
	uint32_t scroll = 0;
	bool second = false;
	bool line_feed = false;
	
	Style styles[4] = { style_black, style_dark_grey, style_light_grey, style_white };
	
	static constexpr Style style_white {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color(255, 255, 255)
	};
	static constexpr Style style_light_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color(170, 170, 170)
	};
	static constexpr Style style_dark_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color(85, 85, 85)
	};
	static constexpr Style style_black {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color(0, 0, 0)
	};

	enum flags {
		SECTION = 0,
		MEMBER = 1,
		MEMBER_LF = 0x41,
		TITLE = 2,
		TITLE_LF = 0x42,
		END = 0x80
	};
		
	typedef struct credits_t {
		std::string role;
		std::string name;
		flags flag;
	} credits_t;
	
	const credits_t credits[14] = {	{"Portapack|HAVOC", 	"Git hash " GIT_REVISION,	SECTION},
									{"Gurus",				"J. Boone", 				TITLE},
									{"M. Ossmann",			"", 						MEMBER_LF},
									{"Immaturity",			"Furrtek", 					TITLE_LF},
									{"POCSAG rx",			"T. Sailer", 				TITLE},
									{"E. Oenal",			"", 						MEMBER_LF},
									{"RDS waveform",		"C. Jacquet", 				TITLE_LF},
									{"Xy. infos",			"cLx", 						TITLE_LF},
									{"Thanks",				"",							SECTION},
									{"Rainer Matla",		"Keld Norman",				TITLE},
									{"Giorgio C.",			"DC1RDB",					TITLE},
									{"Sigmounte",			"Waax",						TITLE},
									{"Windyoona",			"Channels",					TITLE_LF},
									{"",					"MMXVII",					END}
									};

	std::array<Text, 10> text_line { };

	Text text_cpld_hackrf {
		{ 0, 252, 11*8, 16 },
		"HackRF CPLD",
	};

	Text text_cpld_hackrf_status {
		{ 240 - 3*8, 252, 3*8, 16 },
		"???"
	};

	Button button_ok {
		{ 72, 272, 96, 24 },
		"OK"
	};
	
	MessageHandlerRegistration message_handler_update {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->update();
		}
	};
	
};

} /* namespace ui */

#endif/*__UI_ABOUT_H__*/
