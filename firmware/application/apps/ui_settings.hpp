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
#include "portapack_persistent_memory.hpp"

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
	
	std::string title() const override { return "Date/Time"; };

private:
	Labels labels {
		{ { 6 * 8, 7 * 16 }, "YYYY-MM-DD HH:MM:SS", Color::grey() },
		{ { 10 * 8, 9 * 16 }, "-  -     :  :", Color::light_grey() }
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

	Button button_save {
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
	uint32_t freq;
};

class SetRadioView : public View {
public:
	SetRadioView(NavigationView& nav);

	void focus() override;
	
	std::string title() const override { return "Radio"; };

private:
	const Style style_text {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::light_grey(),
	};
	uint8_t freq_step_khz = 3;

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
		{ { 2 * 8, 3 * 16 }, "Frequency correction:", Color::light_grey() },
		{ { 6 * 8, 4 * 16 }, "PPM", Color::light_grey() },
	};

   	Checkbox check_clkout {
		{ 18, (6 * 16 - 4) },
		13,
		"Enable CLKOUT"
	};

	NumberField field_clkout_freq {
		{ 20 * 8, 6 * 16 },
		5,
		{ 10, 60000 },
		1000,
		' '
	};

	Labels labels_clkout_khz {
		{ { 26 * 8, 6 * 16 }, "kHz", Color::light_grey() }
	};

	Text value_freq_step {
		{ 21 * 8, (7 * 16 ), 4 * 8, 16 },
		"|   "
	};

	Labels labels_bias {
		{ { 24, 8 * 16 }, "CAUTION: Ensure that all", Color::red() },
		{ { 28, 9 * 16 }, "devices attached to the", Color::red() },
		{ { 8, 10 * 16 }, "antenna connector can accept", Color::red() },
		{ { 68, 11 * 16 }, "a DC voltage!", Color::red() }
	};

	NumberField field_ppm {
		{ 2 * 8, 4 * 16 },
		3,
		{ -50, 50 },
		1,
		'0',
	};
	
	Checkbox check_bias {
		{ 18, 12 * 16 },
		5,
		"Turn on bias voltage"
	};

    Checkbox check_hamitup {
		{ 18, 14 * 16},
		7,
		"HamItUp"
	};
    Button button_hamitup_freq {
		{ 240 - 15 * 8 , 14 * 16 , 15 * 8 , 24 },
		"",
	};

	Button button_save {
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

using portapack::persistent_memory::backlight_timeout_t;

class SetUIView : public View {
public:
	SetUIView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "UI"; };
	
private:

	Checkbox checkbox_disable_touchscreen {
		{ 3 * 8, 2 * 16 },
		20,
		"Disable touchscreen"
	};
	
	Checkbox checkbox_speaker {
		{ 3 * 8, 4 * 16 },
		20,
		"Hide H1 Speaker option"
	};
	
	Checkbox checkbox_bloff {
		{ 3 * 8, 6 * 16 },
		20,
		"Backlight off after:"
	};
	OptionsField options_bloff {
		{ 52, 7 * 16 + 8 },
		20,
		{
			{ "5 seconds",  backlight_timeout_t::Timeout5Sec    },
			{ "15 seconds", backlight_timeout_t::Timeout15Sec   },
			{ "30 seconds", backlight_timeout_t::Timeout30Sec   },
			{ "1 minute",   backlight_timeout_t::Timeout60Sec   },
			{ "3 minutes",  backlight_timeout_t::Timeout180Sec  },
			{ "5 minutes",  backlight_timeout_t::Timeout300Sec  },
			{ "10 minutes", backlight_timeout_t::Timeout600Sec  },
			{ "1 hour",     backlight_timeout_t::Timeout3600Sec },
		}
	};
	
	Checkbox checkbox_showsplash {
		{ 3 * 8, 9 * 16 },
		20,
		"Show splash"
	};
	
	Checkbox checkbox_showclock {	
		{ 3 * 8, 11 * 16 },
		20,
		"Show clock with:"
	};

	OptionsField options_clockformat {
		{ 52, 12 * 16 + 8 },
		20,
		{
			{ "time only", 0 },
			{ "time and date", 1 }
		}
	};	

    Checkbox checkbox_guireturnflag {	
		{ 3 * 8, 14 * 16 },
		25,
		"add return icon in GUI"
	};
	
	Button button_save {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
	
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel",
	};
};

class SetAppSettingsView : public View {
public:
	SetAppSettingsView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "AppSettings"; };
	
private:

	Checkbox checkbox_load_app_settings {
		{ 3 * 8, 2 * 16 },
		25,
		"Load app settings"
	};
	
	Checkbox checkbox_save_app_settings {
		{ 3 * 8, 4 * 16 },
		25,
		"Save app settings"
	};
	
	Button button_save {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
	
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel",
	};
};

class SetAudioView : public View {
public:
	SetAudioView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "Audio"; };
	
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
	
	Button button_save {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
	
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel",
	};
};


class SetQRCodeView : public View {
public:
	SetQRCodeView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "QR Code"; };
	
private:
	Checkbox checkbox_bigger_qr {
		{ 3 * 8, 9 * 16 },
		20,
		"Show large QR code"
	};
	
	Button button_save {
		{ 2 * 8, 16 * 16, 12 * 8, 32 },
		"Save"
	};
	
	Button button_cancel {
		{ 16 * 8, 16 * 16, 12 * 8, 32 },
		"Cancel",
	};
};

class SettingsMenuView : public BtnGridView {
public:
	SettingsMenuView(NavigationView& nav);
	
	std::string title() const override { return "Settings"; };
};

} /* namespace ui */

#endif/*__UI_SETTINGS_H__*/
