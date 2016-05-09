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

#include "ui_soundboard.hpp"

#include "ch.h"

#include "ui_alphanum.hpp"
#include "ff.h"
#include "hackrf_gpio.hpp"
#include "portapack.hpp"
#include "radio.hpp"
#include "event_m0.hpp"
#include "string_format.hpp"

#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;

namespace ui {
	
void SoundBoardView::on_show() {
	/*
	// Just in case
	EventDispatcher::message_map().unregister_handler(Message::ID::DisplayFrameSync);
	
	// "Vertical blank interrupt"
	EventDispatcher::message_map().register_handler(Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			pbar_test.set_value(testv/4);
			testv++;
		}
	);*/
}

std::string SoundBoardView::title() const {
	return "Sound board";
};

void SoundBoardView::focus() {
	buttons[0].focus();
}

void SoundBoardView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void SoundBoardView::on_button(Button& button) {
	text_test.set(to_string_dec_uint(button.id));
}

SoundBoardView::SoundBoardView(
	NavigationView& nav
)
{
	size_t n;
	
	for (n = 0; n < 12; n++) {
		sounds[n].filename = "";
		sounds[n].shortname = "Empty";
		sounds[n].min = 0;
		sounds[n].sec = 0;
	}
	
	add_children({ {
		&text_test,
		&field_frequency,
		&number_bw,
		&pbar_test,
		&button_load,
		&button_exit
	} });

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	for(auto& button : buttons) {
		add_child(&button);
		button.id = n;
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>((n % 3) * 70 + 15),
			static_cast<Coord>((n / 3) * 50 + 30),
			70, 50
		});
		button.set_text(sounds[n].shortname);
		n++;
	}

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
	
	/*button_transmit.on_select = [](Button&){
		transmitter_model.set_baseband_configuration({
			.mode = 1,
			.sampling_rate = 1536000,
			.decimation_factor = 1,
		});
		transmitter_model.set_rf_amp(true);
		transmitter_model.enable();
	};*/

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

SoundBoardView::~SoundBoardView() {
	transmitter_model.disable();
}

}
