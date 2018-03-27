/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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
#include "ui_transmitter.hpp"
#include "transmitter_model.hpp"

namespace ui {

class RemoteView : public View {
public:
	RemoteView(NavigationView& nav);
	~RemoteView();
	
	void focus() override;
	
	std::string title() const override { return "Custom remote"; };

private:
	/*enum tx_modes {
		IDLE = 0,
		SINGLE,
		SCAN
	};
	
	tx_modes tx_mode = IDLE;

	struct remote_layout_t {
		Point position;
		std::string text;
	};
	
	const std::array<remote_layout_t, 32> remote_layout { };*/
	
	Labels labels {
		{ { 1 * 8, 0 }, "Work in progress...", Color::light_grey() }
	};
	
	Button button {
		{ 60, 64, 120, 32 },
		"Exit"
	};
};

} /* namespace ui */
