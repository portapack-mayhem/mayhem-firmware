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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "bmp_bulb_on.hpp"
#include "bmp_bulb_off.hpp"
#include "bmp_bulb_ignore.hpp"

#include "bht.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"
#include "portapack.hpp"

namespace ui {

class BHTView : public View {
public:
	BHTView(NavigationView& nav);
	~BHTView();
	
	void focus() override;
	
	std::string title() const override { return "BHT transmit"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	tx_modes tx_mode = IDLE;
	
	bool speaker_enabled = false;
	size_t _mode = 0;
	
	void start_tx();
	void generate_message();
	void on_tx_progress(const int progress, const bool done);
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	const Style style_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	
	OptionsField options_mode {
		{ 10 * 8, 4 },
		10,
		{
			{ "Mode Xy.", 0 },
			{ "Mode EP.", 1 }
		}
	};
	
	Checkbox checkbox_speaker {
		{ 22 * 8, 4 },
		0,
		""
	};
	Image bmp_speaker {
		{ 204, 8, 16, 16 },
		&bitmap_icon_speaker,
		ui::Color::white(),
		ui::Color::black()
	};
	
	Text text_header {
		{ 8 * 8, 3 * 8, 7 * 8, 16 },
		"Header:"
	};
	NumberField header_code_a {
		{ 16 * 8, 3 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	NumberField header_code_b {
		{ 18 * 8, 3 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	
	Text text_city {
		{ 4 * 8, 5 * 8, 11 * 8, 16 },
		"Code ville:"
	};
	NumberField city_code_xy {
		{ 16 * 8, 5 * 8 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	NumberField city_code_ep {
		{ 16 * 8, 5 * 8 },
		3,
		{ 0, 255 },
		1,
		' '
	};
	
	Text text_family {
		{ 7 * 8, 7 * 8, 8 * 8, 16 },
		"Famille:"
	};
	NumberField family_code_xy {
		{ 16 * 8, 7 * 8 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	OptionsField family_code_ep {
		{ 16 * 8, 7 * 8 },
		2,
		{
			{ "A ", 2 },	// See receiver PCB
			{ "B ", 1 },
			{ "C ", 0 },
			{ "TP", 3 }
		}
	};
	
	Text text_subfamily {
		{ 2 * 8, 9 * 8 + 2, 13 * 8, 16 },
		"Sous-famille:"
	};
	NumberField subfamily_code {
		{ 16 * 8, 9 * 8 + 2 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	Checkbox checkbox_wcsubfamily {
		{ 20 * 8, 8 * 8 + 6 },
		6,
		"Toutes"
	};
	
	Text text_receiver {
		{ 2 * 8, 13 * 8, 13 * 8, 16 },
		"ID recepteur:"
	};
	NumberField receiver_code {
		{ 16 * 8, 13 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	Checkbox checkbox_wcid {
		{ 20 * 8, 12 * 8 + 4 },
		4,
		"Tous"
	};
	
	Text text_relais {
		{ 1 * 8, 8 * 16 + 8, 7 * 8, 16 },
		"Relais:"
	};
	
	std::array<ImageOptionsField, 4> relay_states { };
	
	ImageOptionsField::options_t relay_options = {
		{ &bulb_ignore_bmp[0], 0 },
		{ &bulb_off_bmp[0], 1 },
		{ &bulb_on_bmp[0], 2 }
	};
	
	ProgressBar progressbar {
		{ 5 * 8, 13 * 16, 20 * 8, 16 },
	};
	Text text_message {
		{ 5 * 8, 14 * 16, 20 * 8, 16 },
		""
	};
	
	Checkbox checkbox_cligno {
		{ 18 * 8 + 4, 10 * 16},
		3,
		"J/N"
	};
	NumberField tempo_cligno {
		{ 25 * 8 + 4, 10 * 16 + 4},
		2,
		{ 1, 99 },
		1,
		' '
	};
	Text text_cligno {
		{ 27 * 8 + 4, 10 * 16 + 4, 2 * 8, 16 },
		"s."
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
