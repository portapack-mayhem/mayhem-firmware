/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_audiotx.hpp"

#include "baseband_api.hpp"
#include "ui_alphanum.hpp"
#include "audio.hpp"
#include "portapack.hpp"
#include "pins.hpp"
#include "string_format.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void AudioTXView::focus() {
	button_transmit.focus();
}

void AudioTXView::paint(Painter& painter) {
	_painter = &painter;
}

void AudioTXView::draw_vumeter() {
	uint32_t bar;
	Color color;
	bool lit = true;
	uint32_t bar_level = audio_level / 15;
	
	if (bar_level > 16) bar_level = 16;
	
	for (bar = 0; bar < 16; bar++) {
		if (bar >= bar_level)
			lit = false;
		
		if (bar < 11)
			color = lit ? Color::green() : Color::dark_green();
		else if ((bar >= 11) && (bar < 13))
			color = lit ? Color::yellow() : Color::dark_yellow();
		else if ((bar >= 13) && (bar < 15))
			color = lit ? Color::orange() : Color::dark_orange();
		else
			color = lit ? Color::red() : Color::dark_red();
		
		_painter->fill_rectangle({ 100, (Coord)(210 - (bar * 12)), 40, 10 }, color);
	}
	
	//text_power.set(to_string_hex(LPC_I2S0->STATE, 8) + " " + to_string_dec_uint(audio_level) + "  ");
	text_power.set(to_string_dec_uint(audio_level) + "  ");
}

void AudioTXView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

AudioTXView::AudioTXView(
	NavigationView& nav
)
{
	pins[P6_2].mode(3);		// I2S0_RX_SDA !
	
	baseband::run_image(portapack::spi_flash::image_tag_mic_tx);
	
	transmitter_model.set_tuning_frequency(92200000);
		
	add_children({
		&text_power,
		&field_frequency,
		&button_transmit,
		&button_exit
	});
	
	field_frequency.set_value(transmitter_model.tuning_frequency());
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
	
	button_transmit.on_select = [](Button&){
		transmitter_model.set_sampling_rate(1536000U);
		transmitter_model.set_rf_amp(true);
		transmitter_model.set_lna(40);
		transmitter_model.set_vga(40);
		transmitter_model.set_baseband_bandwidth(1750000);
		transmitter_model.enable();
		
		baseband::set_audiotx_data(
			76800,		// 20Hz level update
			10000,		// 10kHz bw
			false,
			0
		);
	};

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
	audio::set_rate(audio::Rate::Hz_24000);
	audio::input::start();
}

AudioTXView::~AudioTXView() {
	transmitter_model.disable();
	baseband::shutdown();
}

}
