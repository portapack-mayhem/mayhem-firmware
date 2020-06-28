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

#include "message.hpp"
#include "transmitter_model.hpp"
#include "portapack.hpp"

namespace ui {

class CoasterPagerView : public View {
public:
	CoasterPagerView(NavigationView& nav);
	~CoasterPagerView();
	
	void focus() override;
	
	std::string title() const override { return "Si44xx TX"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	tx_modes tx_mode = IDLE;
	
	void start_tx();
	void generate_frame();
	void on_tx_progress(const uint32_t progress, const bool done);
	
	Labels labels {
		{ { 1 * 8, 3 * 8 }, "Syscall pager TX beta", Color::light_grey() },
		{ { 1 * 8, 8 * 8 }, "Data:", Color::light_grey() }
	};
	
	SymField sym_data {
		{ 7 * 8, 8 * 8 },
		16,		// 14 ? 12 ?
		SymField::SYMFIELD_HEX
	};
	
	Checkbox checkbox_scan {
		{ 10 * 8, 14 * 8 },
		4,
		"Scan"
	};
	
	/*ProgressBar progressbar {
		{ 5 * 8, 12 * 16, 20 * 8, 16 },
	};*/
	Text text_message {
		{ 5 * 8, 13 * 16, 20 * 8, 16 },
		""
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		12
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
