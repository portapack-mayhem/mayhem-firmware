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

#ifndef __UI_TEST_H__
#define __UI_TEST_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"

#include "event_m0.hpp"

#include "test_packet.hpp"
#include "log_file.hpp"

#include <cstddef>
#include <string>

class TestLogger {
public:
	Optional<File::Error> append(const std::string& filename) {
		return log_file.append(filename);
	}
	
	void log_raw_data(const testapp::Packet& packet, const int32_t alt);

private:
	LogFile log_file { };
};

namespace ui {

class TestView : public View {
public:
	static constexpr uint32_t sampling_rate = 2457600*2;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	TestView(NavigationView& nav);
	~TestView();

	void focus() override;

	std::string title() const override { return "Test app"; };

private:
	uint32_t target_frequency_ { 439206000 };
	Coord cur_x { 0 };
	uint32_t packet_count { 0 };
	uint32_t packets_lost { 0 };
	uint32_t prev_v { 0 };
	uint32_t raw_alt { 0 };
	uint32_t cal_value { 0 };
	bool logging { false };
	
	Labels labels {
		{ { 0 * 8, 1 * 16 }, "Data:", Color::light_grey() }
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
	
	Text text_debug_a {
		{ 0 * 8, 4 * 16, 30 * 8, 16 },
		"..."
	};
	Text text_debug_b {
		{ 0 * 8, 5 * 16, 30 * 8, 16 },
		"..."
	};
	
	Button button_cal {
		{ 17 * 8, 2 * 16, 5 * 8, 2 * 16 },
		"CAL"
	};
	Checkbox check_log {
		{ 23 * 8, 2 * 16 },
		3,
		"LOG"
	};
	
	std::unique_ptr<TestLogger> logger { };

	MessageHandlerRegistration message_handler_packet {
		Message::ID::TestAppPacket,
		[this](Message* const p) {
			const auto message = static_cast<const TestAppPacketMessage*>(p);
			const testapp::Packet packet { message->packet };
			this->on_packet(packet);
		}
	};

	void on_packet(const testapp::Packet& packet);
	void set_target_frequency(const uint32_t new_value);
	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__UI_TEST_H__*/
