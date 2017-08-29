/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_afsk_rx.hpp"
#include "baseband_api.hpp"

//#include "string_format.hpp"

using namespace portapack;

namespace ui {

void AFSKRxView::focus() {
	field_frequency.focus();
}

void AFSKRxView::update_freq(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

void AFSKRxView::on_bitrate_changed(const uint32_t new_bitrate) {
	baseband::set_afsk(new_bitrate);
}

AFSKRxView::AFSKRxView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_afsk_rx);
	
	add_children({
		&rssi,
		&channel,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&field_frequency,
		&options_bitrate,
		&console
	});
	
	//receiver_model.set_sampling_rate(3072000);
	//receiver_model.set_baseband_bandwidth(1750000);
	//receiver_model.enable();
	
	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		update_freq(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			update_freq(f);
			field_frequency.set_value(f);
		};
	};
	
	options_bitrate.on_change = [this](size_t, OptionsField::value_t v) {
		on_bitrate_changed(v);
	};
	options_bitrate.set_selected_index(1);	// 1200bps
}

void AFSKRxView::on_data(uint_fast8_t byte) {
	std::string str_byte(1, byte);
	console.write(str_byte);
}

AFSKRxView::~AFSKRxView() {
	receiver_model.disable();
	baseband::shutdown();
}

} /* namespace ui */
