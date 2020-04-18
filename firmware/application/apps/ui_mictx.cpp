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
#include "audio.hpp"
#include "tonesets.hpp"
#include "portapack_hal.hpp"
#include "string_format.hpp"
#include "irq_controls.hpp"

#include <cstring>

using namespace tonekey;
using namespace portapack;

namespace ui {

void MicTXView::focus() {
	field_frequency.focus();
}

void MicTXView::update_vumeter() {
	vumeter.set_value(audio_level);
}

void MicTXView::on_tx_progress(const bool done) {
	// Roger beep played, stop transmitting
	if (done)
		set_tx(false);
}

void MicTXView::configure_baseband() {
	baseband::set_audiotx_config(
		sampling_rate / 20,		// Update vu-meter at 20Hz
		transmitting ? transmitter_model.channel_bandwidth() : 0,
		mic_gain,
		TONES_F2D(tone_key_frequency(tone_key_index), sampling_rate)
	);
}

void MicTXView::set_tx(bool enable) {
	if (enable) {
		transmitting = true;
		configure_baseband();
		transmitter_model.enable();
		portapack::pin_i2s0_rx_sda.mode(3);		// This is already done in audio::init but gets changed by the CPLD overlay reprogramming
		//gpio_tx.write(1);
		//led_tx.on();
	} else {
		if (transmitting && rogerbeep_enabled) {
			baseband::request_beep();
			transmitting = false;
		} else {
			transmitting = false;
			configure_baseband();
			transmitter_model.disable();
			//gpio_tx.write(0);
			//led_tx.off();
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
					attack_timer += lcd_frame_duration;
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
					decay_timer += lcd_frame_duration;
				}
			} else {
				decay_timer = 0;
			}
		}
	} else {
		// Check for PTT release
		const auto switches_state = get_switches_state();
		if (!switches_state[0] && transmitting)		// Right button
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
	portapack::pin_i2s0_rx_sda.mode(3);		// This is already done in audio::init but gets changed by the CPLD overlay reprogramming
	
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
		&options_tone_key,
		&check_rogerbeep,
		&text_ptt
	});
	
	tone_keys_populate(options_tone_key);
	options_tone_key.on_change = [this](size_t i, int32_t) {
		tone_key_index = i;
	};
	options_tone_key.set_selected_index(0);
	
	options_gain.on_change = [this](size_t, int32_t v) {
		mic_gain = v / 10.0;
		configure_baseband();
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
		transmitter_model.set_channel_bandwidth(v * 1000);
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
	field_va_decay.set_value(1000);
	
	transmitter_model.set_sampling_rate(sampling_rate);
	transmitter_model.set_baseband_bandwidth(1750000);
	
	set_tx(false);
	
	audio::set_rate(audio::Rate::Hz_24000);
	audio::input::start();
}

MicTXView::~MicTXView() {
	audio::input::stop();
	transmitter_model.disable();
	baseband::shutdown();
}

}
