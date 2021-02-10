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

#include "ui_coasterp.hpp"

#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void CoasterPagerView::focus() {
	sym_data.focus();
}

CoasterPagerView::~CoasterPagerView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void CoasterPagerView::generate_frame() {
	uint8_t frame[19];
	uint32_t c;
	
	// Preamble (8 bytes)
	for (c = 0; c < 8; c++)
		frame[c] = 0x55;		// Isn't this 0xAA ?
	
	// Sync word
	frame[8] = 0x2D;
	frame[9] = 0xD4;
	
	// Data length
	frame[10] = 8;
	
	// Data
	for (c = 0; c < 8; c++)
		frame[c + 11] = (sym_data.get_sym(c * 2) << 4) | sym_data.get_sym(c * 2 + 1);

	// Copy for baseband
	memcpy(shared_memory.bb_data.data, frame, 19);
}

void CoasterPagerView::start_tx() {
	generate_frame();
	
	transmitter_model.set_sampling_rate(2280000);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();

	baseband::set_fsk_data(19 * 8, 2280000 / 1000, 5000, 32);
}

void CoasterPagerView::on_tx_progress(const uint32_t progress, const bool done) {
	(void)progress;
	
	uint16_t address = 0;
	uint32_t c;
	
	if (done) {
		if (tx_mode == SINGLE) {
			transmitter_model.disable();
			tx_mode = IDLE;
			tx_view.set_transmitting(false);
		} else if (tx_mode == SCAN) {
			// Increment address
			
			for (c = 0; c < 4; c++) {
				address <<= 4;
				address |= sym_data.get_sym(12 + c);
			}
			
			address++;
			
			for (c = 0; c < 4; c++) {
				sym_data.set_sym(15 - c, address & 0x0F);
				address >>= 4;
			}
			
			start_tx();
		}
	}
}

CoasterPagerView::CoasterPagerView(NavigationView& nav) {
	const uint8_t data_init[8] = { 0x44, 0x01, 0x3B, 0x30, 0x30, 0x30, 0x34, 0xBC };
	uint32_t c;
	
	baseband::run_image(portapack::spi_flash::image_tag_fsktx);
	
	add_children({
		&labels,
		&sym_data,
		&checkbox_scan,
		&text_message,
		&tx_view
	});
	
	// Bytes to nibbles
	for (c = 0; c < 16; c++)
		sym_data.set_sym(c, (data_init[c >> 1] >> ((c & 1) ? 0 : 4)) & 0x0F);
	
	checkbox_scan.set_value(false);
	
	generate_frame();
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (tx_mode == IDLE) {
			if (checkbox_scan.value())
				tx_mode = SCAN;
			else
				tx_mode = SINGLE;
			tx_view.set_transmitting(true);
			start_tx();
		}
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		tx_mode = IDLE;
	};
}

} /* namespace ui */
