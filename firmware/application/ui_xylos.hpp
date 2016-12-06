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
#include "ui_font_fixed_8x16.hpp"

#include "bmp_bulb_on.hpp"
#include "bmp_bulb_off.hpp"
#include "bmp_bulb_ignore.hpp"

#include "message.hpp"
//#include "volume.hpp"
#include "transmitter_model.hpp"
//#include "receiver_model.hpp"
#include "portapack.hpp"

#define CCIR_TONELENGTH (15360*2)-1		// 1536000/10/10

namespace ui {

/*
#define XYLOS_VOICE_ZERO 0
#define XYLOS_VOICE_HEADER 16
#define XYLOS_VOICE_RELAYS 21
#define XYLOS_VOICE_TRAILER 25

class XylosRXView : public View {
public:
	XylosRXView(NavigationView& nav);
	~XylosRXView();
	
	void talk();
	void on_show() override;
	void focus() override;
	
	Text text_dbg {
		{ 5 * 8, 14 * 16, 20 * 8, 16 },
		"--------------------"
	};
	
	void do_something(void *p);

private:
	uint8_t p_idx;
	const rf::Frequency xylos_freqs[7] = { 31325000, 31387500, 31437500, 31475000, 31687500, 31975000, 88000000 };
	uint8_t xylos_voice_phrase[32] = { 0xFF };
	const char * xylos_voice_filenames[26] = {	"zero.wav",
												"one.wav",
												"two.wav",
												"three.wav",
												"four.wav",
												"five.wav",
												"six.wav",
												"seven.wav",
												"eight.wav",
												"nine.wav",
												"a.wav",
												"b.wav",
												"c.wav",
												"d.wav",
												"e.wav",
												"f.wav",
												"header.wav",
												"city.wav",
												"family.wav",
												"subfamily.wav",
												"address.wav",
												"relays.wav",
												"ignored.wav",
												"off.wav",
												"on.wav",
												"trailer.wav"
										};
	char ccir_received[21];
	
	Text text_title {
		{ 1 * 8, 1 * 16, 11, 16 },
		"BH Xylos RX"
	};
	
	Text text_city {
		{ 4 * 8, 3 * 16, 11 * 8, 16 },
		"Code ville:"
	};
	NumberField city_code {
		{ 16 * 8, 3 * 16 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	
	Text text_freq {
		{ 5 * 8, 9 * 16, 10 * 8, 16 },
		"Frequence:"
	};
	OptionsField options_freq {
		{ 16 * 8, 9 * 16 },
		7,
		{
			{ "31.3250", 0 },
			{ "31.3875", 1 },
			{ "31.4375", 2 },
			{ "31.4750", 3 },
			{ "31.6875", 4 },
			{ "31.9750", 5 },
			{ "TEST 88", 6 }
		}
	};
	
	Button button_start {
		{ 2 * 8, 16 * 16, 64, 32 },
		"START"
	};
	
	Button button_exit {
		{ 21 * 8, 16 * 16, 64, 32 },
		"Exit"
	};
};*/

class XylosView : public View {
public:
	XylosView(NavigationView& nav);
	~XylosView();
	
	void focus() override;
	
	std::string title() const override { return "Xylos transmit"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE,
		TESTING
	};
	
	tx_modes tx_mode = IDLE;
	
	// 93.975
	// 94.1625
	// 94.3125
	// 94.425
	// 95.0625
	// 95.925
	const rf::Frequency xylos_freqs[7] = { 31325000, 31387500, 31437500, 31475000, 31687500, 31975000, 88000000 };
	
	char ccir_message[21];
	
	const char ccir_base[21] = "0000000000B0000B0000";
	
	const char xylos_sequence[9][21] = {
		"0E0E18920EB1E10B0E0E",
		"0E0E1890E0B0E12B0E0E",
		"0E0E18920EB1E20B0E0E",
		"0E0E18920EB1210B0E0E",
		"0E0E18920EB1E10B0E0E",
		"0E0E18920EB1E10B0E0E",
		"0E0E181AEAB10E0B0E0E",
		"0E01E81AEAB10E0B0E0E",	// 2016-05-22 05:22:29 0E01E81AEA/10E0/0E0E
		"0E03181AEAB10E0B0E0E"
	};
	
	unsigned int sequence_idx;
	
	void ascii_to_ccir(char *ascii);
	void start_tx();
	void generate_message();
	void on_txdone(const int n);
	
	const Style style_val {
		.font = font::fixed_8x16,
		.background = Color::green(),
		.foreground = Color::black(),
	};
	const Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::red(),
		.foreground = Color::black(),
	};
	const Style style_grey {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	Text text_header {
		{ 8 * 8, 1 * 16, 7 * 8, 16 },
		"Header:"
	};
	NumberField header_code_a {
		{ 16 * 8, 1 * 16 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	NumberField header_code_b {
		{ 18 * 8, 1 * 16 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	
	Text text_city {
		{ 4 * 8, 2 * 16, 11 * 8, 16 },
		"Code ville:"
	};
	NumberField city_code {
		{ 16 * 8, 2 * 16 },
		2,
		{ 0, 99 },
		1,
		' '
	};
	
	Text text_family {
		{ 7 * 8, 3 * 16, 8 * 8, 16 },
		"Famille:"
	};
	NumberField family_code {
		{ 16 * 8, 3 * 16 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	
	Text text_subfamily {
		{ 2 * 8, 4 * 16 + 4, 13 * 8, 16 },
		"Sous-famille:"
	};
	NumberField subfamily_code {
		{ 16 * 8, 4 * 16 + 4 },
		1,
		{ 0, 9 },
		1,
		' '
	};
	Checkbox checkbox_wcsubfamily {
		{ 20 * 8, 4 * 16},
		6,
		"Toutes"
	};
	
	Text text_receiver {
		{ 2 * 8, 6 * 16, 13 * 8, 16 },
		"ID recepteur:"
	};
	NumberField receiver_code {
		{ 16 * 8, 6 * 16 },
		2,
		{ 0, 99 },
		1,
		'0'
	};
	Checkbox checkbox_wcid {
		{ 20 * 8, 6 * 16 },
		4,
		"Tous"
	};
	
	Text text_freq {
		{ 6 * 8, 8 * 16, 10 * 8, 16 },
		"Frequence:"
	};
	OptionsField options_freq {
		{ 17 * 8, 8 * 16},
		7,
		{
			{ "31.3250", 0 },
			{ "31.3875", 1 },
			{ "31.4375", 2 },
			{ "31.4750", 3 },
			{ "31.6875", 4 },
			{ "31.9750", 5 },
			{ "TEST 88", 6 }
		}
	};
	
	Text text_relais {
		{ 8, 9 * 16 + 4, 7 * 8, 16 },
		"Relais:"
	};
	
	ImageOptionsField options_ra {
		{ 26, 166, 24, 24 },
		{
			{ &bulb_ignore_bmp[0], 0 },
			{ &bulb_off_bmp[0], 1 },
			{ &bulb_on_bmp[0], 2 }
		}
	};
	ImageOptionsField options_rb {
		{ 79, 166, 24, 24 },
		{
			{ &bulb_ignore_bmp[0], 0 },
			{ &bulb_off_bmp[0], 1 },
			{ &bulb_on_bmp[0], 2 }
		}
	};
	ImageOptionsField options_rc {
		{ 133, 166, 24, 24 },
		{
			{ &bulb_ignore_bmp[0], 0 },
			{ &bulb_off_bmp[0], 1 },
			{ &bulb_on_bmp[0], 2 }
		}
	};
	ImageOptionsField options_rd {
		{ 186, 166, 24, 24 },
		{
			{ &bulb_ignore_bmp[0], 0 },
			{ &bulb_off_bmp[0], 1 },
			{ &bulb_on_bmp[0], 2 }
		}
	};
	
	ProgressBar progress {
		{ 5 * 8, 13 * 16, 20 * 8, 16 },
	};
	Text text_message {
		{ 5 * 8, 14 * 16, 20 * 8, 16 },
		"--------------------"
	};
	
	Button button_transmit {
		{ 2 * 8, 16 * 16, 64, 32 },
		"START"
	};
	
	Checkbox checkbox_cligno {
		{ 96, 16 * 16},
		3,
		"J/N"
	};
	NumberField tempo_cligno {
		{ 104, 16 * 16 + 28 },
		2,
		{ 1, 99 },
		1,
		' '
	};
	Text text_cligno {
		{ 104 + 16, 16 * 16 + 28, 2 * 8, 16 },
		"s."
	};
	
	Button button_txtest {
		{ 20 * 8, 16 * 16, 64, 32 },
		"TEST"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_txdone(message.n);
		}
	};
};

} /* namespace ui */
