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

#include "ui_rds.hpp"
#include "ui_afsksetup.hpp"
#include "ui_receiver.hpp"

#include "ch.h"

#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace hackrf::one;

namespace ui {

void AFSKSetupView::focus() {
	button_setfreq.focus();
}

void AFSKSetupView::paint(Painter& painter) {

}

void AFSKSetupView::updfreq(rf::Frequency f) {
	char finalstr[9] = {0};
	
	persistent_memory::set_tuned_frequency(f);
	transmitter_model.set_tuning_frequency(f);
	
	auto mhz = to_string_dec_int(f / 1000000, 3);
	auto hz100 = to_string_dec_int((f / 100) % 10000, 4, '0');

	strcat(finalstr, mhz.c_str());
	strcat(finalstr, ".");
	strcat(finalstr, hz100.c_str());

	this->button_setfreq.set_text(finalstr);
}

AFSKSetupView::AFSKSetupView(
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	
	add_children({ {
		&text_title,
		&button_setfreq,
		&button_setbps,
		&text_mark,
		&field_mark,
		&text_space,
		&field_space,
		&button_done
	} });
	
	updfreq(persistent_memory::tuned_frequency());
	
	field_mark.set_value(persistent_memory::afsk_mark_freq()*100);
	field_space.set_value(persistent_memory::afsk_space_freq()*100);

	button_setfreq.on_select = [this,&nav](Button&){
		auto new_view = new FrequencyKeypadView { nav, this->transmitter_model.tuning_frequency() };
		new_view->on_changed = [this](rf::Frequency f) {
			updfreq(f);
		};
		nav.push(new_view);
	};
	
	if (persistent_memory::afsk_bitrate() == 1200) {
		button_setbps.set_text("1200 bps");
	} else {
		button_setbps.set_text("2400 bps");
	}
	
	button_setbps.on_select = [this](Button&){
		if (persistent_memory::afsk_bitrate() == 1200) {
			persistent_memory::set_afsk_bitrate(2400);
			button_setbps.set_text("2400 bps");
		} else {
			persistent_memory::set_afsk_bitrate(1200);
			button_setbps.set_text("1200 bps");
		}
	};

	button_done.on_select = [this,&nav](Button&){
		persistent_memory::set_afsk_mark(field_mark.value()/100);
		persistent_memory::set_afsk_space(field_space.value()/100);
		nav.pop();
	};
}

} /* namespace ui */
