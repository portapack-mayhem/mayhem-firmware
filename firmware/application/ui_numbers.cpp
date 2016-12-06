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

// TODO: Total transmission time (all durations / 44100)

using namespace portapack;
using namespace hackrf::one;

namespace ui {

void NumbersStationView::focus() {
	button_exit.focus();

	if (file_error) {
		nav_.display_modal("No files", "Missing files in /numbers/", ABORT, nullptr);
	}
}

NumbersStationView::~NumbersStationView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void NumbersStationView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void NumbersStationView::prepare_audio() {
	uint8_t code;
	
	if (cnt >= sample_duration) {
		if (segment == ANNOUNCE) {
			if (!announce_loop) {
				segment = MESSAGE;
			} else {
				auto error = file.open("/numbers/announce.wav");
				if (error.is_valid()) return;
				
				sample_duration = sound_sizes[10];
				
				file.seek(44);		// Skip header
				
				announce_loop--;
			}
		}
		
		if (segment == MESSAGE) {
			if (id_test == 10)
				transmitter_model.disable();
			
			code = symfield_code.value(id_test);
			
			if (code == 10) {
				pause = 11025;		// p: 0.25s @ 44100Hz
				memset(audio_buffer, 0, 1024);
			} else if (code == 11) {
				pause = 33075;		// P: 0.75s @ 44100Hz
				memset(audio_buffer, 0, 1024);
			} else {
				auto error = file.open("/numbers/" + file_names[code] + ".wav");
				if (error.is_valid()) return;
				
				sample_duration = sound_sizes[code];
				
				file.seek(44);		// Skip header
			}
			
			id_test++;
		}
		
		cnt = 0;
	}
	
	if (!pause) {
		size_t bytes_read = file.read(audio_buffer, 1024).value();
		
		// Unsigned to signed, pretty stupid :/
		for (size_t n = 0; n < bytes_read; n++)
			audio_buffer[n] -= 0x80;
		
		cnt += 1024;
	} else {
		if (pause >= 1024) {
			pause -= 1024;
		} else {
			cnt = sample_duration;
			pause = 0;
		}
	}
	
	baseband::set_fifo_data(audio_buffer);
}

void NumbersStationView::start_tx() {
	uint32_t divider;

	sample_duration = sound_sizes[0];
	cnt = sample_duration;
	
	id_test = 0;
	announce_loop = 2;
	segment = ANNOUNCE;
	
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

// TODO: Copied from soundboard, make globally available
uint16_t NumbersStationView::fb_to_uint16(const std::string& fb) {
	return (fb[1] << 8) + fb[0];
}

uint32_t NumbersStationView::fb_to_uint32(const std::string& fb) {
	return (fb[3] << 24) + (fb[2] << 16) + (fb[1] << 8) + fb[0];
}

NumbersStationView::NumbersStationView(
	NavigationView& nav
) : nav_ (nav)
{
	uint8_t c;
	uint8_t y, m, d, dayofweek;
	size_t size;
	
	char file_buffer[32];
	
	c = 0;
	for (auto& file_name : file_names) {
		auto error = file.open("/numbers/" + file_name + ".wav");
		if (!error.is_valid()) {
			file.seek(22);
			file.read(file_buffer, 2);
			
			// Is file mono ?
			if (fb_to_uint16(file_buffer) == 1) {
				file.seek(40);
				file.read(file_buffer, 4);
				size = fb_to_uint32(file_buffer);
				if (!size) break;
				sound_sizes[c] = size;
				c++;
			} else {
				break;
			}
		} else {
			break;
		}
	}
	
	if (c != 11) file_error = true;
	
	baseband::run_image(portapack::spi_flash::image_tag_audio_tx);
	
	add_children({ {
		&text_title,
		&field_frequency,
		&number_bw,
		&symfield_code,
		&button_tx,
		&button_exit
	} });

	number_bw.set_value(120);

	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(50000);
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};
	
	// DEBUG
	symfield_code.set_value(0, 10);
	symfield_code.set_value(1, 3);
	symfield_code.set_value(2, 4);
	symfield_code.set_value(3, 11);
	symfield_code.set_value(4, 6);
	symfield_code.set_value(5, 1);
	symfield_code.set_value(6, 9);
	symfield_code.set_value(7, 7);
	symfield_code.set_value(8, 8);
	symfield_code.set_value(9, 0);
	transmitter_model.set_tuning_frequency(103300000);	// 103.3MHz

	for (c = 0; c < 10; c++)
		symfield_code.set_symbol_list(c, "0123456789pP");



	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	
	// Thanks, Sakamoto-sama !
	y = datetime.year();
	m = datetime.month();
	d = datetime.day();
	y -= m < 3;
	dayofweek = (y + y/4 - y/100 + y/400 + month_table[m-1] + d) % 7;
	
	text_title.set(day_of_week[dayofweek]);

	button_tx.on_select = [this, &nav](Button&){
		this->start_tx();
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

} /* namespace ui */
