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

#include "ui_setup.hpp"

#include "ui_touch_calibration.hpp"

#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "portapack.hpp"
using portapack::receiver_model;

#include "cpld_update.hpp"

namespace ui {

SetDateTimeView::SetDateTimeView(
	NavigationView& nav
) {
	button_ok.on_select = [&nav, this](Button&){
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

	add_children({ {
		&text_title,
		&field_year,
		&text_slash1,
		&field_month,
		&text_slash2,
		&field_day,
		&field_hour,
		&text_colon1,
		&field_minute,
		&text_colon2,
		&field_second,
		&text_format,
		&button_ok,
		&button_cancel,
	} });

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

SetFrequencyCorrectionView::SetFrequencyCorrectionView(
	NavigationView& nav
) {
	button_ok.on_select = [&nav, this](Button&){
		const auto model = this->form_collect();
		portapack::persistent_memory::set_correction_ppb(model.ppm * 1000);
		nav.pop();
	},

	button_cancel.on_select = [&nav](Button&){
		nav.pop();
	},

	add_children({ {
		&text_title,
		&field_ppm,
		&text_ppm,
		&button_ok,
		&button_cancel,
	} });

	SetFrequencyCorrectionModel model {
		static_cast<int8_t>(portapack::persistent_memory::correction_ppb() / 1000)
	};

	form_init(model);
}

void SetFrequencyCorrectionView::focus() {
	button_cancel.focus();
}

void SetFrequencyCorrectionView::form_init(const SetFrequencyCorrectionModel& model) {
	field_ppm.set_value(model.ppm);
}

SetFrequencyCorrectionModel SetFrequencyCorrectionView::form_collect() {
	return {
		.ppm = static_cast<int8_t>(field_ppm.value()),
	};
}

AntennaBiasSetupView::AntennaBiasSetupView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_description_1,
		&text_description_2,
		&text_description_3,
		&text_description_4,
		&options_bias,
		&button_done,
	} });

	options_bias.set_by_value(receiver_model.antenna_bias() ? 1 : 0);
	options_bias.on_change = [this](size_t, OptionsField::value_t v) {
		receiver_model.set_antenna_bias(v);
	};

	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void AntennaBiasSetupView::focus() {
	button_done.focus();
}

AboutView::AboutView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_firmware,
		&text_cpld_hackrf,
		&text_cpld_hackrf_status,
		&button_ok,
	} });

	button_ok.on_select = [&nav](Button&){ nav.pop(); };

	if( cpld_hackrf_verify_eeprom() ) {
		text_cpld_hackrf_status.set(" OK");
	} else {
		text_cpld_hackrf_status.set("BAD");
	}
}

void AboutView::focus() {
	button_ok.focus();
}

SetupMenuView::SetupMenuView(NavigationView& nav) {
	add_items<4>({ {
		{ "Date/Time", [&nav](){ nav.push<SetDateTimeView>(); } },
		{ "Frequency Correction", [&nav](){ nav.push<SetFrequencyCorrectionView>(); } },
		{ "Antenna Bias Voltage", [&nav](){ nav.push<AntennaBiasSetupView>(); } },
		{ "Touch",     [&nav](){ nav.push<TouchCalibrationView>(); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
