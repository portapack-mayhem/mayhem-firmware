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
#include "ui_tabview.hpp"
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

class XylosView : public View {
public:
	XylosView(Rect parent_rect);
	
	void focus() override;
	
	void flip_relays();
	void generate_message();
	
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	tx_modes tx_mode = IDLE;
	
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

private:
	Labels labels {
		{ { 8 * 8, 1 * 8 }, "Header:", Color::light_grey() },
		{ { 4 * 8, 3 * 8 }, "Code ville:", Color::light_grey() },
		{ { 7 * 8, 5 * 8 }, "Famille:", Color::light_grey() },
		{ { 2 * 8, 7 * 8 + 2 }, "Sous-famille:", Color::light_grey() },
		{ { 2 * 8, 11 * 8 }, "ID recepteur:", Color::light_grey() },
		{ { 2 * 8, 14 * 8 }, "Relais:", Color::light_grey() }
	};
	
	NumberField field_header_a {
		{ 16 * 8, 1 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	NumberField field_header_b {
		{ 18 * 8, 1 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	
	NumberField field_city {
		{ 16 * 8, 3 * 8 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	
	NumberField field_family {
		{ 16 * 8, 5 * 8 },
		1,
		{ 0, 9 },
		1,
		' '
	};

	NumberField field_subfamily {
		{ 16 * 8, 7 * 8 + 2 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	
	Checkbox checkbox_wcsubfamily {
		{ 20 * 8, 6 * 8 + 6 },
		6,
		"Toutes"
	};
	
	NumberField field_receiver {
		{ 16 * 8, 11 * 8 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	Checkbox checkbox_wcid {
		{ 20 * 8, 10 * 8 + 4 },
		4,
		"Tous"
	};
	
	std::array<ImageOptionsField, 4> relay_states { };
	
	ImageOptionsField::options_t relay_options = {
		{ &bitmap_bulb_ignore, 0 },
		{ &bitmap_bulb_off, 1 },
		{ &bitmap_bulb_on, 2 }
	};
	
	Button button_seq {
		{ 24 * 8, 1 * 8, 40, 32 },
		"Seq"
	};
};

class EPARView : public View {
public:
	EPARView(Rect parent_rect);
	
	void focus() override;
	
	void flip_relays();
	size_t generate_message();

	size_t half { 0 };
	
private:
	Labels labels {
		{ { 4 * 8, 1 * 8 }, "Code ville:", Color::light_grey() },
		{ { 8 * 8, 3 * 8 }, "Groupe:", Color::light_grey() },
		{ { 8 * 8, 7 * 8 }, "Relais:", Color::light_grey() }
	};
	
	NumberField field_city {
		{ 16 * 8, 1 * 8 },
		3,
		{ 0, 255 },
		1,
		' '
	};
	
	OptionsField field_group {
		{ 16 * 8, 3 * 8 },
		2,
		{
			{ "A ", 2 },	// See receiver PCB
			{ "B ", 1 },
			{ "C ", 0 },
			{ "TP", 3 }
		}
	};
	
	std::array<ImageOptionsField, 2> relay_states { };
	
	ImageOptionsField::options_t relay_options = {
		{ &bitmap_bulb_off, 0 },
		{ &bitmap_bulb_on, 1 }
	};
	
	Button button_scan {
		{ 22 * 8, 1 * 8, 56, 32 },
		"Scan"
	};
};

class BHTView : public View {
public:
	BHTView(NavigationView& nav);
	~BHTView();

	void focus() override;
	
	std::string title() const override { return "BHT transmit"; };

private:
	void on_tx_progress(const int progress, const bool done);
	void start_tx();
	
	enum tx_type_t {
		XYLOS = 0,
		EPAR = 1
	};
	
	tx_type_t tx_type = { };
	
	Rect view_rect = { 0, 3 * 8, 240, 192 };
	
	XylosView view_xylos { view_rect };
	EPARView view_EPAR { view_rect };
	
	TabView tab_view {
		{ "Xylos", Color::cyan(), &view_xylos },
		{ "EPAR", Color::green(), &view_EPAR }
	};
	
	Labels labels {
		{ { 29 * 8, 14 * 16 + 4 }, "s", Color::light_grey() }
	};
	
	ProgressBar progressbar {
		{ 1 * 8, 14 * 16, 20 * 8, 16 },
	};
	Text text_message {
		{ 1 * 8, 15 * 16, 20 * 8, 16 },
		""
	};
	
	Checkbox checkbox_cligno {
		{ 22 * 8, 14 * 16 },
		1,
		"~"
	};
	NumberField field_tempo {
		{ 27 * 8, 14 * 16 + 4 },
		2,
		{ 1, 99 },
		1,
		' '
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
