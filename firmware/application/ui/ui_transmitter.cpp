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

void TransmitterView::on_tx_gain_changed(int32_t tx_gain) {
	transmitter_model.set_tx_gain(tx_gain);
	update_gainlevel_styles();
}

void TransmitterView::on_tx_amp_changed(bool rf_amp) {
	transmitter_model.set_rf_amp(rf_amp);
	update_gainlevel_styles();
}

void TransmitterView::update_gainlevel_styles() {
	const Style *new_style_ptr = NULL;
	int8_t tot_gain = transmitter_model.tx_gain() + (transmitter_model.rf_amp() ? 14 : 0);
	
	if(tot_gain > POWER_THRESHOLD_HIGH) {
		new_style_ptr = &style_power_high;
	} else if(tot_gain > POWER_THRESHOLD_MED) {
		new_style_ptr = &style_power_med;
	} else if(tot_gain > POWER_THRESHOLD_LOW) {
		new_style_ptr = &style_power_low;
	}

	field_gain.set_style(new_style_ptr);
	text_gain.set_style(new_style_ptr);
	field_amp.set_style(new_style_ptr);
	text_amp.set_style(new_style_ptr);
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
	field_frequency_step.set_by_value(receiver_model.frequency_step());

	field_gain.set_value(transmitter_model.tx_gain());
	field_amp.set_value(transmitter_model.rf_amp() ? 14 : 0);

	update_gainlevel_styles();
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
		&field_frequency_step,
		&text_gain,
		&field_gain,
		&button_start,
		&text_amp,
		&field_amp,
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
	
	field_frequency_step.on_change = [this](size_t, OptionsField::value_t v) {
		this->field_frequency.set_step(v);
	};

	field_gain.on_change = [this](uint32_t tx_gain) {
		on_tx_gain_changed(tx_gain);
	};
	
	field_amp.on_change = [this](uint32_t rf_amp) {
		on_tx_amp_changed((bool) rf_amp);
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

/* TransmitterView2 *******************************************************/
// Derivative from TransmitterView (that handles many param. freq, fre_step, start_button, gain, amp, used in the majority of the TX App's.
// This one ,is a simple version ,  it is only handling 2 x param  (TX GAIN and AMP ) in one line .
// We use normal character lines , in Mic App,  called (x pos, y pos,  NORMAL_UI)
// We use short compact char lines , in Replay / GPS Simul / Playlist App , called (x pos , y pos,SHORT_UI )

void TransmitterView2::paint(Painter& painter) {
//	Not using TransmitterView2, but if we delete it,we got , top line 1 a blanking rect.
	(void) painter;	// Avoid  warning: unused parameter .
}

void TransmitterView2::on_tx_gain_changed(int32_t tx_gain) {
	transmitter_model.set_tx_gain(tx_gain);
	update_gainlevel_styles();
}

void TransmitterView2::on_tx_amp_changed(bool rf_amp) {
	transmitter_model.set_rf_amp(rf_amp);
	update_gainlevel_styles();
}

void TransmitterView2::update_gainlevel_styles() {
	const Style *new_style_ptr = NULL;
	int8_t tot_gain = transmitter_model.tx_gain() + (transmitter_model.rf_amp() ? 14 : 0);
	
	if(tot_gain > POWER_THRESHOLD_HIGH) {
		new_style_ptr = &style_power_high;
	} else if(tot_gain > POWER_THRESHOLD_MED) {
		new_style_ptr = &style_power_med;
	} else if(tot_gain > POWER_THRESHOLD_LOW) {
		new_style_ptr = &style_power_low;
	}

	field_gain.set_style(new_style_ptr);
	text_gain_amp.set_style(new_style_ptr);
	field_amp.set_style(new_style_ptr);

	field_gain_short_UI.set_style(new_style_ptr);
	text_gain_amp_short_UI.set_style(new_style_ptr);
	field_amp_short_UI.set_style(new_style_ptr);
}

void TransmitterView2::on_show() {
	field_gain.set_value(transmitter_model.tx_gain());
	field_amp.set_value(transmitter_model.rf_amp() ? 14 : 0);

	field_gain_short_UI.set_value(transmitter_model.tx_gain());
	field_amp_short_UI.set_value(transmitter_model.rf_amp() ? 14 : 0);
	
	update_gainlevel_styles();
}

TransmitterView2::TransmitterView2(	const Coord x, const Coord y, bool short_UI)
{
	set_parent_rect({ x , y, 20 * 8, 1 * 8 });		// set_parent_rect({ 0, y, 30 * 8, 6 * 8 });

	add_children({
		&(short_UI ? text_gain_amp_short_UI : text_gain_amp),
		&(short_UI ? field_gain_short_UI : field_gain),
		&(short_UI ? field_amp_short_UI  : field_amp),
	});

	if (short_UI) {
		field_gain_short_UI.on_change = [this](uint32_t tx_gain) {
			on_tx_gain_changed(tx_gain);
		};
		field_amp_short_UI.on_change = [this](uint32_t rf_amp) {
			on_tx_amp_changed((bool) rf_amp);
		};
	} else {
		field_gain.on_change = [this](uint32_t tx_gain) {
			on_tx_gain_changed(tx_gain);
		};
		field_amp.on_change = [this](uint32_t rf_amp) {
			on_tx_amp_changed((bool) rf_amp);
		};
	}			
}

TransmitterView2::~TransmitterView2() {
	audio::output::stop();
	transmitter_model.disable();
	baseband::shutdown();
}

} /* namespace ui */
