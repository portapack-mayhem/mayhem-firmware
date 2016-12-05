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
#include "ff.h"
#include "portapack.hpp"
#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace hackrf::one;

namespace ui {

void NumbersStationView::focus() {
	button_exit.focus();
}

NumbersStationView::~NumbersStationView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void NumbersStationView::paint(Painter& painter) {
	(void)painter;
}

void NumbersStationView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void NumbersStationView::prepare_audio() {
	if (cnt >= sample_duration) {
		/*if (!check_loop.value()) {
			transmitter_model.disable();
			return;
		} else {
			file.seek(44);
			cnt = 0;
		}*/
		
		// DEBUG
			file.seek(44);
			cnt = 0;
		
	}
	
	file.read(audio_buffer, 1024);
	
	// Unsigned to signed, pretty stupid :/
	for (size_t n = 0; n < 1024; n++)
		audio_buffer[n] -= 0x80;
	
	cnt += 1024;
	
	baseband::set_fifo_data(audio_buffer);
}

void NumbersStationView::play_sound(uint16_t id) {
	uint32_t divider;

	if (sounds[id].size == 0) return;

	auto error = file.open("/numbers/" + filenames[id] + ".wav");
	if (error.is_valid()) return;
	
	sample_duration = sounds[id].sample_duration;
	
	cnt = 0;
	file.seek(44);	// Skip header
	
	prepare_audio();
	
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 1536000,
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	divider = (1536000 / 44100) - 1;
	
	baseband::set_audiotx_data(
		divider,
		number_bw.value(),
		false,
		0
	);
}

NumbersStationView::NumbersStationView(
	NavigationView& nav
) {
	uint8_t m, d, dayofweek;
	uint16_t y;
	
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
	
	add_children({ {
		&text_title,
		&number_bw,
		&button_exit
	} });

	number_bw.set_value(15);

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
