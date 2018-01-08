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

#ifndef __UI_MICTX_H__
#define __UI_MICTX_H__

#include "ui.hpp"
#include "hal.h"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_receiver.hpp"
#include "transmitter_model.hpp"
#include "tone_key.hpp"
#include "message.hpp"

namespace ui {

class MicTXView : public View {
public:
	MicTXView(NavigationView& nav);
	~MicTXView();

	MicTXView(const MicTXView&) = delete;
	MicTXView(MicTXView&&) = delete;
	MicTXView& operator=(const MicTXView&) = delete;
	MicTXView& operator=(MicTXView&&) = delete;
	
	void focus() override;
	
	// PTT: Enable through KeyEvent (only works with presses), disable by polling :(
	bool on_key(const KeyEvent key) {
		if ((key == KeyEvent::Right) && (!va_enabled)) {
			set_tx(true);
			return true;
		} else
			return false;
	};
	
	std::string title() const override { return "Microphone TX"; };

private:
	static constexpr uint32_t sampling_rate = 1536000U;
	static constexpr uint32_t lcd_frame_duration = (256 * 1000UL) / 60;	// 1 frame @ 60fps in ms .8 fixed point
	
	void update_vumeter();
	void do_timing();
	void set_tx(bool enable);
	void on_tuning_frequency_changed(rf::Frequency f);
	void on_tx_progress(const bool done);
	void configure_baseband();
	
	bool transmitting { false };
	bool va_enabled { };
	bool rogerbeep_enabled { };
	uint32_t tone_key_index { };
	uint32_t mic_gain_x10 { 10 };
	uint32_t audio_level { 0 };
	uint32_t va_level { };
	uint32_t attack_ms { };
	uint32_t decay_ms { };
	uint32_t attack_timer { 0 };
	uint32_t decay_timer { 0 };
	
	Labels labels {
		{ { 7 * 8, 1 * 8 }, "Mic. gain:", Color::light_grey() },
		{ { 7 * 8, 4 * 8 }, "Frequency:", Color::light_grey() },
		{ { 7 * 8, 6 * 8 }, "Bandwidth:   kHz", Color::light_grey() },
		{ { 9 * 8, 13 * 8 }, "Level:   /255", Color::light_grey() },
		{ { 9 * 8, 15 * 8 }, "Attack:   ms", Color::light_grey() },
		{ { 9 * 8, 17 * 8 }, "Decay:    ms", Color::light_grey() },
		{ { 7 * 8, 21 * 8 }, "Tone key:", Color::light_grey() }
	};
	
	VuMeter vumeter {
		{ 1 * 8, 2 * 8, 5 * 8, 32 * 8 },
		20,
		false
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
	
	FrequencyField field_frequency {
		{ 17 * 8, 4 * 8 },
	};
	NumberField field_bw {
		{ 17 * 8, 6 * 8 },
		3,
		{ 0, 150 },
		1,
		' '
	};
	
	Checkbox check_va {
		{ 7 * 8, 10 * 8 },
		7,
		"Voice activation",
		false
	};
	
	NumberField field_va_level {
		{ 15 * 8, 13 * 8 },
		3,
		{ 0, 255 },
		2,
		' '
	};
	NumberField field_va_attack {
		{ 16 * 8, 15 * 8 },
		3,
		{ 0, 999 },
		20,
		' '
	};
	NumberField field_va_decay {
		{ 15 * 8, 17 * 8 },
		4,
		{ 0, 9999 },
		100,
		' '
	};
	
	OptionsField options_tone_key {
		{ 7 * 8, 23 * 8 },
		23,
		{ }
	};

	Checkbox check_rogerbeep {
		{ 7 * 8, 26 * 8 },
		10,
		"Roger beep",
		false
	};
	
	Text text_ptt {
		{ 7 * 8, 17 * 16, 16 * 8, 16 },
		"PTT: RIGHT BUTTON"
	};
	
	MessageHandlerRegistration message_handler_lcd_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->update_vumeter();
			this->do_timing();
		}
	};
	
	MessageHandlerRegistration message_handler_audio_level {
		Message::ID::AudioLevelReport,
		[this](const Message* const p) {
			const auto message = static_cast<const AudioLevelReportMessage*>(p);
			this->audio_level = message->value;
		}
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.done);
		}
	};
};

} /* namespace ui */

#endif/*__UI_MICTX_H__*/
