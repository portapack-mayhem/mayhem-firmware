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
#include "volume.hpp"
#include "ui_receiver.hpp"
#include "transmitter_model.hpp"

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
	void paint(Painter& painter) override;
	
	std::string title() const override { return "Microphone TX"; };

private:
	void draw_vumeter();
	void on_tuning_frequency_changed(rf::Frequency f);
	
	uint32_t audio_level { 0 };
	Painter * _painter { };
	
	Text text_power {
		{ 6 * 8, 28 * 8, 16 * 8, 16 },
		"-"
	};
	
	FrequencyField field_frequency {
		{ 6 * 8, 30 * 8 },
	};
	
	Button button_transmit {
		{ 1 * 8, 33 * 8, 12 * 8, 32 },
		"Transmit"
	};
	
	Button button_exit {
		{ 16 * 8, 33 * 8, 12 * 8, 32 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_lcd_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->draw_vumeter();
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
