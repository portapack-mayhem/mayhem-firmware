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

namespace ui {

CaptureAppView::CaptureAppView(NavigationView& nav) {
	add_children({ {
		&rssi,
		&channel,
		&field_frequency,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&record_view,
		&waterfall,
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

	receiver_model.set_baseband_configuration({
		.mode = toUType(ReceiverModel::Mode::Capture),
		.sampling_rate = sampling_rate,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(baseband_bandwidth);
	receiver_model.enable();

	record_view.set_sampling_rate(sampling_rate / 8);
	record_view.on_error = [&nav](std::string message) {
		nav.display_modal("Error", message);
	};
}

CaptureAppView::~CaptureAppView() {
	receiver_model.disable();
}

void CaptureAppView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	View::on_hide();
}

void CaptureAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	const ui::Rect waterfall_rect { 0, header_height, new_parent_rect.width(), static_cast<ui::Dim>(new_parent_rect.height() - header_height) };
	waterfall.set_parent_rect(waterfall_rect);
}

void CaptureAppView::focus() {
	record_view.focus();
}

void CaptureAppView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

} /* namespace ui */
