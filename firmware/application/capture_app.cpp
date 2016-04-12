/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
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

#include "capture_app.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "utility.hpp"

namespace ui {

CaptureAppView::CaptureAppView(NavigationView& nav) {
	add_children({ {
		&rssi,
		&channel,
		&field_frequency,
		&field_lna,
		&field_vga,
		&button_start,
		&button_stop,
	} });

	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};

	field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};

	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};

	button_start.on_select = [this](Button&){ this->on_start(); };
	button_stop.on_select = [this](Button&){ this->on_stop(); };
	
	receiver_model.set_baseband_configuration({
		.mode = toUType(ReceiverModel::Mode::Capture),
		.sampling_rate = sampling_rate,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(baseband_bandwidth);
	receiver_model.enable();
}

CaptureAppView::~CaptureAppView() {
	receiver_model.disable();
}

void CaptureAppView::focus() {
	button_start.focus();
}

void CaptureAppView::on_start() {
	if( !capture_thread ) {
		capture_thread = std::make_unique<AudioThread>("baseband.c16");
	}
}

void CaptureAppView::on_stop() {
	capture_thread.reset();
}

void CaptureAppView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

void CaptureAppView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void CaptureAppView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

} /* namespace ui */
