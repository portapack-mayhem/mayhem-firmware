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

using namespace morse;

namespace ui {

class MorseView : public View {
public:
	MorseView(NavigationView& nav);
	~MorseView();
	
	void focus() override;
	void paint(Painter& painter) override;

private:
	NavigationView& nav_;
	
	bool start_tx();
	void on_tx_progress(const int progress, const bool done);
	
	Text text_time_unit {
		{ 4 * 8, 6 * 8, 15 * 8, 16 },
		"Time unit:   ms"
	};
	NumberField field_time_unit {
		{ 14 * 8, 6 * 8 },
		3,
		{ 1, 999 },
		1,
		' '
	};
	
	Text text_tone {
		{ 4 * 8, 8 * 8, 11 * 8, 16 },
		"Tone:    Hz"
	};
	NumberField field_tone {
		{ 9 * 8, 8 * 8 },
		4,
		{ 100, 9999 },
		20,
		' '
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
