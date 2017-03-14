/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_mictx.hpp"

#include "baseband_api.hpp"
#include "hackrf_gpio.hpp"
#include "audio.hpp"
#include "portapack.hpp"
#include "pins.hpp"
#include "string_format.hpp"
#include "irq_controls.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace ctcss;
using namespace portapack;
using namespace hackrf::one;

namespace ui {

void MicTXView::focus() {
	field_frequency.focus();
}

void MicTXView::update_vumeter() {
	vumeter.set_value(audio_level);
}

void MicTXView::on_tx_done() {
	// Roger beep transmitted, stop transmitting
	transmitting = false;
	set_tx(false);
}

void MicTXView::set_tx(bool enable) {
	uint32_t ctcss_index;
	bool ctcss_enabled;
	
	if (enable) {
		ctcss_index = options_ctcss.selected_index();
		
		if (ctcss_index) {
			ctcss_enabled = true;
			ctcss_index--;
		} else
			ctcss_enabled = false;
		
		baseband::set_audiotx_data(
			1536000U / 20,		// 20Hz level update
			transmitter_model.bandwidth(),
			mic_gain_x10,
			ctcss_enabled,
			(uint32_t)((ctcss_tones[ctcss_index].frequency / 1536000.0) * 0xFFFFFFFFULL)
		);
		gpio_tx.write(1);
		led_tx.on();
		transmitting = true;
	} else {
		if (transmitting && rogerbeep_enabled) {
			baseband::request_beep();
		} else {
			baseband::set_audiotx_data(
				1536000U / 20,		// 20Hz level update
				0,					// BW 0 = TX off
				mic_gain_x10,
				false,				// Ignore CTCSS
				0
			);
			gpio_tx.write(0);
			led_tx.off();
			transmitting = false;
		}
	}
}

void MicTXView::do_timing() {
	if (va_enabled) {
		if (!transmitting) {
			// Attack
			if (audio_level >= va_level) {
				if ((attack_timer >> 8) >= attack_ms) {
					decay_timer = 0;
					attack_timer = 0;
					set_tx(true);
				} else {
					attack_timer += ((256 * 1000) / 60);	// 1 frame @ 60fps in ms .8 fixed point
				}
			} else {
				attack_timer = 0;
			}
		} else {
			// Decay
			if (audio_level < va_level) {
				if ((decay_timer >> 8) >= decay_ms) {
					decay_timer = 0;
					attack_timer = 0;
					set_tx(false);
				} else {
					decay_timer += ((256 * 1000) / 60);		// 1 frame @ 60fps in ms .8 fixed point
				}
			} else {
				decay_timer = 0;
			}
		}
	} else {
		// PTT disable :(
		const auto switches_state = get_switches_state();
		if (!switches_state[1])		// Left button
			set_tx(false);
	}
}

void MicTXView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

MicTXView::MicTXView(
	NavigationView& nav
)
{
	pins[P6_2].mode(3);		// Set P6_2 pin function to I2S0_RX_SDA
	
	baseband::run_image(portapack::spi_flash::image_tag_mic_tx);
		
	add_children({
		&labels,
		&vumeter,
		&options_gain,
		&check_va,
		&field_va_level,
		&field_va_attack,
		&field_va_decay,
		&field_bw,
		&field_frequency,
		&options_ctcss,
		&check_rogerbeep,
		&text_ptt,
		&button_exit
	});
	
	ctcss_populate(options_ctcss);
	options_ctcss.set_selected_index(0);
	
	options_gain.on_change = [this](size_t, int32_t v) {
		mic_gain_x10 = v;
	};
	options_gain.set_selected_index(1);		// x1.0
	
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
	
	field_bw.on_change = [this](uint32_t v) {
		transmitter_model.set_bandwidth(v * 1000);
	};
	field_bw.set_value(10);
	
	check_va.on_select = [this](Checkbox&, bool v) {
		va_enabled = v;
		text_ptt.hidden(v);
		set_dirty();
	};
	check_va.set_value(false);
	
	check_rogerbeep.on_select = [this](Checkbox&, bool v) {
		rogerbeep_enabled = v;
	};
	check_rogerbeep.set_value(false);
	
	field_va_level.on_change = [this](int32_t v) {
		va_level = v;
		vumeter.set_mark(v);
	};
	field_va_level.set_value(40);
	
	field_va_attack.on_change = [this](int32_t v) {
		attack_ms = v;
	};
	field_va_attack.set_value(500);
	
	field_va_decay.on_change = [this](int32_t v) {
		decay_ms = v;
	};
	field_va_decay.set_value(2000);

	button_exit.on_select = [&nav](Button&){
		nav.pop();
	};
	
	// Run baseband as soon as the app starts to get audio levels without transmitting (rf amp off)
	transmitter_model.set_sampling_rate(1536000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	set_tx(false);
	
	audio::set_rate(audio::Rate::Hz_24000);
	audio::input::start();
}

MicTXView::~MicTXView() {
	transmitter_model.disable();
	baseband::shutdown();
}

}
