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
#include "string_format.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

using namespace portapack;

namespace ui {

void BHTView::focus() {
	tx_view.focus();
}

void BHTView::start_tx() {
	baseband::shutdown();
	
	transmitter_model.set_baseband_bandwidth(1750000);
	
	if (target_system == XYLOS) {
		
		baseband::run_image(portapack::spi_flash::image_tag_tones);
		
		view_xylos.generate_message();
		
		//if (tx_mode == SINGLE) {
			progressbar.set_max(XY_TONE_COUNT);
		/*} else if (tx_mode == SCAN) {
			progressbar.set_max(XY_TONE_COUNT * view_xylos.get_scan_remaining());
		}*/
		
		transmitter_model.set_sampling_rate(TONES_SAMPLERATE);
		transmitter_model.enable();
		
		// Setup tones
		for (size_t c = 0; c < ccir_deltas.size(); c++)
			baseband::set_tone(c, ccir_deltas[c], XY_TONE_DURATION);
		
		baseband::set_tones_config(transmitter_model.channel_bandwidth(), XY_SILENCE, XY_TONE_COUNT, false, false);
		
	} else if (target_system == EPAR) {
		
		baseband::run_image(portapack::spi_flash::image_tag_ook);
		
		auto bitstream_length = view_EPAR.generate_message();
		
		//if (tx_mode == SINGLE) {
			progressbar.set_max(2 * EPAR_REPEAT_COUNT);
		/*} else if (tx_mode == SCAN) {
			progressbar.set_max(2 * EPAR_REPEAT_COUNT * view_EPAR.get_scan_remaining());
		}*/
		
		transmitter_model.set_sampling_rate(OOK_SAMPLERATE);
		transmitter_model.enable();

		baseband::set_ook_data(
			bitstream_length,
			EPAR_BIT_DURATION,
			EPAR_REPEAT_COUNT,
			encoder_defs[ENCODER_UM3750].pause_symbols
		);
	}
}

void BHTView::stop_tx() {
	transmitter_model.disable();
	baseband::shutdown();
	tx_mode = IDLE;
	tx_view.set_transmitting(false);
	progressbar.set_value(0);
}

void BHTView::on_tx_progress(const uint32_t progress, const bool done) {
	if (target_system == XYLOS) {
		if (done) {
			if (tx_mode == SINGLE) {
				if (checkbox_cligno.value()) {
					// TODO: Thread !
					chThdSleepMilliseconds(field_tempo.value() * 1000);	// Dirty :(
					view_xylos.flip_relays();
					start_tx();
				} else
					stop_tx();
			} else if (tx_mode == SCAN) {
				if (view_xylos.increment_address())
					start_tx();
				else
					stop_tx();	// Reached end of scan range
			}
		} else
			progressbar.set_value(progress);
	} else if (target_system == EPAR) {
		if (done) {
			
			if (!view_EPAR.half) {
				view_EPAR.half = true;
				start_tx();		// Start second half of transmission
			} else {
				view_EPAR.half = false;
				if (tx_mode == SINGLE) {
					if (checkbox_cligno.value()) {
						// TODO: Thread !
						chThdSleepMilliseconds(field_tempo.value() * 1000);	// Dirty :(
						view_EPAR.flip_relays();
						start_tx();
					} else
						stop_tx();
				} else if (tx_mode == SCAN) {
					if (view_EPAR.increment_address())
						start_tx();
					else
						stop_tx();	// Reached end of scan range
				}
			}
		} else
			progressbar.set_value(progress);
	}
}

BHTView::~BHTView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	settings.save("tx_bht", &app_settings);

	transmitter_model.disable();
}

BHTView::BHTView(NavigationView& nav) {
	add_children({
		&tab_view,
		&labels,
		&view_xylos,
		&view_EPAR,
		&checkbox_scan,
		&checkbox_cligno,
		&field_tempo,
		&progressbar,
		&tx_view
	});
	
	// load app settings
	auto rc = settings.load("tx_bht", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_channel_bandwidth(app_settings.channel_bandwidth);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}

	field_tempo.set_value(1);
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			transmitter_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (tx_mode == IDLE) {
			if (checkbox_scan.value()) {
				tx_mode = SCAN;
			} else {
				tx_mode = SINGLE;
			}
			
			progressbar.set_value(0);
			tx_view.set_transmitting(true);
			target_system = (target_system_t)tab_view.selected();
			view_EPAR.half = false;
			
			start_tx();
		}
	};
	
	tx_view.on_stop = [this]() {
		stop_tx();
	};
}

bool EPARView::increment_address() {
	auto city_code = field_city.value();
	
	if (city_code < EPAR_MAX_CITY) {
		field_city.set_value(city_code + 1);
		return true;
	} else
		return false;
}

uint32_t EPARView::get_scan_remaining() {
	return EPAR_MAX_CITY - field_city.value();
}

void EPARView::flip_relays() {
	// Invert first relay's state
	relay_states[0].set_selected_index(relay_states[0].selected_index() ^ 1);
}

size_t EPARView::generate_message() {
	// R2, then R1
	return gen_message_ep(field_city.value(), field_group.selected_index_value(),
							half ? 0 : 1, relay_states[half].selected_index());
}

EPARView::EPARView(
	Rect parent_rect
) : View(parent_rect) {
	
	hidden(true);
	
	add_children({
		&labels,
		&field_city,
		&field_group
	});

	field_city.set_value(0);
	field_group.set_selected_index(2);
	
	field_city.on_change = [this](int32_t) { generate_message(); };
	field_group.on_change = [this](size_t, int32_t) { generate_message(); };
	
	const auto relay_state_fn = [this](size_t, OptionsField::value_t) {
		generate_message();
	};
	
	size_t n = 0;
	for (auto& relay_state : relay_states) {
		relay_state.on_change = relay_state_fn;
		relay_state.set_parent_rect({
			static_cast<Coord>(90 + (n * 36)),
			80,
			24, 24
		});
		relay_state.set_options(relay_options);
		add_child(&relay_state);
		n++;
	}
}

void EPARView::focus() {
	field_city.focus();
}

void XylosView::flip_relays() {
	// Invert first relay's state if not ignored
	size_t rs = relay_states[0].selected_index();
	
	if (rs > 0)
		relay_states[0].set_selected_index(rs ^ 3);
}

bool XylosView::increment_address() {
	auto city_code = field_city.value();
	
	if (city_code < XY_MAX_CITY) {
		field_city.set_value(city_code + 1);
		return true;
	} else
		return false;
}

uint32_t XylosView::get_scan_remaining() {
	return XY_MAX_CITY - field_city.value();
}

void XylosView::generate_message() {
	gen_message_xy(field_header_a.value(), field_header_b.value(), field_city.value(), field_family.value(), 
					checkbox_wcsubfamily.value(), field_subfamily.value(), checkbox_wcid.value(), field_receiver.value(),
					relay_states[0].selected_index(), relay_states[1].selected_index(), 
					relay_states[2].selected_index(), relay_states[3].selected_index());
}

XylosView::XylosView(
	Rect parent_rect
) : View(parent_rect) {
	
	hidden(true);
	
	add_children({
		&labels,
		&field_header_a,
		&field_header_b,
		&field_city,
		&field_family,
		&field_subfamily,
		&checkbox_wcsubfamily,
		&field_receiver,
		&checkbox_wcid,
		//&button_seq,
	});

	field_header_a.set_value(0);
	field_header_b.set_value(0);
	field_city.set_value(10);
	field_family.set_value(1);
	field_subfamily.set_value(1);
	field_receiver.set_value(1);
	
	const auto field_fn = [this](int32_t) {
		generate_message();
	};
	
	field_header_a.on_change = field_fn;
	field_header_b.on_change = [this](int32_t) { generate_message(); };
	field_city.on_change = [this](int32_t) { generate_message(); };
	field_family.on_change = [this](int32_t) { generate_message(); };
	field_subfamily.on_change = [this](int32_t) { generate_message(); };
	field_receiver.on_change = [this](int32_t) { generate_message(); };
	
	checkbox_wcsubfamily.on_select = [this](Checkbox&, bool v) {
		field_subfamily.set_focusable(!v);
		generate_message();
	};
	
	checkbox_wcid.on_select = [this](Checkbox&, bool v) {
		field_receiver.set_focusable(!v);
		generate_message();
	};
	
	checkbox_wcsubfamily.set_value(true);
	checkbox_wcid.set_value(true);
	
	const auto relay_state_fn = [this](size_t, OptionsField::value_t) {
		generate_message();
	};
	
	size_t n = 0;
	for (auto& relay_state : relay_states) {
		relay_state.on_change = relay_state_fn;
		relay_state.set_parent_rect({
			static_cast<Coord>(54 + (n * 36)),
			134,
			24, 24
		});
		relay_state.set_options(relay_options);
		add_child(&relay_state);
		n++;
	}
}

void XylosView::focus() {
	field_city.focus();
}

} /* namespace ui */
