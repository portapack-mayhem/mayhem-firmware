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
#include "app_settings.hpp"
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

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	bool logging { true };
	bool ignore { true };
	uint32_t last_address = 0xFFFFFFFF;
	pocsag::POCSAGState pocsag_state { };

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
	Audio audio{
		{ 21 * 8, 10, 6 * 8, 4 },
	};
	
	FrequencyField field_frequency {
		{ 0 * 8, 0 * 8 },
	};
	Checkbox check_log {
		{ 24 * 8, 21 },
		3,
		"LOG",
		true
	};
	NumberField field_volume{
		{ 28 * 8, 0 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};
	
	Checkbox check_ignore {
		{ 1 * 8, 21 },
		12,
		"Ignore addr:",
		true
	};
	SymField sym_ignore {
		{ 16 * 8, 21 },
		7,
		SymField::SYMFIELD_DEC
	};

	Console console {
		{ 0, 3 * 16, 240, 256 }
	};

	std::unique_ptr<POCSAGLogger> logger { };

	uint32_t target_frequency_ = initial_target_frequency;
	
	void update_freq(rf::Frequency f);

	void on_packet(const POCSAGPacketMessage * message);

	void on_headphone_volume_changed(int32_t v);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);
	
	MessageHandlerRegistration message_handler_packet {
		Message::ID::POCSAGPacket,
		[this](Message* const p) {
			const auto message = static_cast<const POCSAGPacketMessage*>(p);
			this->on_packet(message);
		}
	};
};

} /* namespace ui */

#endif/*__POCSAG_APP_H__*/
