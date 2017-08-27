/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_transmitter.hpp"

#include "audio.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
using namespace portapack;

#include "string_format.hpp"

#include "max2837.hpp"

namespace ui {

/* TXGainField **********************************************************/

TXGainField::TXGainField(
	Point parent_pos
) : NumberField {
		parent_pos, 2,
		{ max2837::tx::gain_db_range.minimum, max2837::tx::gain_db_range.maximum },
		max2837::tx::gain_db_step,
		' ',
	}
{
	set_value(transmitter_model.tx_gain());

	on_change = [](int32_t v) {
		transmitter_model.set_tx_gain(v);
	};
}

/* TransmitterView *******************************************************/

void TransmitterView::paint(Painter& painter) {
	size_t c;
	Point pos = { 0, screen_pos().y() };
	
	for (c = 0; c < 20; c++) {
		painter.draw_bitmap(
			pos,
			bitmap_stripes,
			ui::Color(191, 191, 0),
			ui::Color::black()
		);
		if (c != 9)
			pos += { 24, 0 };
		else
			pos = { 0, screen_pos().y() + 32 + 8 };
	}
}

void TransmitterView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

void TransmitterView::on_channel_bandwidth_changed(uint32_t channel_bandwidth) {
	transmitter_model.set_channel_bandwidth(channel_bandwidth);
}

void TransmitterView::set_transmitting(const bool transmitting) {
	if (transmitting) {
		button_start.set_text("STOP");
		button_start.set_style(&style_stop);
	} else {
		button_start.set_text("START");
		button_start.set_style(&style_start);
	}
	
	transmitting_ = transmitting;
}

void TransmitterView::on_show() {
	field_frequency.set_value(transmitter_model.tuning_frequency());
}

void TransmitterView::focus() {
	button_start.focus();
}

TransmitterView::TransmitterView(
	const Coord y, const uint64_t frequency_step, const uint32_t channel_bandwidth, const bool lock
) :	lock_ { lock }
{
	set_parent_rect({ 0, y, 30 * 8, 6 * 8 });
	
	add_children({
		&field_frequency,
		&text_gain,
		&field_gain,
		&button_start
	});
	
	set_transmitting(false);
	
	if (lock_) {
		field_frequency.set_focusable(false);
		field_frequency.set_style(&style_locked);
	} else {
		if (channel_bandwidth) {
			add_children({
				&text_bw,
				&field_bw
			});
			
			field_bw.on_change = [this](int32_t v) {
				on_channel_bandwidth_changed(v * 1000);
			};
			field_bw.set_value(channel_bandwidth);
		}
	}

	//field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(frequency_step);
	field_frequency.on_change = [this](rf::Frequency f) {
		on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this]() {
		if (on_edit_frequency)
			on_edit_frequency();
	};
	field_frequency.on_change = [this](rf::Frequency f) {
		transmitter_model.set_tuning_frequency(f);
	};
	
	button_start.on_select = [this](Button&){
		if (transmitting_) {
			if (on_stop)
				on_stop();
		} else {
			if (on_start)
				on_start();
		}
	};
}

TransmitterView::~TransmitterView() {
	audio::output::stop();
	transmitter_model.disable();
	baseband::shutdown();
}

} /* namespace ui */
