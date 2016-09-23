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

#include "ui_nuoptix.hpp"

#include "ch.h"

//#include "lfsr_random.hpp"
#include "ui_alphanum.hpp"
#include "portapack.hpp"
#include "string_format.hpp"

#include "portapack_shared_memory.hpp"

#include <cstring>
#include <vector>

using namespace portapack;

namespace ui {

void NuoptixView::focus() {
	number_timecode.focus();
}

void NuoptixView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void NuoptixView::transmit(bool setup) {
	uint8_t mod;
	uint8_t c;
	
	if (!txing) {
		transmitter_model.disable();
		return;
	}
	
	if (setup) {
		pbar.set_max(4);
		
		timecode = number_timecode.value();
		
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
		
		shared_memory.tx_data[0] = '*';	// "Pre-tone for restart" method #1
		shared_memory.tx_data[1] = 'A';	// "Restart" method #1
	} else {
		shared_memory.tx_data[0] = '#';
		shared_memory.tx_data[1] = (timecode / 1000) % 10;
		chThdSleepMilliseconds(92);		// 141-49ms
		number_timecode.set_value(timecode);
	}
	
	pbar.set_value(0);
	
	//strcpy("#00028", shared_memory.tx_data);
	shared_memory.tx_data[2] = (timecode / 100) % 10;
	shared_memory.tx_data[3] = (timecode / 10) % 10;
	shared_memory.tx_data[4] = timecode % 10;
	
	mod = 0;
	for (c = 1; c < 5; c++)
		if (shared_memory.tx_data[c] <= 9)
			mod += shared_memory.tx_data[c];
	
	mod = 10 - (mod % 10);
	if (mod == 10) mod = 0;		// Is this right ?
	text_mod.set("Mod: " + to_string_dec_uint(mod));
	
	shared_memory.tx_data[5] = mod;
	
	shared_memory.tx_data[6] = 0xFF;
	
	baseband::set_dtmf_data(number_bw.value(), 49, 49);	// 49ms tone, 49ms space
	
	timecode++;
}

NuoptixView::NuoptixView(
	NavigationView& nav
)
{
	
	baseband::run_image(portapack::spi_flash::image_tag_dtmf_tx);

	add_children({ {
		&field_frequency,
		&number_bw,
		&text_kHz,
		&number_timecode,
		&text_timecode,
		&text_mod,
		&pbar,
		&button_tx,
		&button_exit
	} });
	
	//check_loop.set_value(false);
	number_bw.set_value(15);
	number_timecode.set_value(1);

	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(10000);
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

	button_tx.on_select = [this](Button&){
		if (txing) {
			txing = false;
		} else {
			txing = true;
			transmit(true);
		}
	};
	
	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

NuoptixView::~NuoptixView() {
	transmitter_model.disable();
	baseband::shutdown();
}

}
