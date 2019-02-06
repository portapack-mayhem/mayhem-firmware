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

#include "message.hpp"
#include "transmitter_model.hpp"
#include "portapack.hpp"

namespace ui {

class LGEView : public View {
public:
	LGEView(NavigationView& nav);
	~LGEView();
	
	void focus() override;
	
	std::string title() const override { return "LGE tool"; };

private:
	enum tx_modes {
		IDLE = 0,
		SINGLE,
		ALL
	};
	
	tx_modes tx_mode = IDLE;
	
	uint32_t frame_size { 0 };
	uint32_t repeats { 0 };
	uint32_t channel_index { 0 };
	
	rf::Frequency channels[3] = { 868067000, 868183000, 868295000 };

	void start_tx();
	void stop_tx();
	void generate_frame();
	void on_tx_progress(const uint32_t progress, const bool done);
	
	Labels labels {
		{ { 7 * 8, 4 * 8 }, "NO FUN ALLOWED !", Color::red() },
		{ { 11 * 8, 8 * 8 }, "Zone:", Color::light_grey() }
	};
	
	NumberField field_zone {
		{ 16 * 8, 8 * 8 },
		1,
		{ 1, 6 },
		16,
		'0'
	};
	
	Checkbox checkbox_channels {
		{ 7 * 8, 14 * 8 },
		12,
		"All channels"
	};
	
	Text text_messagea {
		{ 0 * 8, 10 * 16, 30 * 8, 16 },
		""
	};
	Text text_messageb {
		{ 0 * 8, 11 * 16, 30 * 8, 16 },
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
