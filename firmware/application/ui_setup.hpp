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

#ifndef __UI_SETUP_H__
#define __UI_SETUP_H__

#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include <cstdint>

namespace ui {

struct SetDateTimeModel {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

class SetDateTimeView : public View {
public:
	std::function<void(SetDateTimeModel)> on_ok;
	std::function<void()> on_cancel;

	SetDateTimeView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 10 * 8, 7 * 16, 9 * 16, 16 },
		"Date/Time"
	};

	NumberField field_year {
		{ 4 * 8, 9 * 16 },
		4,
		{ 2015, 2099 },
		1,
		'0',
	};
	Text text_slash1 {
		{ 8 * 8, 9 * 16, 1 * 8, 16 },
		"/",
	};
	NumberField field_month {
		{ 9 * 8, 9 * 16 },
		2,
		{ 1, 12 },
		1,
		'0',
	};
	Text text_slash2 {
		{ 11 * 8, 9 * 16, 1 * 8, 16 },
		"/",
	};
	NumberField field_day {
		{ 12 * 8, 9 * 16 },
		2,
		{ 1, 31 },
		1,
		'0',
	};

	NumberField field_hour {
		{ 15 * 8, 9 * 16 },
		2,
		{ 0, 23 },
		1,
		'0',
	};
	Text text_colon1 {
		{ 17 * 8, 9 * 16, 1 * 8, 16 },
		":"
	};
	NumberField field_minute {
		{ 18 * 8, 9 * 16 },
		2,
		{ 0, 59 },
		1,
		'0',
	};
	Text text_colon2 {
		{ 20 * 8, 9 * 16, 1 * 8, 16 },
		":",
	};
	NumberField field_second {
		{ 21 * 8, 9 * 16 },
		2,
		{ 0, 59 },
		1,
		'0',
	};

	Text text_format {
		{ 4 * 8, 11 * 16, 19 * 8, 16 },
		"YYYY/MM/DD HH:MM:SS",
	};

	Button button_ok {
		{ 4 * 8, 13 * 16, 8 * 8, 24 },
		"OK",
	};
	Button button_cancel {
		{ 18 * 8, 13 * 16, 8 * 8, 24 },
		"Cancel",
	};

	void form_init(const SetDateTimeModel model);
	SetDateTimeModel form_collect();
};

struct SetFrequencyCorrectionModel {
	int8_t ppm;
};

class SetFrequencyCorrectionView : public View {
public:
	std::function<void(SetFrequencyCorrectionModel)> on_ok;
	std::function<void()> on_cancel;

	SetFrequencyCorrectionView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 5 * 8, 7 * 16, 20 * 8, 16 },
		"Frequency Correction"
	};

	NumberField field_ppm {
		{ 11 * 8, 9 * 16 },
		3,
		{ -50, 50 },
		1,
		'0',
	};
	Text text_ppm {
		{ 15 * 8, 9 * 16, 3 * 8, 16 },
		"PPM",
	};

	Button button_ok {
		{ 4 * 8, 13 * 16, 8 * 8, 24 },
		"OK",
	};
	Button button_cancel {
		{ 18 * 8, 13 * 16, 8 * 8, 24 },
		"Cancel",
	};

	void form_init(const SetFrequencyCorrectionModel model);
	SetFrequencyCorrectionModel form_collect();
};

class AntennaBiasSetupView : public View {
public:
	AntennaBiasSetupView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 5 * 8, 3 * 16, 20 * 8, 16 },
		"Antenna Bias Voltage"
	};

	Text text_description_1 {
		{ 24, 6 * 16, 24 * 8, 16 },
		"CAUTION: Ensure that all"
	};

	Text text_description_2 {
		{ 28, 7 * 16, 23 * 8, 16 },
		"devices attached to the"
	};

	Text text_description_3 {
		{  8, 8 * 16, 28 * 8, 16 },
		"antenna connector can accept"
	};

	Text text_description_4 {
		{ 68, 9 * 16, 13 * 8, 16 },
		"a DC voltage!"
	};

	OptionsField options_bias {
		{ 100, 12 * 16 },
		5,
		{
			{ " Off ", 0 },
			{ " On  ", 1 },
		}
	};

	Button button_done {
		{ 72, 15 * 16, 96, 24 },
		"Done"
	};
};

class AboutView : public View {
public:
	AboutView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 100, 96, 40, 16 },
		"About",
	};

	Text text_firmware {
		{ 0, 128, 240, 16 },
		"Git Commit Hash        " GIT_REVISION,
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

class SetupMenuView : public MenuView {
public:
	SetupMenuView(NavigationView& nav);
};

} /* namespace ui */

#endif/*__UI_SETUP_H__*/
