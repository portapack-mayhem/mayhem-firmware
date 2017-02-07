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

#include "ui_pocsag_tx.hpp"

#include "pocsag.hpp"
#include "baseband_api.hpp"
#include "string_format.hpp"

#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>
#include <math.h>

using namespace portapack;
using namespace pocsag;

namespace ui {

void POCSAGTXView::focus() {
	tx_view.focus();
}

POCSAGTXView::~POCSAGTXView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void POCSAGTXView::on_tx_progress(const int progress, const bool done) {
	if (done) {
		transmitter_model.disable();
		progressbar.set_value(0);
		tx_view.set_transmitting(false);
	} else
		progressbar.set_value(progress);
}

void POCSAGTXView::start_tx() {
	uint32_t total_frames, i, codeword, bi, address;
	pocsag::BitRate bitrate;
	std::vector<uint32_t> codewords;
	
	address = address_field.value_dec_u32();
	if (address > 0x1FFFFFU)
		address = 0;			// Todo: Error screen
	
	pocsag_encode(BCH_code, message, address, codewords);
	
	total_frames = codewords.size() / 2;
	
	progressbar.set_max(total_frames);
	
	transmitter_model.set_sampling_rate(2280000);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	uint8_t * data_ptr = shared_memory.bb_data.data;
	
	bi = 0;
	for (i = 0; i < codewords.size(); i++) {
		codeword = codewords[i];
		data_ptr[bi++] = (codeword >> 24) & 0xFF;
		data_ptr[bi++] = (codeword >> 16) & 0xFF;
		data_ptr[bi++] = (codeword >> 8) & 0xFF;
		data_ptr[bi++] = codeword & 0xFF;
	}
	
	text_debug_a.set("Codewords: " + to_string_dec_uint(codewords.size()));
	
	bitrate = pocsag_bitrates[options_bitrate.selected_index()];
	
	baseband::set_fsk_data(
		codewords.size() * 32,
		2280000 / bitrate,
		4500,
		64
	);
}

void POCSAGTXView::paint(Painter&) {
	message = buffer;
	text_message.set("Message:" + message);
}

void POCSAGTXView::on_set_text(NavigationView& nav) {
	textentry(nav, buffer, 16);
}

POCSAGTXView::POCSAGTXView(NavigationView& nav) {

	baseband::run_image(portapack::spi_flash::image_tag_fsktx);

	add_children({
		&text_debug_a,
		&text_address,
		&address_field,
		&options_bitrate,
		&text_message,
		&button_message,
		&progressbar,
		&tx_view
	});

	options_bitrate.set_selected_index(1);	// 1200bps
	
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
		tx_view.set_transmitting(true);
		start_tx();
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
	};
	
}

} /* namespace ui */
