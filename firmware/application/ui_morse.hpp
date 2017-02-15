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

#ifndef __MORSE_TX_H__
#define __MORSE_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "portapack.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"
#include "morse.hpp"

#include <ch.h>

using namespace morse;

namespace ui {

class MorseView : public View {
public:
	MorseView(NavigationView& nav);
	~MorseView();
	
	MorseView(const MorseView&) = delete;
	MorseView(MorseView&&) = delete;
	MorseView& operator=(const MorseView&) = delete;
	MorseView& operator=(MorseView&&) = delete;
	
	void focus() override;
	void paint(Painter& painter) override;

	void on_tx_progress(const int progress, const bool done);
	
	std::string title() const override { return "Morse TX"; };
	
	uint32_t time_unit_ms { 0 };
	size_t symbol_count { 0 };
private:
	NavigationView& nav_;
	char buffer[29] = "PORTAPACK";
	std::string message { };
	
	bool start_tx();
	void on_set_text(NavigationView& nav);
	
	size_t modulation { 0 };
	Thread * ookthread { };
	
	Labels labels {
		{ { 4 * 8, 6 * 8 }, "Time unit:   ms", Color::light_grey() },
		{ { 4 * 8, 8 * 8 }, "Tone:    Hz", Color::light_grey() },
		{ { 4 * 8, 10 * 8 }, "Modulation:", Color::light_grey() },
	};
	
	Checkbox checkbox_foxhunt {
		{ 4 * 8, 16 },
		8,
		"Foxhunt:"
	};
	OptionsField options_foxhunt {
		{ 17 * 8, 16 + 4 },
		7,
		{
			{ "0 (MOE)", 0 },
			{ "1 (MOI)", 1 },
			{ "2 (MOS)", 2 },
			{ "3 (MOH)", 3 },
			{ "4 (MO5)", 4 },
			{ "5 (MON)", 5 },
			{ "6 (MOD)", 6 },
			{ "7 (MOB)", 7 },
			{ "8 (MO6)", 8 },
			{ "9 (MO) ", 9 },
			{ "10 (S) ", 10 }
		}
	};
	
	NumberField field_time_unit {
		{ 14 * 8, 6 * 8 },
		3,
		{ 10, 999 },
		1,
		' '
	};
	
	NumberField field_tone {
		{ 9 * 8, 8 * 8 },
		4,
		{ 100, 9999 },
		20,
		' '
	};
	
	OptionsField options_modulation {
		{ 15 * 8, 10 * 8 },
		2,
		{
			{ "CW", 0 },
			{ "FM", 1 }
		}
	};
	
	Text text_message {
		{ 1 * 8, 14 * 8, 28 * 8, 16 },
		""
	};
	
	Button button_message {
		{ 1 * 8, 16 * 8, 12 * 8, 28 },
		"Set message"
	};
	
	ProgressBar progressbar {
		{ 2 * 8, 14 * 16, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		12
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */

#endif/*__MORSE_TX_H__*/
