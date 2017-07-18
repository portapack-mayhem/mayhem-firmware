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

#include "bht.hpp"
#include "bitmap.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"
#include "encoders.hpp"
#include "portapack.hpp"

#define XY_SEQ_COUNT 14

namespace ui {

class BHTView : public View {
public:
	BHTView(NavigationView& nav);
	~BHTView();

	void focus() override;
	
	std::string title() const override { return "BHT transmit"; };

private:
	uint32_t seq_index { 0 };
	
	struct sequence_t {
		const std::string code;
		const uint32_t delay;
	};

	const sequence_t sequence_matin[XY_SEQ_COUNT] = {
		{ "0000189000B1002B0000", 19 }, // 18:9:0:00	 R1=OFF (1)	
		{ "0000189200B2110B0000", 16 },	// 18:9:2:00	 R1=ON  (4)	
		{ "0000189200B1110B0000", 52 },	// 18:9:2:00	 R1=OFF (4)	
		{ "0000189200B2110B0000", 17 },	// 18:9:2:00	 R1=ON  (4)	
		{ "0000189200B1110B0000", 16 },	// 18:9:2:00	 R1=OFF (4)	
		{ "0000189000B0012B0000", 22 },	// 18:9:0:00	 R3=OFF (2)	
		{ "0000189200B1120B0000", 17 },	// 18:9:2:00	 R3=ON  (6)	
		{ "0000189200B1110B0000", 17 },	// 18:9:2:00	 R3=OFF (6)	
		{ "0000181AAAB1000B0000", 17 },	// 18:1:A:AA	 R1=OFF (10)	
		{ "0000189400B1000B0000", 17 },	// 18:9:4:00	 R1=OFF (7)	
		{ "0000189200B1120B0000", 14 },	// 18:9:2:00	 R3=ON  (6)	
		{ "0000189200B1110B0000", 17 },	// 18:9:2:00	 R3=OFF (6)	
		{ "0000181AAAB1000B0000", 17 },	// 18:1:A:AA	
		{ "0000189400B0100B0000", 17 }	// 18:9:4:00	 R2=OFF (8)		
	};
	
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	tx_modes tx_mode = IDLE;

	void generate_message();
	void on_tx_progress(const int progress, const bool done);
	void start_tx();
	
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
	
	Labels labels {
		{ { 8 * 8, 3 * 8 }, "Header:", Color::light_grey() },
		{ { 4 * 8, 5 * 8 }, "Code ville:", Color::light_grey() },
		{ { 7 * 8, 7 * 8 }, "Famille:", Color::light_grey() },
		{ { 2 * 8, 9 * 8 + 2 }, "Sous-famille:", Color::light_grey() },
		{ { 2 * 8, 13 * 8 }, "ID recepteur:", Color::light_grey() },
		{ { 1 * 8, 16 * 8 }, "Relais:", Color::light_grey() },
		{ { 27 * 8 + 4, 19 * 8 + 4 }, "s.", Color::light_grey() }
	};
	
	OptionsField options_mode {
		{ 10 * 8, 4 },
		10,
		{
			{ "Mode Xy.", 0 },
			{ "Mode EP.", 1 }
		}
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
	
	NumberField city_code_xy {
		{ 16 * 8, 5 * 8 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	/*NumberField city_code_ep {
		{ 16 * 8, 5 * 8 },
		3,
		{ 0, 255 },
		1,
		' '
	};*/
	
	NumberField family_code_xy {
		{ 16 * 8, 7 * 8 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	/*OptionsField family_code_ep {
		{ 16 * 8, 7 * 8 },
		2,
		{
			{ "A ", 2 },	// See receiver PCB
			{ "B ", 1 },
			{ "C ", 0 },
			{ "TP", 3 }
		}
	};*/
	
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
	
	std::array<ImageOptionsField, 4> relay_states { };
	
	ImageOptionsField::options_t relay_options = {
		{ &bitmap_bulb_ignore, 0 },
		{ &bitmap_bulb_off, 1 },
		{ &bitmap_bulb_on, 2 }
	};
	
	ProgressBar progressbar {
		{ 3 * 8, 12 * 16, 20 * 8, 16 },
	};
	Text text_message {
		{ 3 * 8, 13 * 16, 20 * 8, 16 },
		""
	};
	
	Checkbox checkbox_cligno {
		{ 18 * 8 + 4, 19 * 8 },
		3,
		"Alt"
	};
	NumberField tempo_cligno {
		{ 25 * 8 + 4, 19 * 8 + 4 },
		2,
		{ 1, 99 },
		1,
		' '
	};
	
	Button button_seq {
		{ 24 * 8, 13 * 16, 40, 32 },
		"Seq"
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
