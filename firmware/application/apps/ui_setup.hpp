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

#ifndef __UI_SETUP_H__
#define __UI_SETUP_H__

#include "ui_widget.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"
#include "ff.h"

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
	SetDateTimeView(NavigationView& nav);

	void focus() override;
	
	std::string title() const override { return "Set Date/Time"; };

private:
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

	void form_init(const SetDateTimeModel& model);
	SetDateTimeModel form_collect();
};

struct SetFrequencyCorrectionModel {
	int8_t ppm;
};

class SetRadioView : public View {
public:
	SetRadioView(NavigationView& nav);

	void focus() override;
	
	std::string title() const override { return "Radio settings"; };

private:
	Labels labels {
		{ { 2 * 8, 2 * 16 }, "Frequency correction:", Color::light_grey() },
		{ { 6 * 8, 3 * 16 }, "PPM", Color::light_grey() },
	};

	NumberField field_ppm {
		{ 2 * 8, 3 * 16 },
		3,
		{ -50, 50 },
		1,
		'0',
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

	Checkbox check_bias {
		{ 28, 12 * 16 },
		5,
		"Turn on bias voltage"
	};

	Button button_done {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Done"
	};
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel",
	};

	void form_init(const SetFrequencyCorrectionModel& model);
	SetFrequencyCorrectionModel form_collect();
};

class SetTouchCalibView : public View {
public:
	SetTouchCalibView(NavigationView& nav);
	void focus() override;
	bool on_touch(const TouchEvent event) override;
	
private:
	
	Text text_title {
		{ 64, 32, 40, 16 },
		"UL,UR,DL,DR !",
	};
	
	Text text_debugx {
		{ 64, 64, 40, 16 },
		"X",
	};
	
	Text text_debugy {
		{ 64, 80, 40, 16 },
		"Y",
	};
	
	Button button_ok {
		{ 72, 192, 96, 24 },
		"OK"
	};
};

class SetUIView : public View {
public:
	SetUIView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "UI settings"; };
	
private:
	Checkbox checkbox_login {
		{ 3 * 8, 2 * 16 },
		20,
		"Login with play dead"
	};
	
	Checkbox checkbox_bloff {
		{ 3 * 8, 5 * 16 },
		20,
		"Backlight off after:"
	};

	OptionsField options_bloff {
		{ 52, 6 * 16 + 8 },
		10,
		{
			{ "5 seconds", 5 },
			{ "15 seconds", 15 },
			{ "1 minute", 60 },
			{ "5 minutes", 300 },
			{ "10 minutes", 600 }
		}
	};
	
	Checkbox checkbox_showsplash {
		{ 3 * 8, 8 * 16 },
		11,
		"Show splash"
	};
	
	Button button_ok {
		{ 72, 260, 96, 32 },
		"OK"
	};
};

class SetPlayDeadView : public View {
public:
	SetPlayDeadView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "Playdead settings"; };
	
private:
	bool entermode = false;
	uint32_t sequence { 0 };
	uint8_t keycount { 0 }, key_code { };
	char sequence_txt[11] = { 0 };
	
	Text text_sequence {
		{ 64, 32, 14 * 8, 16 },
		"Enter sequence",
	};
	
	Button button_enter {
		{ 16, 192, 96, 24 },
		"Enter"
	};
	Button button_cancel {
		{ 128, 192, 96, 24 },
		"Cancel"
	};
};

/*class ModInfoView : public View {
public:
	ModInfoView(NavigationView& nav);
	void focus() override;
	void on_show() override;
	
private:
	void update_infos(uint8_t modn);
	
	typedef struct moduleinfo_t{
		char filename[9];
		uint16_t version;
		uint32_t size;
		char md5[16];
		char name[16];
		char description[214];
	} moduleinfo_t;
	
	moduleinfo_t module_list[8];	// 8 max for now
	
	uint8_t modules_nb;
	
	Text text_modcount {
		{ 2 * 8, 1 * 16, 18 * 8, 16 },
		"Searching..."
	};
	
	OptionsField option_modules {
		{ 2 * 8, 2 * 16 },
		24,
		{
			{ "-", 0 }
		}
	};
	
	Text text_name {
		{ 2 * 8, 4 * 16, 5 * 8, 16 },
		"Name:"
	};
	Text text_namestr {
		{ 8 * 8, 4 * 16, 16 * 8, 16 },
		"..."
	};
	Text text_size {
		{ 2 * 8, 5 * 16, 5 * 8, 16 },
		"Size:"
	};
	Text text_sizestr {
		{ 8 * 8, 5 * 16, 16 * 8, 16 },
		"..."
	};
	Text text_md5 {
		{ 2 * 8, 6 * 16, 4 * 8, 16 },
		"MD5:"
	};
	Text text_md5_a {
		{ 7 * 8, 6 * 16, 16 * 8, 16 },
		"..."
	};
	Text text_md5_b {
		{ 7 * 8, 7 * 16, 16 * 8, 16 },
		"..."
	};
	
	Button button_ok {
		{ 4 * 8, 272, 64, 24 },
		"Ok"
	};
};*/

class SetupMenuView : public MenuView {
public:
	SetupMenuView(NavigationView& nav);
	
	std::string title() const override { return "Settings"; };
};

} /* namespace ui */

#endif/*__UI_SETUP_H__*/
