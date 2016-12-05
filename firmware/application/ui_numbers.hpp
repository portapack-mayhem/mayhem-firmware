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

#ifndef __UI_NUMBERS_H__
#define __UI_NUMBERS_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "clock_manager.hpp"
#include "message.hpp"
#include "baseband_api.hpp"
#include "file.hpp"

namespace ui {

class NumbersStationView : public View {
public:
	NumbersStationView(NavigationView& nav);
	~NumbersStationView();

	void focus() override;
	void paint(Painter& painter) override;

	std::string title() const override { return "Numbers station"; };
	
private:
	// Different from the one in ui_soundboard.hpp, simpler
	struct sound {
		uint32_t size = 0;
		uint32_t sample_duration = 0;
	};
	
	sound sounds[11];
	
	const std::string filenames[11] = {
		"zero",
		"one",
		"two",
		"three",
		"four",
		"five",
		"six",
		"seven",
		"eight",
		"nine",
		"anounce"
	};

	const uint8_t month_table[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	const char * day_of_week[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
	
	File file;
	uint32_t cnt;
	uint32_t sample_duration;
	int8_t audio_buffer[1024];
	
	void on_tuning_frequency_changed(rf::Frequency f);
	void prepare_audio();
	void play_sound(uint16_t id);
	
	// Schedule: save on sd card
	// For each day of the week, max 8 messages ?
	// For each message: Normal, accent. Can chose accent on first or last digit
	// Prelude with number of repeats
	// Message 1 with number of repeats
	// Interlude ?
	// Message 2 with number of repeats
	// End
	// Frequency list and sequence
	
	Text text_title {
		{ 1 * 8, 1 * 16, 11, 16 },
		"Schedule:"
	};
	
	NumberField number_bw {
		{ 11 * 8, 3 * 16 },
		3,
		{1, 150},
		1,
		' '
	};
	
	Button button_exit {
		{ 21 * 8, 16 * 16, 64, 32 },
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

#endif/*__UI_NUMBERS_H__*/
