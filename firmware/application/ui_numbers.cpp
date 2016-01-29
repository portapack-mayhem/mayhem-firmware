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

#include "ui_numbers.hpp"

#include "ch.h"
#include "evtimer.h"

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

void NumbersStationView::focus() {
	button_exit.focus();
}

NumbersStationView::~NumbersStationView() {
	transmitter_model.disable();
}

void NumbersStationView::paint(Painter& painter) {
	(void)painter;
}

NumbersStationView::NumbersStationView(
	NavigationView& nav,
	TransmitterModel& transmitter_model
) : transmitter_model(transmitter_model)
{
	uint8_t m, d, dayofweek;
	uint16_t y;
	
	add_children({ {
		&text_title,
		&button_exit
	} });
	
	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	
	// Thanks, Sakamoto-sama !
	y = datetime.year();
	m = datetime.month();
	d = datetime.day();
	y -= m < 3;
	dayofweek = (y + y/4 - y/100 + y/400 + month_table[m-1] + d) % 7;
	
	text_title.set(day_of_week[dayofweek]);

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
