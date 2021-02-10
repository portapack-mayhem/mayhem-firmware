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
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"
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
	std::string buffer { "PORTAPACK" };
	std::string message { };
	NavigationView& nav_;

	BCHCode BCH_code {
		{ 1, 0, 1, 0, 0, 1 },
		5, 31, 21, 2
	};
	
	void on_set_text(NavigationView& nav);
	void on_tx_progress(const uint32_t progress, const bool done);
	bool start_tx();
	
	Labels labels {
		{ { 3 * 8, 4 * 8 }, "Bitrate:", Color::light_grey() },
		{ { 3 * 8, 6 * 8 }, "Address:", Color::light_grey() },
		{ { 6 * 8, 8 * 8 }, "Type:", Color::light_grey() },
		{ { 2 * 8, 10 * 8 }, "Function:", Color::light_grey() },
		{ { 5 * 8, 12 * 8 }, "Phase:", Color::light_grey() },
		{ { 0 * 8, 14 * 8 }, "Message:", Color::light_grey() }
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

	SymField field_address {
		{ 11 * 8, 6 * 8 },
		7,
		SymField::SYMFIELD_DEC
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
	
	OptionsField options_function {
		{ 11 * 8, 10 * 8 },
		1,
		{
			{ "A", 0 },
			{ "B", 1 },
			{ "C", 2 },
			{ "D", 3 }
		}
	};

	OptionsField options_phase {
		{ 11 * 8, 12 * 8 },
		1,
		{
			{ "P", 0 },
			{ "N", 1 },
		}
	};
	
	Text text_message {
		{ 0 * 8, 16 * 8, 16 * 8, 16 },
		""
	};
	
	Button button_message {
		{ 0 * 8, 18 * 8, 14 * 8, 32 },
		"Set message"
	};
	
	ProgressBar progressbar {
		{ 16, 200, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		9
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */

#endif/*__POCSAG_TX_H__*/
