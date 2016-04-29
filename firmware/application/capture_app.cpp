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

#include "file.hpp"
#include "time.hpp"

#include "utility.hpp"

#include "string_format.hpp"

namespace ui {

CaptureAppView::CaptureAppView(NavigationView& nav) {
	add_children({ {
		&button_record,
		&rssi,
		&channel,
		&field_frequency,
		&field_lna,
		&field_vga,
		&text_record_filename,
		&text_record_dropped,
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

	field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};

	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};

	button_record.on_select = [this](ImageButton&) {
		this->on_record();
	};
	
	receiver_model.set_baseband_configuration({
		.mode = toUType(ReceiverModel::Mode::Capture),
		.sampling_rate = sampling_rate,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(baseband_bandwidth);
	receiver_model.enable();

	signal_token_tick_second = time::signal_tick_second += [this]() {
		this->on_tick_second();
	};
}

CaptureAppView::~CaptureAppView() {
	time::signal_tick_second -= signal_token_tick_second;
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
	button_record.focus();
}

void CaptureAppView::on_record() {
	if( capture_thread ) {
		capture_thread.reset();
		button_record.set_bitmap(&bitmap_record);
	} else {
		const auto filename_stem = next_filename_stem_matching_pattern("BBD_????");
		text_record_filename.set(filename_stem);
		text_record_dropped.set("");
		if( filename_stem.empty() ) {
			return;
		}

		capture_thread = std::make_unique<CaptureThread>(filename_stem + ".C16", 14, 1);
		button_record.set_bitmap(&bitmap_stop);
	}
}

void CaptureAppView::on_tick_second() {
	if( capture_thread ) {
		const auto dropped_percent = std::min(99U, capture_thread->state().dropped_percent());
		const auto s = to_string_dec_uint(dropped_percent, 2, ' ') + "\%";
		text_record_dropped.set(s);
	}
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
