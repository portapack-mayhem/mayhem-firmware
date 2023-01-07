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

#include "ui_settings.hpp"

#include "ui_navigation.hpp"
#include "ui_touch_calibration.hpp"

#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "audio.hpp"
#include "portapack.hpp"
using portapack::receiver_model;
using namespace portapack;

#include "string_format.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "cpld_update.hpp"

namespace ui {

SetDateTimeView::SetDateTimeView(
	NavigationView& nav
) {
	button_save.on_select = [&nav, this](Button&){
		const auto model = this->form_collect();
		const rtc::RTC new_datetime {
			model.year, model.month, model.day,
			model.hour, model.minute, model.second
		};
		rtcSetTime(&RTCD1, &new_datetime);
		nav.pop();
	},

	button_cancel.on_select = [&nav](Button&){
		nav.pop();
	},

	add_children({
		&labels,
		&field_year,
		&field_month,
		&field_day,
		&field_hour,
		&field_minute,
		&field_second,
		&button_save,
		&button_cancel,
	});

	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	SetDateTimeModel model {
		datetime.year(),
		datetime.month(),
		datetime.day(),
		datetime.hour(),
		datetime.minute(),
		datetime.second()
	};

	form_init(model);
}

void SetDateTimeView::focus() {
	button_cancel.focus();
}

void SetDateTimeView::form_init(const SetDateTimeModel& model) {
	field_year.set_value(model.year);
	field_month.set_value(model.month);
	field_day.set_value(model.day);
	field_hour.set_value(model.hour);
	field_minute.set_value(model.minute);
	field_second.set_value(model.second);
}

SetDateTimeModel SetDateTimeView::form_collect() {
	return {
		.year = static_cast<uint16_t>(field_year.value()),
		.month = static_cast<uint8_t>(field_month.value()),
		.day = static_cast<uint8_t>(field_day.value()),
		.hour = static_cast<uint8_t>(field_hour.value()),
		.minute = static_cast<uint8_t>(field_minute.value()),
		.second = static_cast<uint8_t>(field_second.value())
	};
}

SetRadioView::SetRadioView(
	NavigationView& nav
) {
	button_cancel.on_select = [&nav](Button&){
		nav.pop();
	};

	const auto reference = portapack::clock_manager.get_reference();
	
	std::string source_name("---");
	switch(reference.source) {
	case ClockManager::ReferenceSource::Xtal:      source_name = "HackRF";    break;
	case ClockManager::ReferenceSource::PortaPack: source_name = "PortaPack"; break;
	case ClockManager::ReferenceSource::External:  source_name = "External";  break;
	}

	value_source.set(source_name);
	value_source_frequency.set(to_string_dec_uint(reference.frequency / 1000000, 2) + "." + to_string_dec_uint((reference.frequency % 1000000) / 100, 4, '0') + " MHz");

	label_source.set_style(&style_text);
	value_source.set_style(&style_text);
	value_source_frequency.set_style(&style_text);

	add_children({
		&label_source,
		&value_source,
		&value_source_frequency,
	});

	if( reference.source == ClockManager::ReferenceSource::Xtal ) {
		add_children({
			&labels_correction,
			&field_ppm,
		});
	}

	add_children({
		&check_clkout,
		&field_clkout_freq,
		&labels_clkout_khz,
		&value_freq_step,
		&labels_bias,
		&check_bias,
		&button_save,
		&button_cancel
	});

	SetFrequencyCorrectionModel model {
		static_cast<int8_t>(portapack::persistent_memory::correction_ppb() / 1000) , 0
	};

	form_init(model);

	check_clkout.set_value(portapack::persistent_memory::clkout_enabled());
	check_clkout.on_select = [this](Checkbox&, bool v) {
		clock_manager.enable_clock_output(v);
		portapack::persistent_memory::set_clkout_enabled(v);
		StatusRefreshMessage message { };
		EventDispatcher::send_message(message);
	};

	field_clkout_freq.set_value(portapack::persistent_memory::clkout_freq());
	value_freq_step.set_style(&style_text);

	field_clkout_freq.on_select = [this](NumberField&) {
		freq_step_khz++;
		if(freq_step_khz > 3) {
			freq_step_khz = 0;
		}
		switch(freq_step_khz) {
			case 0:
				value_freq_step.set("   |");
				break;
			case 1:
				value_freq_step.set("  | ");
				break;
			case 2:
				value_freq_step.set(" |  ");
				break;
			case 3:
				value_freq_step.set("|   ");
				break;
		}
		field_clkout_freq.set_step(pow(10, freq_step_khz));
	};

	check_bias.set_value(portapack::get_antenna_bias());
	check_bias.on_select = [this](Checkbox&, bool v) {
		portapack::set_antenna_bias(v);
		StatusRefreshMessage message { };
		EventDispatcher::send_message(message);
	};

	button_save.on_select = [this, &nav](Button&){
		const auto model = this->form_collect();
		portapack::persistent_memory::set_correction_ppb(model.ppm * 1000);
		portapack::persistent_memory::set_clkout_freq(model.freq);
		clock_manager.enable_clock_output(portapack::persistent_memory::clkout_enabled());
		nav.pop();
	};
}

void SetRadioView::focus() {
	button_save.focus();
}

void SetRadioView::form_init(const SetFrequencyCorrectionModel& model) {
	field_ppm.set_value(model.ppm);
}

SetFrequencyCorrectionModel SetRadioView::form_collect() {
	return {
		.ppm = static_cast<int8_t>(field_ppm.value()),
		.freq = static_cast<uint32_t>(field_clkout_freq.value()),
	};
}


SetUIView::SetUIView(NavigationView& nav) {
	add_children({
		&checkbox_disable_touchscreen,
		&checkbox_speaker,
		&checkbox_bloff,
		&options_bloff,
		&checkbox_showsplash,
		&checkbox_showclock,
		&options_clockformat,
		&checkbox_guireturnflag,
		&button_save,
		&button_cancel
	});
	
	checkbox_disable_touchscreen.set_value(persistent_memory::disable_touchscreen());
	checkbox_speaker.set_value(persistent_memory::config_speaker());
	checkbox_showsplash.set_value(persistent_memory::config_splash());
	checkbox_showclock.set_value(!persistent_memory::hide_clock());
	checkbox_guireturnflag.set_value(persistent_memory::show_gui_return_icon());
	
	const auto backlight_config = persistent_memory::config_backlight_timer();
	checkbox_bloff.set_value(backlight_config.timeout_enabled());
	options_bloff.set_by_value(backlight_config.timeout_enum());

	if (persistent_memory::clock_with_date()) {
		options_clockformat.set_selected_index(1);
	} else {
		options_clockformat.set_selected_index(0);
	}


	button_save.on_select = [&nav, this](Button&) {
		persistent_memory::set_config_backlight_timer({
			(persistent_memory::backlight_timeout_t)options_bloff.selected_index_value(),
			checkbox_bloff.value()
		});
				
		if (checkbox_showclock.value()){
		    if (options_clockformat.selected_index() == 1)
			    persistent_memory::set_clock_with_date(true);    
     		else
			    persistent_memory::set_clock_with_date(false);		
		}

		if (checkbox_speaker.value())  audio::output::speaker_mute();		//Just mute audio if speaker is disabled
		persistent_memory::set_config_speaker(checkbox_speaker.value());	//Store Speaker status
        	StatusRefreshMessage message { };					//Refresh status bar with/out speaker
        	EventDispatcher::send_message(message);
	
		persistent_memory::set_config_splash(checkbox_showsplash.value());
		persistent_memory::set_clock_hidden(!checkbox_showclock.value());
		persistent_memory::set_gui_return_icon(checkbox_guireturnflag.value());
		persistent_memory::set_disable_touchscreen(checkbox_disable_touchscreen.value());
		nav.pop();
	};
	button_cancel.on_select = [&nav, this](Button&) {
		nav.pop();
	};
}

void SetUIView::focus() {
	button_save.focus();
}


// ---------------------------------------------------------
// Appl. Settings
// ---------------------------------------------------------
SetAppSettingsView::SetAppSettingsView(NavigationView& nav) {
	add_children({
		&checkbox_load_app_settings,
		&checkbox_save_app_settings,
		&button_save,
		&button_cancel
	});
	
	checkbox_load_app_settings.set_value(persistent_memory::load_app_settings());
	checkbox_save_app_settings.set_value(persistent_memory::save_app_settings());


	button_save.on_select = [&nav, this](Button&) {
		persistent_memory::set_load_app_settings(checkbox_load_app_settings.value());
		persistent_memory::set_save_app_settings(checkbox_save_app_settings.value());
		nav.pop();
	};
	button_cancel.on_select = [&nav, this](Button&) {
		nav.pop();
	};
}

void SetAppSettingsView::focus() {
	button_save.focus();
}

SetAudioView::SetAudioView(NavigationView& nav) {
	add_children({
		&labels,
		&field_tone_mix,
		&button_save,
		&button_cancel
	});

	field_tone_mix.set_value(persistent_memory::tone_mix());
	
	button_save.on_select = [&nav, this](Button&) {
		persistent_memory::set_tone_mix(field_tone_mix.value());
		nav.pop();
	};

	button_cancel.on_select = [&nav, this](Button&) {
		nav.pop();
	};
}

void SetAudioView::focus() {
	button_save.focus();
}

SetQRCodeView::SetQRCodeView(NavigationView& nav) {
	add_children({
		&checkbox_bigger_qr,
		&button_save,
		&button_cancel
	});

	checkbox_bigger_qr.set_value(persistent_memory::show_bigger_qr_code());
	
	button_save.on_select = [&nav, this](Button&) {
		persistent_memory::set_show_bigger_qr_code(checkbox_bigger_qr.value());
		nav.pop();
	};

	button_cancel.on_select = [&nav, this](Button&) {
		nav.pop();
	};

}

void SetQRCodeView::focus() {
	button_save.focus();
}

// ---------------------------------------------------------
// Settings main menu
// ---------------------------------------------------------
SettingsMenuView::SettingsMenuView(NavigationView& nav) {
    if( portapack::persistent_memory::show_gui_return_icon() )
    {
        add_items( { { "..", 		ui::Color::light_grey(),&bitmap_icon_previous,	[&nav](){ nav.pop(); } } } );
    }
	add_items({
		{ "Audio", 		ui::Color::dark_cyan(), &bitmap_icon_speaker,			[&nav](){ nav.push<SetAudioView>(); } },
		{ "Radio",		ui::Color::dark_cyan(), &bitmap_icon_options_radio,		[&nav](){ nav.push<SetRadioView>(); } },
		{ "User Interface", 	ui::Color::dark_cyan(), &bitmap_icon_options_ui,		[&nav](){ nav.push<SetUIView>(); } },
		{ "Date/Time",		ui::Color::dark_cyan(), &bitmap_icon_options_datetime,		[&nav](){ nav.push<SetDateTimeView>(); } },
		{ "Calibration",	ui::Color::dark_cyan(), &bitmap_icon_options_touch,		[&nav](){ nav.push<TouchCalibrationView>(); } },
		{ "App Settings",	ui::Color::dark_cyan(), &bitmap_icon_setup,			[&nav](){ nav.push<SetAppSettingsView>(); } },
		{ "QR Code",		ui::Color::dark_cyan(), &bitmap_icon_qr_code,			[&nav](){ nav.push<SetQRCodeView>(); } }
	});
	set_max_rows(2); // allow wider buttons
}

} /* namespace ui */
