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
#include "app_settings.hpp"
#include "portapack.hpp"

namespace ui {

class APRSTXView : public View {
public:
	APRSTXView(NavigationView& nav);
	~APRSTXView();
	
	void focus() override;
	
	std::string title() const override { return "APRS TX"; };

private:
	
	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	std::string payload { "" };
	
	void start_tx();
	void generate_frame();
	void generate_frame_pos();
	void on_tx_progress(const uint32_t progress, const bool done);
	
	Labels labels {
		{ { 0 * 8, 1 * 16 }, "Source:       SSID:", Color::light_grey() },	// 6 alphanum + SSID
		{ { 0 * 8, 2 * 16 }, " Dest.:       SSID:", Color::light_grey() },
		{ { 0 * 8, 4 * 16 }, "Info field:", Color::light_grey() },
	};
	
	SymField sym_source {
		{ 7 * 8, 1 * 16 },
		6,
		SymField::SYMFIELD_ALPHANUM
	};
	NumberField num_ssid_source {
		{ 19 * 8, 1 * 16 },
		2,
		{ 0, 15 },
		1,
		' '
	};
	
	SymField sym_dest {
		{ 7 * 8, 2 * 16 },
		6,
		SymField::SYMFIELD_ALPHANUM
	};
	
	NumberField num_ssid_dest {
		{ 19 * 8, 2 * 16 },
		2,
		{ 0, 15 },
		1,
		' '
	};
	
	Text text_payload {
		{ 0 * 8, 5 * 16, 30 * 8, 16 },
		"-"
	};
	Button button_set {
		{ 0 * 8, 6 * 16, 80, 32 },
		"Set"
	};
	
	TransmitterView tx_view {
		16 * 16,
		5000,
		0 // disable setting bandwith, since APRS used fixed 10k bandwidth
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
