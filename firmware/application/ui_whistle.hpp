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
#include "ui_painter.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {

class WhistleView : public View {
public:
	WhistleView(NavigationView& nav);
	~WhistleView();
	
	void focus() override;
	
	std::string title() const override { return "Whistle"; };

private:
	void start_tx();
	void on_tx_progress(const bool done);

	enum tx_modes {
		IDLE = 0,
		SINGLE
	};
	
	tx_modes tx_mode = IDLE;

	Labels labels {
		{ { 3 * 8, 4 * 8 }, "Tone frequency:     Hz", Color::light_grey() },
		{ { 22 * 8, 8 * 8 + 4 }, "s.", Color::light_grey() }
	};
	
	NumberField field_tone {
		{ 19 * 8, 4 * 8 },
		4,
		{ 1, 9999 },
		5,
		' '
	};
	
	Checkbox checkbox_stop {
		{ 5 * 8, 8 * 8 },
		10,
		"Stop after"
	};
	
	NumberField field_stop {
		{ 20 * 8, 8 * 8 + 4 },
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
			this->on_tx_progress(message.done);
		}
	};
};

} /* namespace ui */
