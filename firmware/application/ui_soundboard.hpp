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
#include "message.hpp"
#include "file.hpp"

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
	
	uint32_t cnt;
	uint32_t sample_duration;
	uint8_t page = 0;
	
	File file;
	
	uint16_t lfsr_v = 0x1337;
	
	sound sounds[100];
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
	uint16_t fb_to_uint16(const std::string& fb);
	uint32_t fb_to_uint32(const std::string& fb);
	
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
		{
			{ "None  ", 0 },
			{ "XZ 000", 67000 },
			{ "WZ 001", 69400 },
			{ "XA 039", 71900 },
			{ "WA 003", 74400 },
			{ "XB 004", 77000 },
			{ "WB 005", 79700 },
			{ "YZ 006", 82500 },
			{ "YA 007", 85400 },
			{ "YB 008", 88500 },
			{ "ZZ 009", 91500 },
			{ "ZA 010", 94800 },
			{ "ZB 011", 97400 },
			{ "1Z 012", 100000 },
			{ "1A 013", 103500 },
			{ "1B 014", 107200 },
			{ "2Z 015", 110900 },
			{ "2Z 016", 114800 },
			{ "2B 017", 118800 },
			{ "3Z 018", 123000 },
			{ "3A 019", 127300 },
			{ "3B 020", 131800 },
			{ "4Z 021", 136500 },
			{ "4A 022", 141300 },
			{ "4B 023", 146200 },
			{ "MIL   ", 150000 },
			{ "5Z 024", 151400 },
			{ "5A 025", 156700 },
			{ "-- 040", 159800 },
			{ "5B 026", 162200 },
			{ "-- 041", 165500 },
			{ "6Z 027", 167900 },
			{ "-- 042", 171300 },
			{ "6A 028", 173800 },
			{ "-- 043", 177300 },
			{ "6B 029", 179900 },
			{ "-- 044", 183500 },
			{ "7Z 030", 186200 },
			{ "-- 045", 189900 },
			{ "7A 031", 192800 },
			{ "-- 046", 196600 },
			{ "-- 047", 199500 },
			{ "M1 032", 203500 },
			{ "8Z 048", 206500 },
			{ "M2 033", 210700 },
			{ "M3 034", 218100 },
			{ "M4 035", 225700 },
			{ "9Z 049", 229100 },
			{ "-- 036", 233600 },
			{ "-- 037", 241800 },
			{ "-- 038", 250300 },
			{ "0Z 050", 254100 }
		}
	};
	
	Text text_page {
		{ 25 * 8, 4, 3 * 8, 16 },
		"-/-"
	};
	
	ProgressBar pbar {
		{ 72, 236, 150, 16 }
	};
	
	Checkbox check_loop {
		{ 16, 274 },
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
