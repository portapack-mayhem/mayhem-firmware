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

#include "ui_morse.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
#include "hackrf_gpio.hpp"
#include "portapack_shared_memory.hpp"
#include "ui_textentry.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace morse;
using namespace hackrf::one;

namespace ui {

void MorseView::on_set_text(NavigationView& nav) {
	textentry(nav, buffer, 28);
}

void MorseView::focus() {
	tx_view.focus();
}

MorseView::~MorseView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void MorseView::paint(Painter&) {
	message = buffer;
	text_message.set(message);
}

static WORKING_AREA(ookthread_wa, 256);

static msg_t ookthread_fn(void * arg) {
	uint32_t v = 0, delay = 0;
	size_t i = 0;
	uint8_t * message = shared_memory.bb_data.tones_data.message;
	uint8_t symbol;
	MorseView * arg_c = (MorseView*)arg;
	
	chRegSetThreadName("ookthread");
	for (i = 0; i < arg_c->symbol_count; i++) {
		if (chThdShouldTerminate()) break;
		
		symbol = message[i];
		
		v = (symbol < 2) ? 1 : 0;
		delay = morse_symbols[v];
		
		gpio_tx.write(v);
		arg_c->on_tx_progress(i, false);
		
		chThdSleepMilliseconds(delay * arg_c->time_unit_ms);
	}
	
	gpio_tx.write(0);
	arg_c->on_tx_progress(0, true);
	chThdExit(0);
	
	return 0;
}

bool MorseView::start_tx() {
	if (checkbox_foxhunt.value())
		message = foxhunt_codes[options_foxhunt.selected_index_value()];
	
	time_unit_ms = field_time_unit.value();
	symbol_count = morse_encode(message, time_unit_ms, field_tone.value());
	
	if (!symbol_count) {
		nav_.display_modal("Error", "Message too long.", INFO, nullptr);
		return false;
	}
	
	progressbar.set_max(symbol_count);
	
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	if (options_modulation.selected_index() == 0) {
		ookthread = chThdCreateStatic(ookthread_wa, sizeof(ookthread_wa), NORMALPRIO + 10, ookthread_fn, this);
	} else {
		baseband::set_tones_data(transmitter_model.bandwidth(), 0, symbol_count, false, false);
	}
	
	return true;
}

void MorseView::on_tx_progress(const int progress, const bool done) {
	if (done) {
		transmitter_model.disable();
		progressbar.set_value(0);
		tx_view.set_transmitting(false);
	} else
		progressbar.set_value(progress);
}

MorseView::MorseView(
	NavigationView& nav
) : nav_ (nav)
{
	
	baseband::run_image(portapack::spi_flash::image_tag_tones);
	
	add_children({
		&labels,
		&checkbox_foxhunt,
		&options_foxhunt,
		&field_time_unit,
		&field_tone,
		&options_modulation,
		&text_message,
		&button_message,
		&progressbar,
		&tx_view
	});
	
	field_time_unit.set_value(50);				// 50ms
	field_tone.set_value(700);					// 700Hz
	options_modulation.set_selected_index(0);	// CW
	
	button_message.on_select = [this, &nav](Button&) {
		this->on_set_text(nav);
	};
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (start_tx())
			tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		chThdTerminate(ookthread);
		tx_view.set_transmitting(false);
	};

}

} /* namespace ui */
