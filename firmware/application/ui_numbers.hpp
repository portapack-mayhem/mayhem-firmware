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
#include "ui_receiver.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "time.hpp"
#include "clock_manager.hpp"
#include "baseband_api.hpp"
#include "utility.hpp"
#include "message.hpp"
#include "wavfile.hpp"

namespace ui {

class NumbersStationView : public View {
public:
	NumbersStationView(NavigationView& nav);
	~NumbersStationView();

	void focus() override;
	
	std::string title() const override { return "Numbers station"; };
	
private:
	// Sequencing state machine
	enum segments {
		ANNOUNCE = 0,
		MESSAGE,
		SIGNOFF
	};
	
	Style style_red {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black()
	};
	
	NavigationView& nav_;
	
	segments segment; 
	bool armed = false;
	bool file_error = false;
	uint32_t sound_sizes[11];
	
	const std::vector<std::string> file_names = {
		{ "0" },
		{ "1" },
		{ "2" },
		{ "3" },
		{ "4" },
		{ "5" },
		{ "6" },
		{ "7" },
		{ "8" },
		{ "9" },
		{ "announce" }
	};
	
	// const uint8_t month_table[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	// const char * day_of_week[7] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
	
	std::unique_ptr<WAVFileReader> reader;
	
	uint8_t code_index, announce_loop;
	uint32_t sample_counter;
	uint32_t sample_duration;
	int8_t audio_buffer[1024];
	uint32_t pause = 0;
	bool armed_blink;
	SignalToken signal_token_tick_second;

	void on_tick_second();	
	void on_tuning_frequency_changed(rf::Frequency f);
	void prepare_audio();
	void start_tx();
	
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
		{ 1 * 8, 8 * 16, 11, 16 },
		"Schedule:"
	};
	
	FrequencyField field_frequency {
		{ 1 * 8, 4 },
	};
	NumberField number_bw {
		{ 12 * 8, 4 },
		3,
		{1, 150},
		1,
		' '
	};
	
	Text text_code {
		{ 20, 4 * 16, 5 * 8, 16 },
		"Code:"
	};	
	SymField symfield_code {
		{ 20, 5 * 16 },
		25,
		false
	};
	
	Checkbox check_armed {
		{ 2 * 8, 13 * 16 },
		5,
		"Armed"
	};
	Button button_tx_now {
		{ 18 * 8, 13 * 16, 10 * 8, 32 },
		"TX now"
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
