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

#ifndef __POCSAG_TX_H__
#define __POCSAG_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
#include "ui_textentry.hpp"
#include "bch_code.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"
#include "pocsag.hpp"

using namespace pocsag;

namespace ui {

class POCSAGTXView : public View {
public:
	POCSAGTXView(NavigationView& nav);
	~POCSAGTXView();
	
	POCSAGTXView(const POCSAGTXView&) = delete;
	POCSAGTXView(POCSAGTXView&&) = delete;
	POCSAGTXView& operator=(const POCSAGTXView&) = delete;
	POCSAGTXView& operator=(POCSAGTXView&&) = delete;
	
	void focus() override;
	void paint(Painter&) override;
	
	std::string title() const override { return "POCSAG TX"; };

private:
	char buffer[17] = "PORTAPACK";
	std::string message { };
	NavigationView& nav_;

	BCHCode BCH_code {
		{ 1, 0, 1, 0, 0, 1 },
		5, 31, 21, 2
	};
	
	void on_set_text(NavigationView& nav);
	void on_tx_progress(const int progress, const bool done);
	bool start_tx();
	
	Text text_bitrate {
		{ 3 * 8, 4 * 8, 8 * 8, 16 },
		"Bitrate:"
	};
	OptionsField options_bitrate {
		{ 11 * 8, 4 * 8 },
		8,
		{
			{ "512 bps ", 0 },
			{ "1200 bps", 1 },
			{ "2400 bps", 2 }
		}
	};
	
	Text text_address {
		{ 3 * 8, 6 * 8, 8 * 8, 16 },
		"Address:"
	};
	SymField field_address {
		{ 11 * 8, 6 * 8 },
		7,
		SymField::SYMFIELD_DEC
	};
	
	Text text_type {
		{ 6 * 8, 8 * 8, 5 * 8, 16 },
		"Type:"
	};
	OptionsField options_type {
		{ 11 * 8, 8 * 8 },
		12,
		{
			{ "Address only", MessageType::ADDRESS_ONLY },
			{ "Numeric only", MessageType::NUMERIC_ONLY },
			{ "Alphanumeric", MessageType::ALPHANUMERIC }
		}
	};
	
	Text text_message {
		{ 3 * 8, 12 * 8, 16 * 8, 16 },
		""
	};
	Button button_message {
		{ 3 * 8, 14 * 8, 8 * 8, 28 },
		"Set"
	};
	
	ProgressBar progressbar {
		{ 16, 200, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		9
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

#endif/*__POCSAG_TX_H__*/
