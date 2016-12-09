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

#ifndef __UI_SOUNDBOARD_H__
#define __UI_SOUNDBOARD_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "baseband_api.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "utility.hpp"
#include "message.hpp"
#include "wavfile.hpp"
#include "ctcss.hpp"

namespace ui {

class SoundBoardView : public View {
public:
	SoundBoardView(NavigationView& nav);
	~SoundBoardView();

	void focus() override;
	
	std::string title() const override { return "Soundboard"; };
	
private:
	NavigationView& nav_;
	
	enum tx_modes {
		NORMAL = 0,
		RANDOM
	};
	
	tx_modes tx_mode = NORMAL;
	
	struct sound {
		std::string filename = "";
		std::string shortname = "";
		bool sixteenbit = false;
		uint32_t sample_rate = 0;
		uint32_t size = 0;
		uint32_t sample_duration = 0;
		uint32_t ms_duration = 0;
	};
	
	uint32_t sample_counter;
	uint32_t sample_duration;
	uint8_t page = 0;
	
	uint16_t lfsr_v = 0x1337;
	
	std::unique_ptr<WAVFileReader> reader;
	
	sound sounds[105];
	uint8_t max_sound;
	uint8_t max_page;
	
	uint32_t _ctcss_freq;

	int8_t audio_buffer[1024];
	
	Style style_a {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 255, 51, 153 }
	};
	Style style_b {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 153, 204, 0 }
	};
	Style style_c {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 51, 204, 204 }
	};
	Style style_d {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = { 153, 102, 255 }
	};

	std::array<Button, 21> buttons;
	const Style * styles[4] = { &style_a, &style_b, &style_c, &style_d };
	
	void on_tuning_frequency_changed(rf::Frequency f);
	
	void do_random();
	void show_infos(uint16_t id);
	void change_page(Button& button, const KeyEvent key);
	void refresh_buttons(uint16_t id);
	void play_sound(uint16_t id);
	void prepare_audio();
	void on_ctcss_changed(uint32_t v);
	
	Text text_duration {
		{ 16, 236, 5 * 8, 16 }
	};
	
	FrequencyField field_frequency {
		{ 1 * 8, 4 },
	};
	
	NumberField number_bw {
		{ 11 * 8, 4 },
		3,
		{1, 150},
		1,
		' '
	};
	
	Text text_kHz {
		{ 14 * 8, 4, 3 * 8, 16 },
		"kHz"
	};
	
	OptionsField options_ctcss {
		{ 18 * 8, 4 },
		6,
		{ }
	};
	
	Text text_page {
		{ 25 * 8, 4, 3 * 8, 16 },
		"-/-"
	};
	
	ProgressBar pbar {
		{ 72, 236, 150, 16 }
	};
	
	Checkbox check_loop {
		{ 8, 274 },
		4,
		"Loop"
	};
	
	Button button_random {
		{ 80, 270, 72, 32 },
		"Random"
	};
	
	Button button_exit {
		{ 160, 270, 64, 32 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_fifo_signal {
		Message::ID::FIFOSignal,
		[this](const Message* const p) {
			const auto message = static_cast<const FIFOSignalMessage*>(p);
			if (message->signaltype == 1) {
				this->prepare_audio();
			}
		}
	};
};

} /* namespace ui */

#endif/*__UI_SOUNDBOARD_H__*/
