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

#include "ui_bht_tx.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void BHTView::focus() {
	relay_states[0].focus();
}

BHTView::~BHTView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void BHTView::generate_message() {
	if (!_mode) {
		text_message.set(
			gen_message_xy(header_code_a.value(), header_code_b.value(), city_code_xy.value(), family_code_xy.value(), 
							checkbox_wcsubfamily.value(), subfamily_code.value(), checkbox_wcid.value(), receiver_code.value(),
							relay_states[0].selected_index(), relay_states[1].selected_index(), 
							relay_states[2].selected_index(), relay_states[3].selected_index())
		);
	} else {
		text_message.set(
			gen_message_ep(city_code_ep.value(), family_code_ep.selected_index_value(),
							relay_states[0].selected_index(), relay_states[1].selected_index())
		);
	}
}

void BHTView::start_tx() {
	if (speaker_enabled && !_mode)
		audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);
	
	generate_message();
	
	transmitter_model.set_tuning_frequency(bht_freqs[options_freq.selected_index()]);
	transmitter_model.set_sampling_rate(1536000);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	// Setup for Xy
	for (uint8_t c = 0; c < 16; c++) {
		shared_memory.bb_data.tones_data.tone_defs[c].delta = ccir_deltas[c];
		shared_memory.bb_data.tones_data.tone_defs[c].duration = CCIR_TONE_LENGTH;
	}
	
	audio::set_rate(audio::Rate::Hz_24000);
	baseband::set_tones_data(field_bw.value(), CCIR_SILENCE, 20, false, checkbox_speaker.value());
}

void BHTView::on_tx_progress(const int progress, const bool done) {
	size_t c;
	uint8_t sr;
	
	if (tx_mode == SINGLE) {
		if (done) {
			audio::headphone::set_volume(volume_t::decibel(0 - 99) + audio::headphone::volume_range().max);
			
			transmitter_model.disable();
			progressbar.set_value(0);
			
			if (!checkbox_cligno.value()) {
				tx_mode = IDLE;
				button_transmit.set_style(&style_val);
				button_transmit.set_text("START");
			} else {
				chThdSleepMilliseconds(tempo_cligno.value() * 1000);	// Dirty :(
				
				// Invert all relay states
				for (c = 0; c < 1; c++) {
					sr = relay_states[c].selected_index();
					if (sr > 0) relay_states[c].set_selected_index(sr ^ 3);
				}
				
				generate_message();
				start_tx();
			}
		} else {
			progressbar.set_value(progress);
		}
	}
}

BHTView::BHTView(NavigationView& nav) {
	(void)nav;
	size_t n;
	
	baseband::run_image(portapack::spi_flash::image_tag_tones);
	//baseband::run_image(portapack::spi_flash::image_tag_encoders);

	add_children({
		&options_mode,
		&text_header,
		&header_code_a,
		&header_code_b,
		&checkbox_speaker,
		&bmp_speaker,
		&text_city,
		&city_code_xy,
		&text_family,
		&family_code_xy,
		&text_subfamily,
		&subfamily_code,
		&checkbox_wcsubfamily,
		&text_receiver,
		&receiver_code,
		&checkbox_wcid,
		&text_freq,
		&options_freq,
		&field_bw,
		&text_relais,
		&progressbar,
		&text_message,
		&button_transmit,
		&checkbox_cligno,
		&tempo_cligno,
		&text_cligno
	});
	
	options_mode.set_selected_index(0);			// Start up in Xy mode
	header_code_a.set_value(0);
	header_code_b.set_value(0);
	city_code_xy.set_value(10);
	city_code_ep.set_value(220);
	family_code_xy.set_value(1);
	family_code_ep.set_selected_index(2);
	subfamily_code.set_value(1);
	receiver_code.set_value(1);
	options_freq.set_selected_index(0);
	tempo_cligno.set_value(1);
	progressbar.set_max(20);
	relay_states[0].set_selected_index(1);		// R1 OFF
	
	field_bw.set_value(10);
	
	options_mode.on_change = [this](size_t mode, OptionsField::value_t) {
		_mode = mode;
		
		if (_mode) {
			// EP layout
			remove_children({
				&text_header,
				&header_code_a,
				&header_code_b,
				&checkbox_speaker,
				&bmp_speaker,
				&city_code_xy,
				&family_code_xy,
				&text_subfamily,
				&subfamily_code,
				&checkbox_wcsubfamily,
				&text_receiver,
				&receiver_code,
				&checkbox_wcid,
				&relay_states[2],
				&relay_states[3]
			});
			add_children({
				&city_code_ep,
				&family_code_ep
			});
			set_dirty();
		} else {
			// Xy layout
			remove_children({
				&city_code_ep,
				&family_code_ep
			});
			add_children({
				&text_header,
				&header_code_a,
				&header_code_b,
				&checkbox_speaker,
				&bmp_speaker,
				&city_code_xy,
				&family_code_xy,
				&text_subfamily,
				&subfamily_code,
				&checkbox_wcsubfamily,
				&text_receiver,
				&receiver_code,
				&checkbox_wcid,
				&relay_states[2],
				&relay_states[3]
			});
			set_dirty();
		};
		generate_message();
	};
	
	checkbox_speaker.on_select = [this](Checkbox&, bool v) {
		speaker_enabled = v;
	};
	
	header_code_a.on_change = [this](int32_t) {
		generate_message();
	};
	header_code_b.on_change = [this](int32_t) {
		generate_message();
	};
	city_code_xy.on_change = [this](int32_t) {
		generate_message();
	};
	family_code_xy.on_change = [this](int32_t) {
		generate_message();
	};
	subfamily_code.on_change = [this](int32_t) {
		generate_message();
	};
	receiver_code.on_change = [this](int32_t) {
		generate_message();
	};
	
	checkbox_wcsubfamily.on_select = [this](Checkbox&, bool v) {
		if (v) {
			subfamily_code.set_focusable(false);
			subfamily_code.set_style(&style_grey);
			text_subfamily.set_style(&style_grey);
		} else {
			subfamily_code.set_focusable(true);
			subfamily_code.set_style(&style());
			text_subfamily.set_style(&style());
		}
		generate_message();
	};
	
	checkbox_wcid.on_select = [this](Checkbox&, bool v) {
		if (v) {
			receiver_code.set_focusable(false);
			receiver_code.set_style(&style_grey);
			text_receiver.set_style(&style_grey);
		} else {
			receiver_code.set_focusable(true);
			receiver_code.set_style(&style());
			text_receiver.set_style(&style());
		}
		generate_message();
	};
	
	checkbox_wcsubfamily.set_value(true);
	checkbox_wcid.set_value(true);
	
	const auto relay_state_fn = [this](size_t, OptionsField::value_t) {
		this->generate_message();
	};
	
	n = 0;
	for (auto& relay_state : relay_states) {
		relay_state.on_change = relay_state_fn;
		relay_state.set_parent_rect({
			static_cast<Coord>(26 + (n * 53)),
			174,
			24, 24
		});
		relay_state.set_options(relay_options);
		add_child(&relay_state);
		n++;
	}
	
	button_transmit.set_style(&style_val);
	
	generate_message();

	button_transmit.on_select = [this, &nav](Button&) {
		if ((tx_mode == IDLE) && (!_mode)) {	// DEBUG
			if (speaker_enabled)
				chThdSleepMilliseconds(40 * 1000);	// DEBUG 40s
			//if (speaker_enabled && _mode)
			//	audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);
			tx_mode = SINGLE;
			button_transmit.set_style(&style_cancel);
			button_transmit.set_text("Wait");
			start_tx();
		}
	};
}

} /* namespace ui */
