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

#ifndef __ACARS_APP_H__
#define __ACARS_APP_H__

#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"

#include "log_file.hpp"

#include "acars_packet.hpp"

class ACARSLogger {
public:
	Optional<File::Error> append(const std::string& filename) {
		return log_file.append(filename);
	}
	
	void log_raw_data(const acars::Packet& packet, const uint32_t frequency);
	//void log_decoded(const acars::Packet& packet, const std::string text);

private:
	LogFile log_file { };
};

namespace ui {

class ACARSAppView : public View {
public:
	ACARSAppView(NavigationView& nav);
	~ACARSAppView();

	void focus() override;

	std::string title() const override { return "ACARS (WIP)"; };

private:
	bool logging { false };
	uint32_t packet_counter { 0 };

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
	Checkbox check_log {
		{ 22 * 8, 21 },
		3,
		"LOG",
		true
	};

	Console console {
		{ 0, 3 * 16, 240, 256 }
	};

	std::unique_ptr<ACARSLogger> logger { };

	uint32_t target_frequency_ { };
	
	void update_freq(rf::Frequency f);

	void on_packet(const acars::Packet& packet);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);
	
	MessageHandlerRegistration message_handler_packet {
		Message::ID::ACARSPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ACARSPacketMessage*>(p);
			const acars::Packet packet { message->packet };
			this->on_packet(packet);
		}
	};

};

} /* namespace ui */

#endif/*__ACARS_APP_H__*/
