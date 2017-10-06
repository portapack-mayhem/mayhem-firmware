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

#ifndef __UI_SONDE_H__
#define __UI_SONDE_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"

#include "event_m0.hpp"

#include "log_file.hpp"

#include "sonde_packet.hpp"

#include "recent_entries.hpp"

#include <cstddef>
#include <string>

/*class SondeLogger {
public:
	Optional<File::Error> append(const std::filesystem::path& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const sonde::Packet& packet);

private:
	LogFile log_file { };
};*/

namespace ui {

class SondeView : public View {
public:
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	SondeView(NavigationView& nav);
	~SondeView();

	void focus() override;

	std::string title() const override { return "Radiosonde RX"; };

private:
	//std::unique_ptr<SondeLogger> logger { };
	uint32_t target_frequency_ { };

	FrequencyField field_frequency {
		{ 0 * 8, 0 * 8 },
	};
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
	
	Text text_debug {
		{ 0, 32, 240, 16 },
		"Waiting for frame..."
	};

	MessageHandlerRegistration message_handler_packet {
		Message::ID::SondePacket,
		[this](Message* const p) {
			const auto message = static_cast<const SondePacketMessage*>(p);
			//const sonde::Packet packet { message->type, message->packet };
			//this->on_packet(packet);
			this->on_packet(message->packet);
		}
	};

	//void on_packet(const sonde::Packet& packet);
	void on_packet(const baseband::Packet& packet);
	void set_target_frequency(const uint32_t new_value);
	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__UI_SONDE_H__*/
