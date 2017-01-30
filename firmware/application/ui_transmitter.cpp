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

void TransmitterView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

void TransmitterView::on_bandwidth_changed(uint32_t bandwidth) {
	transmitter_model.set_bandwidth(bandwidth);
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
	field_frequency.set_value(receiver_model.tuning_frequency());
}

TransmitterView::TransmitterView(
	const Coord y, const uint32_t frequency_step, const uint32_t bandwidth
) {
	set_parent_rect({ 0 * 8, y, 30 * 8, 2 * 16 });
	
	add_children({
		&field_frequency,
		&field_gain,
		&field_bw,
		&text_kHz,
		&button_start
	});
	
	set_transmitting(false);

	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(frequency_step);
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this]() {
		if (on_edit_frequency)
			on_edit_frequency();
	};
	
	field_bw.on_change = [this](int32_t bandwidth) {
		transmitter_model.set_bandwidth(bandwidth);
	};
	field_bw.set_value(bandwidth);
	
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
	/*audio::output::stop();

	transmitter_model.disable();

	baseband::shutdown();*/
}

} /* namespace ui */
