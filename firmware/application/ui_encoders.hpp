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
#include "ui_receiver.hpp"
#include "encoders.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"

using namespace encoders;

namespace ui {

class EncodersView : public View {
public:
	EncodersView(NavigationView& nav);
	~EncodersView();
	
	EncodersView(const EncodersView&) = delete;
	EncodersView(EncodersView&&) = delete;
	EncodersView& operator=(const EncodersView&) = delete;
	EncodersView& operator=(EncodersView&&) = delete;
	
	void focus() override;
	void on_show() override;
	
	std::string title() const override { return "Encoders TX"; };

private:
	void on_tuning_frequency_changed(rf::Frequency f);

	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	uint8_t enc_type = 0;
	const encoder_def_t * encoder_def;
	tx_modes tx_mode = IDLE;
	//bool abort_scan = false;
	//uint8_t scan_count;
	//double scan_progress;
	//unsigned int scan_index;
	std::string debug_text = "0";
	uint8_t repeat_index;
	int8_t waveform_buffer[512];
	
	void draw_waveform();
	void on_bitfield();
	void on_type_change(size_t index);
	void generate_frame();
	void update_progress();
	void start_tx(const bool scan);
	void on_txdone(int n, const bool txdone);
	
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
	
	const Style style_address {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	const Style style_data {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::blue(),
	};
	
	FrequencyField field_frequency {
		{ 1 * 8, 4 },
	};
	
	Text text_enctype {
		{ 1 * 8, 32, 5 * 8, 16 },
		"Type:"
	};
	OptionsField options_enctype {		// Options are loaded at runtime
		{ 6 * 8, 32 },
		7,
		{
		}
	};
	
	Text text_clk {
		{ 16 * 8, 4 * 8, 4 * 8, 16 },
		"Clk:"
	};
	NumberField numberfield_clk {
		{ 21 * 8, 4 * 8 },
		3,
		{ 1, 500 },
		1,
		' '
	};
	Text text_kHz {
		{ 24 * 8, 4 * 8, 3 * 8, 16 },
		"kHz"
	};
	
	Text text_bitduration {
		{ 16 * 8, 6 * 8, 4 * 8, 16 },
		"Bit:"
	};
	NumberField numberfield_bitduration {
		{ 21 * 8, 6 * 8 },
		4,
		{ 50, 9999 },
		1,
		' '
	};
	Text text_us1 {
		{ 25 * 8, 6 * 8, 2 * 8, 16 },
		"us"
	};
	
	Text text_wordduration {
		{ 15 * 8, 8 * 8, 5 * 8, 16 },
		"Word:"
	};
	NumberField numberfield_wordduration {
		{ 21 * 8, 8 * 8 },
		5,
		{ 300, 99999 },
		100,
		' '
	};
	Text text_us2 {
		{ 26 * 8, 8 * 8, 2 * 8, 16 },
		"us"
	};
	
	Text text_symfield {
		{ 2 * 8, 10 * 8, 5 * 8, 16 },
		"Word:"
	};
	SymField symfield_word {
		{ 2 * 8, 12 * 8 },
		20
	};
	Text text_format {
		{ 2 * 8, 14 * 8, 24 * 8, 16 },
		""
	};
	
	//Text text_format_a;	// DEBUG
	//Text text_format_d;	// DEBUG
	
	Text text_waveform {
		{ 1 * 8, 136, 9 * 8, 16 },
		"Waveform:"
	};
	
	Waveform waveform {
		{ 0, 160, 240, 32 },
		waveform_buffer,
		0,
		0,
		true,
		Color::yellow()
	};
	
	Text text_status {
		{ 2 * 8, 224, 128, 16 },
		"Ready"
	};
	ProgressBar progress {
		{ 16, 224 + 20, 208, 16 }
	};
	
	Button button_transmit {
		{ 16, 270, 80, 32 },
		"TX"
	};
	
	MessageHandlerRegistration message_handler_tx_done {
		Message::ID::TXDone,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXDoneMessage*>(p);
			this->on_txdone(message.progress, message.done);
		}
	};
};

} /* namespace ui */
