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

#include "ui_epar.hpp"

#include "ch.h"
#include "hackrf_hal.hpp"

#include "event_m0.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void EPARView::focus() {
	city_code.focus();
}

EPARView::~EPARView() {
	transmitter_model.disable();
}

void EPARView::update_message() {
	uint8_t c;
	
	// Start bit
	epar_bits[0] = 1;
	
	// To binary
	for (c = 0; c < 8; c++)
		epar_bits[c + 1] = ((city_code.value() >> c) & 1);
	
	epar_bits[9] = options_group.selected_index_value() & 1;
	epar_bits[10] = (options_group.selected_index_value() >> 1) & 1;

	if (checkbox_ra.value())
		epar_bits[11] = 1;	// Bit 11 is relay 1 state
	else
		epar_bits[11] = 0;

	if (checkbox_rb.value())
		epar_bits[12] = 1;	// Bit 12 is relay 2 state
	else
		epar_bits[12] = 0;

	// DEBUG
	char debug_binary[14];
	for (c = 0; c < 13; c++) 
		debug_binary[c] = 0x30 + epar_bits[c];
	debug_binary[13] = 0;

	text_debug.set(debug_binary);
}

void EPARView::journuit() {
	chThdSleepMilliseconds(1000);
	
	// Invert relay states
	checkbox_ra.set_value(~checkbox_ra.value());
	checkbox_rb.set_value(~checkbox_rb.value());
	
	update_message();
	
	shared_memory.transmit_done = false;
	memcpy(shared_memory.epardata, epar_bits, 13);
	transmitter_model.enable();
}

void EPARView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

EPARView::EPARView(
	NavigationView& nav
)
{
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	
	static constexpr Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	
	transmitter_model.set_baseband_configuration({
		.mode = 4,
		.sampling_rate = 1536000,
		.decimation_factor = 1,
	});

	add_children({ {
		&text_city,
		&city_code,
		&text_group,
		&options_group,
		&checkbox_ra,
		&checkbox_rb,
		&excur,
		&text_freq,
		//&options_freq,
		&field_frequency,
		&progressbar,
		&text_debug,
		&button_transmit,
		&checkbox_cligno,
		&button_exit
	} });
	
	city_code.set_value(220);
	options_group.set_selected_index(3);	// TP
	//options_freq.set_selected_index(6);		// 5 ! DEBUG
	
	checkbox_ra.set_value(true);
	checkbox_rb.set_value(true);
	
	excur.set_value(500);
	shared_memory.excursion = 500;
	excur.on_change = [this](int32_t v) {
		(void)v;
		shared_memory.excursion = excur.value();
	};
	
	field_frequency.set_value(31387500);	// 31.3805 receiver_model.tuning_frequency()
	field_frequency.set_step(500);
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

	city_code.on_change = [this](int32_t v) {
		(void)v;
		EPARView::update_message();
	};
	options_group.on_change = [this](size_t n, OptionsField::value_t v) {
		(void)n;
		(void)v;
		EPARView::update_message();
	};
	checkbox_ra.on_select = [this](Checkbox&) {
		EPARView::update_message();
	};
	checkbox_rb.on_select = [this](Checkbox&) {
		EPARView::update_message();
	};
	
	button_transmit.set_style(&style_val);
	
	EPARView::update_message();

	button_transmit.on_select = [this](Button&) {
		if (txing == false) {
			update_message();
			
			EventDispatcher::message_map().unregister_handler(Message::ID::TXDone);
			
			EventDispatcher::message_map().register_handler(Message::ID::TXDone,
				[this](Message* const p) {
					const auto message = static_cast<const TXDoneMessage*>(p);
					if (message->n == 100) {
						transmitter_model.disable();
						progressbar.set_value(0);
						if (checkbox_cligno.value() == false) {
							txing = false;
							button_transmit.set_style(&style_val);
							button_transmit.set_text("START");
						} else {
							journuit();
						}
					} else {
						progressbar.set_value(message->n * 2);	// 100/52
					}
				}
			);
			
			shared_memory.transmit_done = false;
			memcpy(shared_memory.epardata, epar_bits, 13);

			transmitter_model.set_tuning_frequency(field_frequency.value());
			//transmitter_model.set_tuning_frequency(epar_freqs[options_freq.selected_index()]);

			txing = true;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			transmitter_model.enable();
		}
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
}

} /* namespace ui */
