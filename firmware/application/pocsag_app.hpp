/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __POCSAG_APP_H__
#define __POCSAG_APP_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"

#include "event_m0.hpp"

#include "log_file.hpp"

#include "pocsag.hpp"
#include "pocsag_packet.hpp"

class POCSAGLogger {
public:
	Optional<File::Error> append(const std::string& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const pocsag::POCSAGPacket& packet, const uint32_t frequency);
	void on_decoded(const pocsag::POCSAGPacket& packet,	const std::string text);

private:
	LogFile log_file { };
};

namespace ui {

class POCSAGAppView : public View {
public:
	POCSAGAppView(NavigationView& nav);
	~POCSAGAppView();

	void set_parent_rect(const Rect new_parent_rect) override;
	void focus() override;

	std::string title() const override { return "POCSAG RX"; };

private:
	static constexpr uint32_t initial_target_frequency = 466175000;
	static constexpr uint32_t sampling_rate = 3072000;
	//static constexpr uint32_t baseband_bandwidth = 1750000;

	bool logging { true };
	uint32_t last_address = 0xFFFFFFFF;
	pocsag::POCSAGState pocsag_state { };
	
	MessageHandlerRegistration message_handler_packet {
		Message::ID::POCSAGPacket,
		[this](Message* const p) {
			const auto message = static_cast<const POCSAGPacketMessage*>(p);
			this->on_packet(message);
		}
	};

	static constexpr ui::Dim header_height = 1 * 16;

	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};

	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};

	OptionsField options_freq {
		{ 0 * 8, 0 * 16 },
		7,
		{
			{ "Entered", 0 },
			{ "FR .025", 466025000 },
			{ "FR .050", 466050000 },
			{ "FR .075", 466075000 },
			{ "FR .175", 466175000 },
			{ "FR .206", 466206250 },
			{ "FR .231", 466231250 }
		}
	};
	
	Button button_setfreq {
		{ 0, 20, 11 * 8, 20 },
		"----.----"
	};
	OptionsField options_bitrate {
		{ 12 * 8, 22 },
		7,
		{
			{ "512bps ", 0 },
			{ "1200bps", 1 },
			{ "2400bps", 2 }
		}
	};
	Checkbox check_log {
		{ 20 * 8, 22 },
		3,
		"LOG",
		true
	};

	Console console {
		{ 0, 48, 240, 256 }
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

	std::unique_ptr<POCSAGLogger> logger { };

	uint32_t target_frequency_ = initial_target_frequency;
	
	void update_freq(rf::Frequency f);

	void on_packet(const POCSAGPacketMessage * message);
	void on_show_list();

	void on_band_changed(const uint32_t new_band_frequency);
	void on_bitrate_changed(const uint32_t new_bitrate);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);
};

} /* namespace ui */

#endif/*__POCSAG_APP_H__*/
