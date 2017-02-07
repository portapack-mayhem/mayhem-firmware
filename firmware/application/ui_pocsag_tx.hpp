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
#include "bch_code.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {

class POCSAGTXView : public View {
public:
	POCSAGTXView(NavigationView& nav);
	~POCSAGTXView();
	
	/*POCSAGTXView(const EncodersView&) = delete;
	POCSAGTXView(EncodersView&&) = delete;
	POCSAGTXView& operator=(const EncodersView&) = delete;
	POCSAGTXView& operator=(EncodersView&&) = delete;*/
	
	void focus() override;
	
	std::string title() const override { return "POCSAG TX"; };

private:

	BCHCode BCH_code {
		{ 1, 0, 1, 0, 0, 1 },
		5, 31, 21, 2
	};
	
	void on_tx_progress(const int progress, const bool done);
	void start_tx();
	
	Text text_debug_a {
		{ 1 * 8, 4 * 8, 20 * 8, 16 },
		"-"
	};
	Text text_debug_b {
		{ 1 * 8, 6 * 8, 20 * 8, 16 },
		"-"
	};
	Text text_debug_c {
		{ 1 * 8, 12 * 8, 20 * 8, 16 },
		"Address:"
	};
	
	SymField address_field {
		{ 9 * 8, 12 * 8 },
		7,
		SymField::SYMFIELD_DEC
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
