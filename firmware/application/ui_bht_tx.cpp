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
	if (tx_mode == SINGLE) {
		text_message.set(
			gen_message_xy(header_code_a.value(), header_code_b.value(), city_code_xy.value(), family_code_xy.value(), 
							checkbox_wcsubfamily.value(), subfamily_code.value(), checkbox_wcid.value(), receiver_code.value(),
							relay_states[0].selected_index(), relay_states[1].selected_index(), 
							relay_states[2].selected_index(), relay_states[3].selected_index())
		);
		/*} else {
			text_message.set(
				gen_message_ep(city_code_ep.value(), family_code_ep.selected_index_value(),
								relay_states[0].selected_index(), relay_states[1].selected_index())
			);*/
	} else if (tx_mode == SEQUENCE) {
		text_message.set(
			gen_message_xy(sequence_matin[seq_index].code)
		);
	}
}

void BHTView::start_tx() {
	uint8_t c;
	
	generate_message();
	
	if (tx_mode == SINGLE)
		progressbar.set_max(20);
	else if (tx_mode == SEQUENCE)
		progressbar.set_max(20 * XY_SEQ_COUNT);
	
	transmitter_model.set_sampling_rate(1536000);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	// Setup tones for Xy
	for (c = 0; c < 16; c++)
		baseband::set_tone(c, ccir_deltas[c], XY_TONE_LENGTH);
	
	baseband::set_tones_config(transmitter_model.bandwidth(), XY_SILENCE, 20, false, false);
}

void BHTView::on_tx_progress(const int progress, const bool done) {
	uint8_t c;
	uint8_t rs;
	
	if (tx_mode == SINGLE) {
		if (done) {
			transmitter_model.disable();
			progressbar.set_value(0);
			
			if (!checkbox_cligno.value()) {
				tx_mode = IDLE;
				tx_view.set_transmitting(false);
			} else {
				chThdSleepMilliseconds(tempo_cligno.value() * 1000);	// Dirty :(
				
				// Invert first relay's state
				rs = relay_states[0].selected_index();
				if (rs > 0) relay_states[0].set_selected_index(rs ^ 3);
				
				start_tx();
			}
		} else {
			progressbar.set_value(progress);
		}
	} else if (tx_mode == SEQUENCE) {
		if (done) {
			transmitter_model.disable();
			
			if (seq_index < (XY_SEQ_COUNT - 1)) {
				for (c = 0; c < sequence_matin[seq_index].delay; c++)
					chThdSleepMilliseconds(1000);
				
				seq_index++;
				
				start_tx();
			} else {
				progressbar.set_value(0);
				tx_mode = IDLE;
				tx_view.set_transmitting(false);
			}
		} else {
			progressbar.set_value((seq_index * 20) + progress);
		}
	}
}

BHTView::BHTView(NavigationView& nav) {
	size_t n;
	
	baseband::run_image(portapack::spi_flash::image_tag_tones);
	//baseband::run_image(portapack::spi_flash::image_tag_encoders);
	
	add_children({
		&labels,
		&options_mode,
		&header_code_a,
		&header_code_b,
		&city_code_xy,
		&family_code_xy,
		&subfamily_code,
		&checkbox_wcsubfamily,
		&receiver_code,
		&checkbox_wcid,
		&progressbar,
		&text_message,
		&checkbox_cligno,
		&tempo_cligno,
		&button_seq,
		&tx_view
	});
	
	options_mode.set_selected_index(0);			// Start up in Xy mode
	header_code_a.set_value(0);
	header_code_b.set_value(0);
	city_code_xy.set_value(10);
	//city_code_ep.set_value(220);
	family_code_xy.set_value(1);
	//family_code_ep.set_selected_index(2);
	subfamily_code.set_value(1);
	receiver_code.set_value(1);
	tempo_cligno.set_value(0);
	
/*	options_mode.on_change = [this](size_t mode, OptionsField::value_t) {
		_mode = mode;
		
		if (_mode) {
			// EP layout
			remove_children({
				&header_code_a,
				&header_code_b,
				&checkbox_speaker,
				&bmp_speaker,
				&city_code_xy,
				&family_code_xy,
				&subfamily_code,
				&checkbox_wcsubfamily,
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
				&header_code_a,
				&header_code_b,
				&checkbox_speaker,
				&bmp_speaker,
				&city_code_xy,
				&family_code_xy,
				&subfamily_code,
				&checkbox_wcsubfamily,
				&receiver_code,
				&checkbox_wcid,
				&relay_states[2],
				&relay_states[3]
			});
			set_dirty();
		};
		generate_message();
	};*/
	
	header_code_a.on_change = [this](int32_t) { generate_message(); };
	header_code_b.on_change = [this](int32_t) { generate_message(); };
	city_code_xy.on_change = [this](int32_t) { generate_message(); };
	family_code_xy.on_change = [this](int32_t) { generate_message(); };
	subfamily_code.on_change = [this](int32_t) { generate_message(); };
	receiver_code.on_change = [this](int32_t) { generate_message(); };
	
	checkbox_wcsubfamily.on_select = [this](Checkbox&, bool v) {
		if (v)
			subfamily_code.set_focusable(false);
		else
			subfamily_code.set_focusable(true);
		generate_message();
	};
	
	checkbox_wcid.on_select = [this](Checkbox&, bool v) {
		if (v)
			receiver_code.set_focusable(false);
		else
			receiver_code.set_focusable(true);
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
			static_cast<Coord>(4 + (n * 36)),
			150,
			24, 24
		});
		relay_state.set_options(relay_options);
		add_child(&relay_state);
		n++;
	}
	
	generate_message();
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	button_seq.on_select = [this, &nav](Button&) {
		if (tx_mode == IDLE) {
			seq_index = 0;
			tx_mode = SEQUENCE;
			tx_view.set_transmitting(true);
			start_tx();
		}
	};
	
	tx_view.on_start = [this]() {
		if (tx_mode == IDLE) {
			tx_mode = SINGLE;
			tx_view.set_transmitting(true);
			start_tx();
		}
	};
	
	tx_view.on_stop = [this]() {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
		tx_mode = IDLE;
	};
}

} /* namespace ui */
