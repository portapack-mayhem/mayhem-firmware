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

#include "ui_cw.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;

namespace ui {

void CWTXView::focus() {
	tx_view.focus();
}

CWTXView::~CWTXView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void CWTXView::start_tx() {
	transmitter_model.set_sampling_rate(1536000U);		// Not important
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(1750000);	// Not important
	transmitter_model.enable();
}

void CWTXView::stop_tx() {
	transmitter_model.disable();
}

CWTXView::CWTXView(
	NavigationView& nav
)
{
	baseband::run_image(portapack::spi_flash::image_tag_noop);
	
	add_children({
		&tx_view
	});
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		start_tx();
		tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		stop_tx();
		tx_view.set_transmitting(false);
	};

}

} /* namespace ui */
