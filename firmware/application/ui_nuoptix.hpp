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

#ifndef __UI_NUOPTIX_H__
#define __UI_NUOPTIX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "baseband_api.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "message.hpp"

namespace ui {
	
class NuoptixView : public View {
public:
	NuoptixView(NavigationView& nav);
	~NuoptixView();

	void focus() override;
	
	std::string title() const override { return "Nuoptix sync"; };
	
private:
	/*enum tx_modes {
		NORMAL = 0,
		RANDOM
	};
	
	tx_modes tx_mode = NORMAL;*/

	void on_tuning_frequency_changed(rf::Frequency f);
	void transmit(bool setup);
	
	bool txing = false;
	uint32_t timecode;
	
	FrequencyField field_frequency {
		{ 1 * 8, 4 },
	};
	
	NumberField number_bw {
		{ 13 * 8, 4 },
		3,
		{1, 150},
		1,
		' '
	};
	
	Text text_kHz {
		{ 16 * 8, 4, 3 * 8, 16 },
		"kHz"
	};
	
	Text text_timecode {
		{ 10 * 8, 32, 9 * 8, 16 },
		"Timecode:"
	};
	
	NumberField number_timecode {
		{ 13 * 8, 48 },
		4,
		{ 1, 9999 },
		1,
		'0'
	};
	
	Text text_mod {
		{ 10 * 8, 80, 6 * 8, 16 },
		"Mod: "
	};
	
	ProgressBar pbar {
		{ 16, 236, 208, 16 }
	};
	
	/*Checkbox check_loop {
		{ 16, 274 },
		4,
		"Loop"
	};*/
	
	Button button_tx {
		{ 32, 270, 64, 32 },
		"TX"
	};
	
	Button button_exit {
		{ 160, 270, 64, 32 },
		"Exit"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			if (message.n == 64)
				transmit(false);
			else
				pbar.set_value(message.n);
		}
	};
};

} /* namespace ui */

#endif/*__UI_NUOPTIX_H__*/
