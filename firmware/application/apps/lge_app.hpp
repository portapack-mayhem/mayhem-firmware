/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2019 Furrtek
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
#include "rfm69.hpp"

#include "message.hpp"
#include "transmitter_model.hpp"
#include "portapack.hpp"
#include "app_settings.hpp"

namespace ui {

class LGEView : public View {
public:
	LGEView(NavigationView& nav);
	~LGEView();
	
	void focus() override;
	
	std::string title() const override { return "LGE tool TX"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		ALL
	};

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };
	
	tx_modes tx_mode = IDLE;
	
    	RFM69 rfm69 { 5, 0x2DD4, true, true };
    
	uint32_t frame_size { 0 };
	uint32_t repeats { 0 };
	uint32_t channel_index { 0 };
	std::string nickname { "ABCDEF" };
	
	rf::Frequency channels[3] = { 868067000, 868183000, 868295000 };

	void start_tx();
	void stop_tx();
	
	void generate_lge_frame(const uint8_t command, std::vector<uint8_t>& data) {
		generate_lge_frame(command, 0xFFFF, 0xFFFF, data);
	}
	void generate_lge_frame(const uint8_t command, const uint16_t address_a, const uint16_t address_b, std::vector<uint8_t>& data);
	void generate_frame_touche();
	void generate_frame_nickname();
	void generate_frame_team();
	void generate_frame_broadcast_nickname();
	void generate_frame_start();
	void generate_frame_gameover();
	void generate_frame_collier();
	
	void on_tx_progress(const uint32_t progress, const bool done);
	
	Labels labels {
		//{ { 7 * 8, 1 * 8 }, "NO FUN ALLOWED !", Color::red() },
		{ { 1 * 8, 1 * 8 }, "Frame:", Color::light_grey() },
		{ { 2 * 8, 3 * 8 }, "Room:", Color::light_grey() },
		{ { 14 * 8, 3 * 8 }, "Text:", Color::light_grey() },
		{ { 2 * 8, 5 * 8 }, "Team:", Color::light_grey() },
		{ { 0 * 8, 7 * 8 }, "Player:", Color::light_grey() },
		{ { 0 * 8, 10 * 8 }, "Vest:", Color::light_grey() },
		{ { 4 * 8, 12 * 8 }, "ID:", Color::light_grey() },
		{ { 3 * 8, 14 * 8 }, "Pow:  /10", Color::light_grey() },
		{ { 2 * 8, 16 * 8 }, "Time:  x100ms", Color::light_grey() }
	};
	
	OptionsField options_frame {
		{ 7 * 8, 1 * 8 },
		13,
		{
			{ "Key", 0 },
			{ "Set nickname", 1 },
			{ "Set team", 2 },
			{ "Brdcst nick", 3 },
			{ "Start", 4 },
			{ "Game over", 5 },
			{ "Set vest", 6 }
		}
	};
	
	Checkbox checkbox_channels {
		{ 20 * 8, 1 * 8 },
		7,
		"All ch.",
		true
	};
	
	NumberField field_room {
		{ 7 * 8, 3 * 8 },
		1,
		{ 1, 2 },
		1,
		'0'
	};
	
	Button button_text {
		{ 14 * 8, 5 * 8, 16 * 8, 3 * 8 },
		"ABCDEF"
	};
	
	NumberField field_team {
		{ 7 * 8, 5 * 8 },
		1,
		{ 1, 6 },
		1,
		'0'
	};
	
	NumberField field_player {
		{ 7 * 8, 7 * 8 },
		2,
		{ 1, 50 },
		1,
		'0'
	};
	
	Checkbox checkbox_heartbeat {
		{ 17 * 8, 12 * 8 },
		9,
		"Heartbeat",
		true
	};
	Checkbox checkbox_rxtick {
		{ 17 * 8, 15 * 8 },
		7,
		"RX tick",
		true
	};
	NumberField field_id {
		{ 7 * 8, 12 * 8 },
		1,
		{ 1, 2 },
		1,
		'0'
	};
	NumberField field_power {
		{ 7 * 8, 14 * 8 },
		2,
		{ 1, 10 },
		1,
		'0'
	};
	NumberField field_duration {
		{ 7 * 8, 16 * 8 },
		2,
		{ 1, 25 },
		1,
		'0'
	};
	
	Console console {
		{ 0, 18 * 8, 30 * 8, 7 * 16 }
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
