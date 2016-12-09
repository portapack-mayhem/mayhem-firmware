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

#include "portapack.hpp"
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
	size_t c;
	const encoder_def_t * um3750_def;
	uint8_t bit[12];
	uint8_t city_code;
	std::string ep_symbols;
	char ook_bitstream[256];
	char ep_message[13] = { 0 };
	
	if (!_mode) {
		// Xy CCIR frame

		// Header
		ccir_message[0] = (header_code_a.value() / 10) + '0';
		ccir_message[1] = (header_code_a.value() % 10) + '0';
		ccir_message[2] = (header_code_b.value() / 10) + '0';
		ccir_message[3] = (header_code_b.value() % 10) + '0';
		
		// Addresses
		ccir_message[4] = (city_code_xy.value() / 10) + '0';
		ccir_message[5] = (city_code_xy.value() % 10) + '0';
		ccir_message[6] = family_code_xy.value() + '0';
		
		if (!checkbox_wcsubfamily.value())
			ccir_message[7] = subfamily_code.value() + '0';
		else
			ccir_message[7] = 'A';		// Wildcard
		
		if (!checkbox_wcid.value()) {
			ccir_message[8] = (receiver_code.value() / 10) + '0';
			ccir_message[9] = (receiver_code.value() % 10) + '0';
		} else {
			ccir_message[8] = 'A';		// Wildcard
			ccir_message[9] = 'A';		// Wildcard
		}
		
		ccir_message[10] = 'B';
		
		// Relay states
		for (c = 0; c < 4; c++)
			ccir_message[c + 11] = relay_states[c].selected_index() + '0';
		
		ccir_message[15] = 'B';
		
		// End
		for (c = 16; c < 20; c++)
			ccir_message[c] = '0';
		
		ccir_message[20] = 0;
		
		// Replace repeats with E code
		for (c = 1; c < 20; c++)
			if (ccir_message[c] == ccir_message[c - 1]) ccir_message[c] = 'E';
		
		// Display as text
		text_message.set(ccir_message);
		
		ascii_to_ccir(ccir_message);
	} else {
		// EP frame
		// Repeated 2x 26 times
		// Whole frame + space = 128ms, data only = 64ms
		
		um3750_def = &encoder_defs[8];

		city_code = city_code_ep.value();

		for (c = 0; c < 8; c++)
			bit[c] = (city_code >> c) & 1;
		
		bit[8] = family_code_ep.selected_index_value() >> 1;
		bit[9] = family_code_ep.selected_index_value() & 1;
		bit[10] = 0;		// R1 first
		if (relay_states[0].selected_index())
			bit[11] = relay_states[0].selected_index() - 1;
		else
			bit[11] = 0;
		
		for (c = 0; c < 12; c++)
			ep_message[c] = bit[c] + '0';
			
		text_message.set(ep_message);

		c = 0;
		for (auto ch : um3750_def->word_format) {
			if (ch == 'S')
				ep_symbols += um3750_def->sync;
			else
				ep_symbols += um3750_def->bit_format[bit[c++]];
		}
		
		c = 0;
		for (auto ch : ep_symbols) {
			if (ch != '0')
				ook_bitstream[c >> 3] |= (1 << (7 - (c & 7)));
			c++;
		}
	}
}

void BHTView::start_tx() {
	
	if (speaker_enabled && !_mode)
		audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);

	transmitter_model.set_tuning_frequency(bht_freqs[options_freq.selected_index()]);
	transmitter_model.set_baseband_configuration({
		.mode = 0,
		.sampling_rate = 1536000U,
		.decimation_factor = 1,
	});
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	memcpy(shared_memory.bb_data.tones_data.message, ccir_message, 20);
	
	for (uint8_t c = 0; c < 16; c++) {
		shared_memory.bb_data.tones_data.tone_defs[c].delta = ccir_deltas[c];
		shared_memory.bb_data.tones_data.tone_defs[c].duration = CCIR_TONE_LENGTH;
	}
	
	audio::set_rate(audio::Rate::Hz_24000);
	baseband::set_tones_data(10000, CCIR_SILENCE, 20, false, checkbox_speaker.value());
}

// ASCII to frequency LUT index
void BHTView::ascii_to_ccir(char * ascii) {
	for (size_t c = 0; c < 20; c++) {
		if (ascii[c] > '9')
			ascii[c] -= 0x37;
		else
			ascii[c] -= '0';
	}
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
				for (c = 0; c < 4; c++) {
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

	add_children({ {
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
		&text_relais,
		&progressbar,
		&text_message,
		&button_transmit,
		&checkbox_cligno,
		&tempo_cligno,
		&text_cligno
	} });
	
	options_mode.set_selected_index(0);			// Xy
	header_code_a.set_value(0);
	header_code_b.set_value(0);
	city_code_xy.set_value(18);
	family_code_xy.set_value(1);
	subfamily_code.set_value(1);
	receiver_code.set_value(1);
	options_freq.set_selected_index(0);
	tempo_cligno.set_value(1);
	progressbar.set_max(20);
	relay_states[0].set_selected_index(1);		// R1 OFF
	
	options_mode.on_change = [this](size_t mode, OptionsField::value_t) {
		_mode = mode;
		
		if (_mode) {
			// EP layout
			remove_children({ {
				&text_header,
				&header_code_a,
				&header_code_b,
				&checkbox_speaker,
				&bmp_speaker,
				&city_code_xs,
				&family_code_xy,
				&text_subfamily,
				&subfamily_code,
				&checkbox_wcsubfamily,
				&text_receiver,
				&receiver_code,
				&checkbox_wcid,
				&relay_states[2],
				&relay_states[3]
			} });
			add_children({ {
				&city_code_ep,
				&family_code_ep
			} });
			set_dirty();
		} else {
			// Xy layout
			remove_children({ {
				&city_code_ep,
				&family_code_ep
			} });
			add_children({ {
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
			} });
			set_dirty();
		};
		generate_message();
	};
	
	checkbox_speaker.on_select = [this](Checkbox&) {
		speaker_enabled = checkbox_speaker.value();
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
	
	checkbox_wcsubfamily.on_select = [this](Checkbox&) {
		if (checkbox_wcsubfamily.value()) {
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
	
	checkbox_wcid.on_select = [this](Checkbox&) {
		if (checkbox_wcid.value()) {
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
		if (tx_mode == IDLE) {
			//auto modal_view = nav.push<ModalMessageView>("TX", "TX ?", true);
			//modal_view->on_choice = [this](bool choice) {
			//	if (choice) {
					if (speaker_enabled && _mode)
						audio::headphone::set_volume(volume_t::decibel(90 - 99) + audio::headphone::volume_range().max);
					tx_mode = SINGLE;
					button_transmit.set_style(&style_cancel);
					button_transmit.set_text("Wait");
					generate_message();
					start_tx();
			//	}
			//};
		}
	};
}

} /* namespace ui */
