/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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

#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

namespace ui {

CaptureAppView::CaptureAppView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_capture);

	add_children({
		&labels,
		&rssi,
		&channel,
		&field_frequency,
		&field_frequency_step,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&option_bandwidth,
		&record_view,
		&waterfall,
	});

	// Hack for initialization
	// TODO: This should be included in a more global section so apps dont need to do it 
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	//-------------------

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
	
	field_frequency_step.set_by_value(receiver_model.frequency_step());
	field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
		receiver_model.set_frequency_step(v);
		this->field_frequency.set_step(v);
	};
	
	option_bandwidth.on_change = [this](size_t, uint32_t base_rate) {
		sampling_rate = 8 * base_rate;	// Decimation by 8 done on baseband side
		
		waterfall.on_hide();
		record_view.set_sampling_rate(sampling_rate);
		receiver_model.set_sampling_rate(sampling_rate);
		waterfall.on_show();
	};
	
	option_bandwidth.set_selected_index(7);		// 500k
	
	receiver_model.set_modulation(ReceiverModel::Mode::Capture);
	receiver_model.set_baseband_bandwidth(baseband_bandwidth);
	receiver_model.enable();

	record_view.on_error = [&nav](std::string message) {
		nav.display_modal("Error", message);
	};
}

CaptureAppView::~CaptureAppView() {

	// Hack for preventing halting other apps
	// TODO: This should be also part of something global
	receiver_model.set_sampling_rate(3072000);
	receiver_model.set_baseband_bandwidth(1750000);
	receiver_model.set_modulation(ReceiverModel::Mode::WidebandFMAudio);
	// ----------------------------

	receiver_model.disable();
	baseband::shutdown();
}

void CaptureAppView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	View::on_hide();
}

void CaptureAppView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	const ui::Rect waterfall_rect { 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height };
	waterfall.set_parent_rect(waterfall_rect);
}

void CaptureAppView::focus() {
	record_view.focus();
}

void CaptureAppView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

} /* namespace ui */
