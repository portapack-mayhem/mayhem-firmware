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

#include "ui_lcr.hpp"
#include "ui_modemsetup.hpp"

#include "lcr.hpp"
#include "modems.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include "serializer.hpp"

using namespace portapack;

namespace ui {

void LCRView::focus() {
	button_set_rgsb.focus();
}

LCRView::~LCRView() {
	// save app settings
	app_settings.tx_frequency = transmitter_model.tuning_frequency();	
	settings.save("tx_lcr", &app_settings);

	transmitter_model.disable();
	baseband::shutdown();
}

/*
// Recap: frequency @ baudrate
final_str = to_string_short_freq(persistent_memory::tuned_frequency());
final_str += '@';
final_str += to_string_dec_int(persistent_memory::modem_baudrate(), 4);
final_str += "bps ";
//final_str += modem_defs[persistent_memory::modem_def_index()].name;
text_recap.set(final_str);*/

void LCRView::update_progress() {
	if (tx_mode == IDLE) {
		text_status.set("Ready");
		progress.set_value(0);
	} else {
		std::string progress_str = to_string_dec_uint(repeat_index) + "/" + to_string_dec_uint(persistent_memory::modem_repeat()) +
			" " + to_string_dec_uint(scan_index + 1) + "/" + to_string_dec_uint(scan_count);
			
		text_status.set(progress_str);
		
		if (tx_mode == SINGLE)
			progress.set_value(repeat_index);
		else if (tx_mode == SCAN)
			progress.set_value(scan_progress);
	}
}

void LCRView::on_tx_progress(const uint32_t progress, const bool done) {
	if (!done) {
		// Repeating...
		repeat_index = progress + 1;
		
		if (tx_mode == SCAN)
			scan_progress++;
	} else {
		// Done transmitting
		tx_view.set_transmitting(false);
		transmitter_model.disable();
		
		if ((tx_mode == SCAN) && (scan_index < (scan_count - 1))) {
			// Next address
			scan_index++;
			scan_progress++;
			repeat_index = 1;
			start_tx(true);
		} else {
			tx_mode = IDLE;
		}
	}
	
	update_progress();
}

void LCRView::start_tx(const bool scan) {
	uint32_t repeats = persistent_memory::modem_repeat();
	
	if (scan) {
		if (tx_mode != SCAN) {
			scan_index = 0;
			scan_count = scan_list[options_scanlist.selected_index()].count;
			scan_progress = 1;
			repeat_index = 1;
			tx_mode = SCAN;
			progress.set_max(scan_count * repeats);
			update_progress();
		}
		rgsb = scan_list[options_scanlist.selected_index()].addresses[scan_index];
		button_set_rgsb.set_text(rgsb);
	} else {
		tx_mode = SINGLE;
		repeat_index = 1;
		scan_count = 1;
		scan_index = 0;
		progress.set_max(repeats);
		update_progress();
	}
	
	std::vector<std::string> litterals_list;
	
	for (size_t i = 0; i < LCR_MAX_AM; i++) {
		if (checkboxes[i].value())
			litterals_list.push_back(litteral[i]);
	}
	
	modems::generate_data(lcr::generate_message(rgsb, litterals_list, options_ec.selected_index()), lcr_message_data);

	transmitter_model.set_tuning_frequency(persistent_memory::tuned_frequency());
	transmitter_model.set_sampling_rate(AFSK_TX_SAMPLERATE);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();

	memcpy(shared_memory.bb_data.data, lcr_message_data, sizeof(lcr_message_data));
	
	baseband::set_afsk_data(
		AFSK_TX_SAMPLERATE / persistent_memory::modem_baudrate(),
		persistent_memory::afsk_mark_freq(),
		persistent_memory::afsk_space_freq(),
		repeats,
		transmitter_model.channel_bandwidth(),
		serializer::symbol_count(persistent_memory::serial_format())
	);
}

void LCRView::on_button_set_am(NavigationView& nav, int16_t button_id) {
	text_prompt(
		nav,
		litteral[button_id],
		7,
		[this, button_id](std::string& buffer) {
			texts[button_id].set(buffer);
		});
}

LCRView::LCRView(NavigationView& nav) {
	std::string label;
	
	baseband::run_image(portapack::spi_flash::image_tag_afsk);
	
	add_children({
		&labels,
		&options_ec,
		&button_set_rgsb,
		&button_modem_setup,
		&text_status,
		&progress,
		&options_scanlist,
		&check_scan,
		&button_clear,
		&tx_view
	});
	
	// load app settings
	auto rc = settings.load("tx_lcr", &app_settings);
	if(rc == SETTINGS_OK) {
		transmitter_model.set_rf_amp(app_settings.tx_amp);
		transmitter_model.set_channel_bandwidth(app_settings.channel_bandwidth);
		transmitter_model.set_tuning_frequency(app_settings.tx_frequency);
		transmitter_model.set_tx_gain(app_settings.tx_gain);		
	}

	options_scanlist.set_selected_index(0);
	
	const auto button_set_am_fn = [this, &nav](Button& button) {
		on_button_set_am(nav, button.id);
	};
	
	for (size_t n = 0; n < LCR_MAX_AM; n++) {
		Button* button = &buttons[n];
		button->on_select = button_set_am_fn;
		button->id = n;
		button->set_text("AM " + to_string_dec_uint(n + 1));
		button->set_parent_rect({
			static_cast<Coord>(40),
			static_cast<Coord>(n * 32 + 64),
			48, 24
		});
		add_child(button);
		
		Checkbox* checkbox = &checkboxes[n];
		checkbox->set_parent_rect({
			static_cast<Coord>(8),
			static_cast<Coord>(n * 32 + 64),
			48, 24
		});
		checkbox->set_value(false);
		add_child(checkbox);
		
		Rectangle* rectangle = &rectangles[n];
		rectangle->set_parent_rect({
			static_cast<Coord>(98),
			static_cast<Coord>(n * 32 + 66),
			68, 20
		});
		rectangle->set_color(ui::Color::grey());
		rectangle->set_outline(true);
		add_child(rectangle);
		
		Text* text = &texts[n];
		text->set_parent_rect({
			static_cast<Coord>(104),
			static_cast<Coord>(n * 32 + 68),
			7 * 8, 16
		});
		add_child(text);
	}
	
	button_set_rgsb.set_text(rgsb);
	options_ec.set_selected_index(0);	// Auto
	checkboxes[0].set_value(true);
	
	button_set_rgsb.on_select = [this, &nav](Button&) {
		text_prompt(
			nav,
			rgsb,
			4,
			[this](std::string& buffer) {
				button_set_rgsb.set_text(buffer);
			});
	};
	
	button_modem_setup.on_select = [this, &nav](Button&) {
		if (tx_mode == IDLE)
			nav.push<ModemSetupView>();
	};

	button_clear.on_select = [this, &nav](Button&) {
		options_ec.set_selected_index(0);	// Auto
		for (size_t n = 0; n < LCR_MAX_AM; n++) {
			litteral[n] = "       ";
			checkboxes[n].set_value(true);
		}
	};
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			transmitter_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (check_scan.value()) {
			if (tx_mode == IDLE) {
				start_tx(true);
				tx_view.set_transmitting(true);
			} else {
				// Kill scan process
				baseband::kill_afsk();
				tx_view.set_transmitting(false);
				transmitter_model.disable();
				text_status.set("Abort @" + rgsb);
				progress.set_value(0);
				tx_mode = IDLE;
			}
		} else {
			if (tx_mode == IDLE) {
				start_tx(false);
				tx_view.set_transmitting(true);
			}
		}
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
		tx_mode = IDLE;
	};
}

} /* namespace ui */
