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
#include "portapack.hpp"
#include "lfsr_random.hpp"
#include "string_format.hpp"

#include "portapack_shared_memory.hpp"

#include <cstring>
#include <vector>

using namespace portapack;

namespace ui {

void NuoptixView::focus() {
	button_tx.focus();
}

NuoptixView::~NuoptixView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void NuoptixView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void NuoptixView::transmit(bool setup) {
	uint8_t mod, tone_code;
	uint8_t c;
	uint8_t dtmf_message[6];
	
	if (!tx_mode) {
		transmitter_model.disable();
		return;
	}
	
	if (tx_mode == IMPROVISE)
		timecode = lfsr_iterate(timecode) % 1999;	// Could be 9999 but that would be one long audio track !
	
	if (setup) {
		pbar.set_max(4);
		
		if (tx_mode == NORMAL)
			timecode = number_timecode.value();
		else
			timecode = 0125;	// TODO: Use RTC as seed ?
		
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
		
		dtmf_message[0] = '*';			// "Pre-tone for restart" method #1
		dtmf_message[1] = 'A';			// "Restart" method #1
	} else {
		dtmf_message[0] = '#';
		dtmf_message[1] = (timecode / 1000) % 10;
		chThdSleepMilliseconds(92);		// 141-49ms
		number_timecode.set_value(timecode);
	}
	
	pbar.set_value(0);
	
	dtmf_message[2] = (timecode / 100) % 10;
	dtmf_message[3] = (timecode / 10) % 10;
	dtmf_message[4] = timecode % 10;
	
	mod = 0;
	for (c = 1; c < 5; c++)
		if (dtmf_message[c] <= 9)
			mod += dtmf_message[c];
	
	mod = 10 - (mod % 10);
	if (mod == 10) mod = 0;		// Is this right ?
	
	text_mod.set("Mod: " + to_string_dec_uint(mod));
	
	dtmf_message[5] = mod;
	
	for (c = 0; c < 6; c++) {
		tone_code = dtmf_message[c];
		
		if (tone_code == 'A')
			tone_code = 10;
		else if (tone_code == 'B')
			tone_code = 11;
		else if (tone_code == 'C')
			tone_code = 12;
		else if (tone_code == 'D')
			tone_code = 13;
		else if (tone_code == '#')
			tone_code = 14;
		else if (tone_code == '*')
			tone_code = 15;
		
		shared_memory.bb_data.tones_data.message[c * 2] = tone_code;
		shared_memory.bb_data.tones_data.message[c * 2 + 1] = 0xFF;		// Silence
	}
	
	for (c = 0; c < 16; c++) {
		shared_memory.bb_data.tones_data.tone_defs[c * 2].delta = dtmf_deltas[c][0];
		shared_memory.bb_data.tones_data.tone_defs[c * 2].duration = NUOPTIX_TONE_LENGTH;
		shared_memory.bb_data.tones_data.tone_defs[c * 2 + 1].delta = dtmf_deltas[c][1];
		shared_memory.bb_data.tones_data.tone_defs[c * 2 + 1].duration = NUOPTIX_TONE_LENGTH;
	}
	shared_memory.bb_data.tones_data.silence = NUOPTIX_TONE_LENGTH;		// 49ms tone, 49ms space
	
	audio::set_rate(audio::Rate::Hz_24000);
	baseband::set_tones_data(number_bw.value(), 0, 6 * 2, true, true);	
	
	timecode++;
}

NuoptixView::NuoptixView(
	NavigationView& nav
)
{
	baseband::run_image(portapack::spi_flash::image_tag_tones);

	add_children({ {
		&field_frequency,
		&number_bw,
		&text_kHz,
		&number_timecode,
		&text_timecode,
		&text_mod,
		&pbar,
		&button_tx,
		&button_impro,
		&button_exit
	} });
	
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
		if (tx_mode == NORMAL) {
			tx_mode = IDLE;
			button_tx.set_text("TX");
		} else if (tx_mode == IDLE) {
			tx_mode = NORMAL;
			button_tx.set_text("STOP");
			transmit(true);
		}
	};
	
	button_impro.on_select = [this](Button&){
		if (tx_mode == IMPROVISE) {
			tx_mode = IDLE;
			button_impro.set_text("IMPROVISE");
		} else if (tx_mode == IDLE) {
			tx_mode = IMPROVISE;
			button_impro.set_text("STOP");
			transmit(true);
		}
	};
	
	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
}

}
