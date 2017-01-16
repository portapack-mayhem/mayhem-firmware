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

#include "ui_audiotx.hpp"

#include "ch.h"

#include "ui_alphanum.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void AudioTXView::focus() {
	button_transmit.focus();
}

void AudioTXView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

AudioTXView::AudioTXView(
	NavigationView& nav
)
{
	transmitter_model.set_tuning_frequency(92200000);
		
	add_children({
		&text_title,
		&field_frequency,
		&button_transmit,
		&button_exit
	});
	
	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};
	
	button_transmit.on_select = [](Button&){
		transmitter_model.set_baseband_configuration({
			.mode = 1,
			.sampling_rate = 1536000,
			.decimation_factor = 1,
		});
		transmitter_model.set_rf_amp(true);
		transmitter_model.enable();
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

AudioTXView::~AudioTXView() {
	transmitter_model.disable();
}

}
