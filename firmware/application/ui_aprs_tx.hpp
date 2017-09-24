/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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
#include "ui_textentry.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_transmitter.hpp"

#include "message.hpp"
#include "transmitter_model.hpp"
#include "portapack.hpp"

namespace ui {

class APRSTXView : public View {
public:
	APRSTXView(NavigationView& nav);
	~APRSTXView();
	
	void paint(Painter& painter) override;
	
	void focus() override;
	
	std::string title() const override { return "APRS TX (beta)"; };

private:
	/*enum tx_modes {
		IDLE = 0,
		SINGLE,
		SEQUENCE
	};
	
	tx_modes tx_mode = IDLE;*/
	
	void start_tx();
	void generate_frame();
	void generate_frame_pos();
	void on_tx_progress(const uint32_t progress, const bool done);
	
	Labels labels {
		{ { 2 * 8, 2 * 8 }, "Work in progress...", Color::light_grey() }
	};
	
	Text text_frame_a {
		{ 2 * 8, 13 * 16, 14 * 8, 16 },
		"-"
	};
	
	TransmitterView tx_view {
		16 * 16,
		144800000,
		2000000,
		true
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
