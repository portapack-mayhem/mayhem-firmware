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

#ifndef __UI_AUDIOTX_H__
#define __UI_AUDIOTX_H__

#include "ui.hpp"
#include "hal.h"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "message.hpp"
#include "ui_receiver.hpp"
#include "transmitter_model.hpp"
#include "ctcss.hpp"

namespace ui {

class AudioTXView : public View {
public:
	AudioTXView(NavigationView& nav);
	~AudioTXView();

	AudioTXView(const AudioTXView&) = delete;
	AudioTXView(AudioTXView&&) = delete;
	AudioTXView& operator=(const AudioTXView&) = delete;
	AudioTXView& operator=(AudioTXView&&) = delete;
	
	void focus() override;
	
	// PTT: Enable through KeyEvent (only works with presses), disable by polling :(
	bool on_key(const KeyEvent key) {
		if ((key == KeyEvent::Left) && (!va_enabled)) {
			set_tx(true);
			return true;
		} else
			return false;
	};
	
	std::string title() const override { return "Microphone TX"; };

private:
	void update_vumeter();
	void do_timing();
	void set_tx(bool enable);
	void on_tuning_frequency_changed(rf::Frequency f);
	void on_ctcss_changed(uint32_t v);
	
	bool transmitting { false };
	bool va_enabled { };
	uint32_t mic_gain_x10 { };
	uint32_t audio_level { 0 };
	uint32_t va_level { };
	uint32_t attack_ms { };
	uint32_t decay_ms { };
	uint32_t attack_timer { 0 };
	uint32_t decay_timer { 0 };
	
	Labels labels {
		{ { 7 * 8, 1 * 8 }, "Mic. gain:", Color::light_grey() },
		{ { 7 * 8, 4 * 8 }, "Voice activation:", Color::light_grey() },
		{ { 8 * 8, 9 * 8 }, "Level:   /255", Color::light_grey() },
		{ { 8 * 8, 11 * 8 }, "Attack:   ms", Color::light_grey() },
		{ { 8 * 8, 13 * 8 }, "Decay:    ms", Color::light_grey() },
		{ { 7 * 8, 17 * 8 }, "Bandwidth:   kHz", Color::light_grey() },
		{ { 7 * 8, 19 * 8 }, "Frequency:", Color::light_grey() },
		{ { 11 * 8, 21 * 8 }, "CTCSS:", Color::light_grey() }
	};
	
	VuMeter vumeter {
		{ 1 * 8, 2 * 8, 5 * 8, 26 * 8 },
		16
	};
	
	OptionsField options_gain {
		{ 17 * 8, 1 * 8 },
		4,
		{
			{ "x0.5", 5 },
			{ "x1.0", 10 },
			{ "x1.5", 15 },
			{ "x2.0", 20 }
		}
	};
	
	Checkbox check_va {
		{ 8 * 8, 6 * 8 },
		7,
		"Enabled",
		false
	};
	
	NumberField field_va_level {
		{ 14 * 8, 9 * 8 },
		3,
		{ 0, 255 },
		2,
		' '
	};
	NumberField field_va_attack {
		{ 15 * 8, 11 * 8 },
		3,
		{ 0, 999 },
		20,
		' '
	};
	NumberField field_va_decay {
		{ 14 * 8, 13 * 8 },
		4,
		{ 0, 9999 },
		100,
		' '
	};
	
	NumberField field_bw {
		{ 17 * 8, 17 * 8 },
		3,
		{ 0, 150 },
		1,
		' '
	};
	FrequencyField field_frequency {
		{ 17 * 8, 19 * 8 },
	};
	
	OptionsField options_ctcss {
		{ 17 * 8, 21 * 8 },
		8,
		{ }
	};

	Checkbox check_rogerbeep {
		{ 8 * 8, 23 * 8 },
		10,
		"Roger beep",
		false
	};
	
	Text text_ptt {
		{ 7 * 8, 28 * 8, 16 * 8, 16 },
		"PTT: LEFT BUTTON"
	};
	
	Button button_exit {
		{ 18 * 8, 32 * 8, 10 * 8, 40 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_lcd_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->update_vumeter();
			this->do_timing();
		}
	};
	
	MessageHandlerRegistration message_handler_audio_level {
		Message::ID::AudioLevel,
		[this](const Message* const p) {
			const auto message = static_cast<const AudioLevelMessage*>(p);
			this->audio_level = message->value;
		}
	};
};

} /* namespace ui */

#endif/*__UI_AUDIOTX_H__*/
