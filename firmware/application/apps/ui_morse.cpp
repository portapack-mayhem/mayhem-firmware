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
#include "string_format.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace morse;
using namespace hackrf::one;

namespace ui {

static WORKING_AREA(ookthread_wa, 256);

static msg_t ookthread_fn(void * arg) {
	uint32_t v = 0, delay = 0;
	uint8_t * message_symbols = shared_memory.bb_data.tones_data.message;
	uint8_t symbol;
	MorseView * arg_c = (MorseView*)arg;
	
	chRegSetThreadName("ookthread");
	
	for (uint32_t i = 0; i < arg_c->symbol_count; i++) {
		if (chThdShouldTerminate()) break;
		
		symbol = message_symbols[i];
		
		v = (symbol < 2) ? 1 : 0;	// TX on for dot or dash, off for pause
		delay = morse_symbols[symbol];
		
		gpio_tx.write(v);
		arg_c->on_tx_progress(i, false);
		
		chThdSleepMilliseconds(delay * arg_c->time_unit_ms);
	}
	
	gpio_tx.write(0);				// Ensure TX is off
	arg_c->on_tx_progress(0, true);
	chThdExit(0);
	
	return 0;
}

static msg_t loopthread_fn(void * arg) {
	MorseView * arg_c = (MorseView*)arg;
	uint32_t	wait = arg_c->loop;

	chRegSetThreadName("loopthread");

	for (uint32_t i = 0; i < wait; i++) {
		if (chThdShouldTerminate()) break;

		arg_c->on_loop_progress(i, false);
		chThdSleepMilliseconds(1000);
	}

	arg_c->on_loop_progress(0, true);
	chThdExit(0);
	
	return 0;
}

void MorseView::on_set_text(NavigationView& nav) {
	text_prompt(nav, buffer, 28);
}

void MorseView::focus() {
	button_message.focus();
}

MorseView::~MorseView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	settings.save("tx_morse", &app_settings);

	transmitter_model.disable();
	baseband::shutdown();
}

void MorseView::paint(Painter&) {
	message = buffer;
	text_message.set(message);
	update_tx_duration();
}

bool MorseView::start_tx() {
	// Re-generate message, just in case
	update_tx_duration();
	
	if (!symbol_count) {
		nav_.display_modal("Error", "Message too long,\nmust be < 256 symbols.", INFO, nullptr);
		return false;
	}
	
	progressbar.set_max(symbol_count);
	
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	if (modulation == CW) {
		ookthread = chThdCreateStatic(ookthread_wa, sizeof(ookthread_wa), NORMALPRIO + 10, ookthread_fn, this);
	} else if (modulation == FM) {
		baseband::set_tones_config(transmitter_model.channel_bandwidth(), 0, symbol_count, false, false);
	}
	
	return true;
}

void MorseView::update_tx_duration() {
	uint32_t duration_ms;
	
	time_unit_ms = 1200 / field_speed.value();
	symbol_count = morse_encode(message, time_unit_ms, field_tone.value(), &time_units);

	if (symbol_count) {
		duration_ms = time_units * time_unit_ms;
		text_tx_duration.set(to_string_dec_uint(duration_ms / 1000) + "." + to_string_dec_uint((duration_ms / 100) % 10, 1) + "s   ");
	} else {
		text_tx_duration.set("-");		// Error
	}
}

void MorseView::on_tx_progress(const uint32_t progress, const bool done) {
	if (done) {
		transmitter_model.disable();
		progressbar.set_value(0);

		if (loop && run) {
			text_tx_duration.set("wait");
			progressbar.set_max(loop);
			progressbar.set_value(0);
			
			if (loopthread) {
				chThdRelease(loopthread);
				loopthread = nullptr;
			}
			
			loopthread = chThdCreateFromHeap(NULL, 1024, NORMALPRIO, loopthread_fn, this);
		} else {
			tx_view.set_transmitting(false);
		}
	} else
		progressbar.set_value(progress);
}

void MorseView::on_loop_progress(const uint32_t progress, const bool done) {
	if (done) {
		start_tx();
	} else
		progressbar.set_value(progress);
}

void MorseView::set_foxhunt(size_t i) {
	message = foxhunt_codes[i];
	buffer = message.c_str();
	text_message.set(message);
	update_tx_duration();
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
		&field_speed,
		&field_tone,
		&options_modulation,
		&options_loop,
		&text_tx_duration,
		&text_message,
		&button_message,
		&progressbar,
		&tx_view
	});
	
	// load app settings
	auto rc = settings.load("tx_morse", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_channel_bandwidth(app_settings.channel_bandwidth);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}

	// Default settings
	field_speed.set_value(15);					// 15wps
	field_tone.set_value(700);					// 700Hz FM tone
	options_modulation.set_selected_index(0);	// CW mode
	options_loop.set_selected_index(0);			// Off
	
	checkbox_foxhunt.on_select = [this](Checkbox&, bool value) {
		foxhunt_mode = value;
		
		if (foxhunt_mode)
			set_foxhunt(options_foxhunt.selected_index_value());
	};
	
	options_foxhunt.on_change = [this](size_t i, int32_t) {
		if (foxhunt_mode)
			set_foxhunt(i);
	};
	
	options_modulation.on_change = [this](size_t i, int32_t) {
		modulation = (modulation_t)i;
	};

	options_loop.on_change = [this](size_t i, uint32_t n) {
		(void)i; //avoid unused warning
		loop = n;
	};
	
	field_speed.on_change = [this](int32_t) {
		update_tx_duration();
	};
	
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
		if (start_tx()) {
			run = true;
			tx_view.set_transmitting(true);
		}
	};
	
	tx_view.on_stop = [this]() {
		run = false;
		if (ookthread) chThdTerminate(ookthread);

		if (loopthread) {
			chThdTerminate(loopthread);
			chThdWait(loopthread);
			loopthread = nullptr;
		}

		transmitter_model.disable();
		baseband::kill_tone();
		tx_view.set_transmitting(false);
	};
}

} /* namespace ui */
