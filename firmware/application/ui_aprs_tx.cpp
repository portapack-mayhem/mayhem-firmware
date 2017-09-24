/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui_aprs_tx.hpp"
#include "ui_alphanum.hpp"

#include "aprs.hpp"
#include "string_format.hpp"
#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_persistent_memory.hpp"

#include <cstring>
#include <stdio.h>

using namespace aprs;
using namespace portapack;

namespace ui {

void APRSTXView::focus() {
	tx_view.focus();
}

APRSTXView::~APRSTXView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void APRSTXView::paint(Painter& painter) {
	(void)painter;
}

void APRSTXView::generate_frame() {

}

void APRSTXView::start_tx() {
	//generate_frame();
	
	/*transmitter_model.set_tuning_frequency(144800000);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(2500000);
	transmitter_model.enable();*/
}

void APRSTXView::on_tx_progress(const uint32_t progress, const bool done) {
	(void)progress;
	
	if (done) {
		transmitter_model.disable();
		tx_view.set_transmitting(false);
		//progress.set_value(0);
	} else {
		//progress.set_value(n);
	}
}

APRSTXView::APRSTXView(NavigationView& nav) {
	
	baseband::run_image(portapack::spi_flash::image_tag_afsk);

	add_children({
		&labels,
		&text_frame_a,
		&tx_view
	});
	
	tx_view.on_edit_frequency = [this, &nav]() {
		return;
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
	};
}

} /* namespace ui */
