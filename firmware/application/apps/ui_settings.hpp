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

#ifndef __UI_SETTINGS_H__
#define __UI_SETTINGS_H__

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
	Labels labels {
		{ { 6 * 8, 7 * 16 }, "YYYY/MM/DD HH:MM:SS", Color::grey() },
		{ { 10 * 8, 9 * 16 }, "/  /     :  :", Color::light_grey() }
	};
	
	NumberField field_year {
		{ 6 * 8, 9 * 16 },
		4,
		{ 2015, 2099 },
		1,
		'0',
	};
	NumberField field_month {
		{ 11 * 8, 9 * 16 },
		2,
		{ 1, 12 },
		1,
		'0',
	};
	NumberField field_day {
		{ 14 * 8, 9 * 16 },
		2,
		{ 1, 31 },
		1,
		'0',
	};

	NumberField field_hour {
		{ 17 * 8, 9 * 16 },
		2,
		{ 0, 23 },
		1,
		'0',
	};
	NumberField field_minute {
		{ 20 * 8, 9 * 16 },
		2,
		{ 0, 59 },
		1,
		'0',
	};
	NumberField field_second {
		{ 23 * 8, 9 * 16 },
		2,
		{ 0, 59 },
		1,
		'0',
	};

	Button button_done {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel"
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
	
	std::string title() const override { return "Radio Options"; };

private:
	const Style style_text {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::light_grey(),
	};

	Text label_source {
		{ 0, 1 * 16, 17 * 8, 16 },
		"Reference Source:"
	};

	Text value_source {
		{ (240 - 11 * 8), 1 * 16, 11 * 8, 16 },
		"---"
	};

	Text value_source_frequency {
		{ (240 - 11 * 8), 2 * 16, 11 * 8, 16 },
		"---"
	};

	Labels labels_correction {
		{ { 2 * 8, 4 * 16 }, "Frequency correction:", Color::light_grey() },
		{ { 6 * 8, 5 * 16 }, "PPM", Color::light_grey() },
	};

	Labels labels_bias {
		{ { 24, 8 * 16 }, "CAUTION: Ensure that all", Color::red() },
		{ { 28, 9 * 16 }, "devices attached to the", Color::red() },
		{ { 8, 10 * 16 }, "antenna connector can accept", Color::red() },
		{ { 68, 11 * 16 }, "a DC voltage!", Color::red() }
	};

	NumberField field_ppm {
		{ 2 * 8, 5 * 16 },
		3,
		{ -50, 50 },
		1,
		'0',
	};
	
	Checkbox check_bias {
		{ 28, 13 * 16 },
		5,
		"Turn on bias voltage"
	};

	Button button_done {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel",
	};

	void form_init(const SetFrequencyCorrectionModel& model);
	SetFrequencyCorrectionModel form_collect();
};

class SetUIView : public View {
public:
	SetUIView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "UI Options"; };
	
private:
	/*Checkbox checkbox_login {
		{ 3 * 8, 2 * 16 },
		20,
		"Login with play dead"
	};*/
	
	Checkbox checkbox_backbutton {
		{ 3 * 8, 3 * 16 },
		19,
		"Back dots in menues"
	};

	Checkbox checkbox_speaker {
		{ 3 * 8, 5 * 16 },
		20,
		"Speaker present (H1)"
	};


	Checkbox checkbox_bloff {
		{ 3 * 8, 7 * 16 },
		20,
		"Backlight off after:"
	};

	OptionsField options_bloff {
		{ 10 * 8, 9 * 16 },
		10,
		{
			{ "5 seconds", 5 },
			{ "15 seconds", 15 },
			{ "30 seconds", 30 },
			{ "1 minute", 60 },
			{ "3 minutes", 180 },
			{ "5 minutes", 300 },
			{ "10 minutes", 600 }
		}
	};
	
	Checkbox checkbox_showsplash {
		{ 3 * 8, 11 * 16 },
		11,
		"Show splash"
	};
	
	Button button_ok {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
};

class SetAudioView : public View {
public:
	SetAudioView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "Audio Options"; };
	
private:
	Labels labels {
		{ { 2 * 8, 3 * 16 }, "Tone key mix:   %", Color::light_grey() },
	};
	
	NumberField field_tone_mix {
		{ 16 * 8, 3 * 16 },
		2,
		{ 10, 99 },
		1,
		'0'
	};
	
	Button button_ok {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
};

/*
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
};*/

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

class SettingsMenuView : public BtnGridView {
public:
	SettingsMenuView(NavigationView& nav);
	
	std::string title() const override { return "Options"; };
};

} /* namespace ui */

#endif/*__UI_SETTINGS_H__*/
