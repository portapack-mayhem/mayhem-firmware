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
#include "touch.hpp"

#include "portapack_persistent_memory.hpp"
#include "lpc43xx_cpp.hpp"

#include <math.h>

using namespace lpc43xx;

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

void SetDateTimeView::form_init(const SetDateTimeModel model) {
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
		portapack::persistent_memory::correction_ppb() / 1000
	};

	form_init(model);
}

void SetFrequencyCorrectionView::focus() {
	button_cancel.focus();
}

void SetFrequencyCorrectionView::form_init(const SetFrequencyCorrectionModel model) {
	field_ppm.set_value(model.ppm);
}

SetFrequencyCorrectionModel SetFrequencyCorrectionView::form_collect() {
	return {
		.ppm = static_cast<int8_t>(field_ppm.value()),
	};
}

AboutView::AboutView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_firmware,
		&text_cpld_hackrf,
		&text_cpld_portapack,
		&button_ok,
	} });

	button_ok.on_select = [&nav](Button&){ nav.pop(); };
}

void AboutView::focus() {
	button_ok.focus();
}

SetTouchCalibView::SetTouchCalibView(NavigationView& nav) {
	add_children({{
		&text_title,
		&text_debugx,
		&text_debugy,
		&button_ok
		}});

	button_ok.on_select = [&nav](Button&){ nav.pop(); };
}

void SetTouchCalibView::focus() {
	button_ok.focus();
}

bool SetTouchCalibView::on_touch(const TouchEvent event) {
	if (event.type == ui::TouchEvent::Type::Start) {
		text_debugx.set(to_string_dec_uint(round(event.rawpoint.x), 4));
		text_debugy.set(to_string_dec_uint(round(event.rawpoint.y), 4));
	}
	return true;
}

SetupMenuView::SetupMenuView(NavigationView& nav) {
	add_items<3>({ {
		{ "Date/Time", [&nav](){ nav.push(new SetDateTimeView { nav }); } },
		{ "Frequency Correction", [&nav](){ nav.push(new SetFrequencyCorrectionView { nav }); } },
		{ "Touch",     [&nav](){ nav.push(new SetTouchCalibView { nav }); } },
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
