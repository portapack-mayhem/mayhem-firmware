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
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"

#include "log_file.hpp"

#include "pocsag.hpp"
#include "pocsag_packet.hpp"

class POCSAGLogger {
public:
	Optional<File::Error> append(const std::string& filename) {
		return log_file.append(filename);
	}
	
	void log_raw_data(const pocsag::POCSAGPacket& packet, const uint32_t frequency);
	void log_decoded(const pocsag::POCSAGPacket& packet, const std::string text);

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

	bool logging { true };
	bool ignore { false };
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

	OptionsField options_freq {
		{ 0 * 8, 0 * 16 },
		12,
		{
			{ "Entered", 0 },
			{ "BEL Fire", 169625000 },
			{ "DEU Fire", 448425000 },
			{ "DEU e-Msg1", 465970000 },
			{ "DEU e-Msg2", 466075000 },
			{ "FRA e-Msg1", 466025000 },
			{ "FRA e-Msg2", 466050000 },
			{ "FRA e-Msg3", 466075000 },
			{ "FRA e-Msg4", 466175000 },
			{ "FRA e-Msg5", 466206250 },
			{ "FRA e-Msg6", 466231250 },
			{ "FRA Fire", 173512500 },
			{ "NL Medical", 169650000 },
			{ "NL KPN 3 - Public", 17245000 },
			{ "SWE Minicall1", 169800000 },
			{ "SWE Minicall2", 161437500 },
			{ "USA Medical1", 152007500 },
			{ "USA Medical2", 157450000 },
			{ "USA Medical3", 163250000 }
		}
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
	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};
	
	Button button_setfreq {
		{ 0, 19, 11 * 8, 20 },
		"----.----"
	};
	OptionsField options_bitrate {
		{ 12 * 8, 21 },
		7,
		{
			{ "512bps ", 0 },
			{ "1200bps", 1 },
			{ "2400bps", 2 }
		}
	};
	Checkbox check_log {
		{ 22 * 8, 21 },
		3,
		"LOG",
		true
	};
	
	Checkbox check_ignore {
		{ 1 * 8, 40 },
		15,
		"Ignore address:",
		true
	};
	SymField sym_ignore {
		{ 19 * 8, 40 },
		7,
		SymField::SYMFIELD_DEC
	};

	Console console {
		{ 0, 4 * 16, 240, 240 }
	};

	std::unique_ptr<POCSAGLogger> logger { };

	uint32_t target_frequency_ = initial_target_frequency;
	
	void update_freq(rf::Frequency f);

	void on_packet(const POCSAGPacketMessage * message);

	void on_bitrate_changed(const uint32_t new_bitrate);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);
};

} /* namespace ui */

#endif/*__POCSAG_APP_H__*/
