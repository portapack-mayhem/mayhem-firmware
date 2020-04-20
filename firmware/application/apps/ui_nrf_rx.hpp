/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
 * Copyright (C) 2020 Shao
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

#ifndef __UI_NRF_RX_H__
#define __UI_NRF_RX_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_record_view.hpp"	// DEBUG

#include "utility.hpp"

namespace ui {

class NRFRxView : public View {
public:
	NRFRxView(NavigationView& nav);
	~NRFRxView();

	void focus() override;

	std::string title() const override { return "NRF RX"; };
	
private:
	void on_data(uint32_t value, bool is_data);
	
	uint8_t console_color { 0 };
	uint32_t prev_value { 0 };
	std::string str_log { "" };

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
		{ 0 * 8, 0 * 16 },
	};
	
	Text text_debug {
		{ 0 * 8, 1 * 16, 10 * 8, 16 },
		"DEBUG"
	};
	
	
	Button button_modem_setup {
		{ 12 * 8, 1 * 16, 96, 24 },
		"Modem setup"
	};
	
	// DEBUG
	RecordView record_view {
		{ 0 * 8, 3 * 16, 30 * 8, 1 * 16 },
		u"AFS_????", RecordView::FileType::WAV, 4096, 4
	};
	
	Console console {
		{ 0, 4 * 16, 240, 240 }
	};

	void update_freq(rf::Frequency f);
	//void on_data_afsk(const AFSKDataMessage& message);
	
	MessageHandlerRegistration message_handler_packet {
		Message::ID::AFSKData,
		[this](Message* const p) {
			const auto message = static_cast<const AFSKDataMessage*>(p);
			this->on_data(message->value, message->is_data);
		}
	};
};

} /* namespace ui */

#endif/*__UI_NRF_RX_H__*/
