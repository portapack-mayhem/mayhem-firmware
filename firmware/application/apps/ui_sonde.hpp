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
#include "ui_geomap.hpp"

#include "event_m0.hpp"

#include "log_file.hpp"

#include "sonde_packet.hpp"

#include <cstddef>
#include <string>

class SondeLogger {
public:
	Optional<File::Error> append(const std::filesystem::path& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const sonde::Packet& packet);

private:
	LogFile log_file { };
};

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
	std::unique_ptr<SondeLogger> logger { };
	uint32_t target_frequency_ { 402700000 };
	bool logging { false };
	sonde::GPS_data gps_info;
	std::string sonde_id;
	
	Labels labels {
		{ { 0 * 8, 2 * 16 }, "Signature:", Color::light_grey() },
		{ { 3 * 8, 3 * 16 }, "Serial:", Color::light_grey() },
		{ { 4 * 8, 4 * 16 }, "Vbatt:", Color::light_grey() }
	};

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
	
	Checkbox check_log {
		{ 22 * 8, 3 * 16 },
		3,
		"Log"
	};
	
	Text text_signature {
		{ 10 * 8, 2 * 16, 10 * 8, 16 },
		"..."
	};
	Text text_serial {
		{ 10 * 8, 3 * 16, 11 * 8, 16 },
		"..."
	};
	Text text_voltage {
		{ 10 * 8, 4 * 16, 10 * 8, 16 },
		"..."
	};
	
	GeoPos geopos {
		{ 0, 6 * 16 },
		GeoPos::alt_unit::METERS
	};
	
	Button button_see_map {
		{ 8 * 8, 10 * 16, 14 * 8, 3 * 16 },
		"See on map"
	};

	MessageHandlerRegistration message_handler_packet {
		Message::ID::SondePacket,
		[this](Message* const p) {
			const auto message = static_cast<const SondePacketMessage*>(p);
			const sonde::Packet packet { message->packet, message->type };
			this->on_packet(packet);
		}
	};

	void on_packet(const sonde::Packet& packet);
	void set_target_frequency(const uint32_t new_value);
	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__UI_SONDE_H__*/
