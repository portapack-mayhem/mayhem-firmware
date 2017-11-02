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

#include "ui_test.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

namespace ui {

TestView::TestView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_test);

	add_children({
		&labels,
		&field_frequency,
		&field_rf_amp,
		&field_lna,
		&field_vga,
		&rssi,
		&text_debug_a,
		&text_debug_b
	});

	field_frequency.set_value(target_frequency_);
	field_frequency.set_step(10000);
	field_frequency.on_change = [this](rf::Frequency f) {
		set_target_frequency(f);
		field_frequency.set_value(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			set_target_frequency(f);
			field_frequency.set_value(f);
		};
	};
	
	radio::enable({
		tuning_frequency(),
		sampling_rate,
		baseband_bandwidth,
		rf::Direction::Receive,
		receiver_model.rf_amp(),
		static_cast<int8_t>(receiver_model.lna()),
		static_cast<int8_t>(receiver_model.vga()),
	});
}

TestView::~TestView() {
	radio::disable();
	baseband::shutdown();
}

void TestView::focus() {
	field_vga.focus();
}

void TestView::on_packet(const testapp::Packet& packet) {
	const auto hex_formatted = packet.symbols_formatted();
	auto v = packet.value();
	
	packet_count++;
	uint32_t diff = ((v - 1) - prev_v);
	if (diff < 20)
		packets_lost += diff;
	prev_v = v;
	
	text_debug_a.set(hex_formatted.data.substr(0, 30));
	
	text_debug_b.set(to_string_dec_uint((packets_lost * 1000) / packet_count) + " per 1000");
	
	display.draw_pixel(Point(cur_x, 4 * 16 + (256 - packet.alt())), Color::white());
	
	cur_x++;
	if (cur_x >= 240) {
		display.fill_rectangle(Rect(0, 4 * 16, 240, 256), Color::black());
		cur_x = 0;
	}
	
	//radio::disable();
	
	/*text_serial.set(packet.serial_number());
	text_voltage.set(unit_auto_scale(packet.battery_voltage(), 2, 3) + "V");
	
	altitude = packet.GPS_altitude();
	latitude = packet.GPS_latitude();
	longitude = packet.GPS_longitude();*/
}

void TestView::set_target_frequency(const uint32_t new_value) {
	target_frequency_ = new_value;
	radio::set_tuning_frequency(tuning_frequency());
}

uint32_t TestView::tuning_frequency() const {
	return target_frequency_ - (sampling_rate / 4);
}

} /* namespace ui */
