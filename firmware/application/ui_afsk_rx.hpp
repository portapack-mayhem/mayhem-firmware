/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_AFSK_RX_H__
#define __UI_AFSK_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_widget.hpp"
#include "utility.hpp"

namespace ui {

class AFSKRxView : public View {
public:
	AFSKRxView(NavigationView& nav);
	~AFSKRxView();

	void focus() override;

	std::string title() const override { return "AFSK RX (beta)"; };
	
private:
	void on_data(uint_fast8_t byte);

	RFAmpField field_rf_amp {
		{ 13 * 8, 0 * 16 }
	};
	LNAGainField field_lna {
		{ 15 * 8, 0 * 16 }
	};
	VGAGainField field_vga {
		{ 18 * 8, 0 * 16 }
	};
	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};
	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};
	
	FrequencyField field_frequency {
		{ 0 * 8, 0 * 8 },
	};
	OptionsField options_bitrate {
		{ 12 * 8, 21 },
		7,
		{
			{ "600bps ", 600 },
			{ "1200bps", 1200 },
			{ "2400bps", 2400 }
		}
	};
	
	Console console {
		{ 0, 4 * 16, 240, 240 }
	};

	void update_freq(rf::Frequency f);

	void on_bitrate_changed(const uint32_t new_bitrate);

	void on_data_afsk(const AFSKDataMessage& message);
	
	MessageHandlerRegistration message_handler_packet {
		Message::ID::AFSKData,
		[this](Message* const p) {
			const auto message = static_cast<const AFSKDataMessage*>(p);
			this->on_data(message->byte);
		}
	};
};

} /* namespace ui */

#endif/*__UI_AFSK_RX_H__*/
