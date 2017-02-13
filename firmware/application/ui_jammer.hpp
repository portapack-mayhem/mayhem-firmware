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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "ui_navigation.hpp"
#include "transmitter_model.hpp"
#include "message.hpp"
#include "jammer.hpp"

namespace ui {

class JammerView : public View {
public:
	JammerView(NavigationView& nav);
	~JammerView();
	
	void focus() override;
	
	std::string title() const override { return "Jammer"; };

private:
	typedef struct jammer_range {
		bool enabled;
		rf::Frequency min;
		rf::Frequency max;
	} jammer_range_t;

	jammer_range_t frequency_range[3];
	
	void update_range(const uint32_t n);
	void update_button(const uint32_t n);
	void on_retune(const rf::Frequency freq, const uint32_t range);
	
	const jammer_range_t range_presets[23][3] = {
		// Orange
		{{ true, 935000000, 945000000 },	// GSM 900		BW:10M
		{ false, 1808000000, 1832000000 },	// GSM 1800		BW:24M
		{ false, 0, 0 }},
		
		// SFR
		{{ true, 950000000, 960000000 },	// GSM 900		BW:10M
		{ false, 1832000000, 1853000000 },	// GSM 1800		BW:21M
		{ false, 0, 0 }},
		
		// Bouygues
		{{ true, 925000000, 935000000 },	// GSM 900		BW:10M
		{ false, 1858000000, 1880000000 },	// GSM 1800		BW:22M
		{ false, 0, 0 }},
		
		// Free
		{{ true, 945000000, 950000000 },	// GSM 900		BW:5M
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// GSM-R
		{{ true, 921000000, 925000000 },	// GSM 900		BW:4M
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// DECT
		{{ true, 1880000000, 1900000000 },	// BW: 20MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// PMV AFSK
		{{ true, 162930000, 162970000 },	// BW: 40kHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// ISM 433
		{{ true, 433050000, 434790000 },	// Center: 433.92MHz BW: 0.2%
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// ISM 868
		{{ true, 868000000, 868200000 },	// Center: 868.2MHz BW: 40kHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// GPS L1 & L2
		{{ true, 1575420000 - 500000, 1575420000 + 500000 },	// BW: 1MHz
		{ false, 1227600000 - 1000000, 1227600000 + 1000000 },	// BW: 2MHz
		{ false, 0, 0 }},
		
		// WLAN 2.4G CH1
		{{ true, 2412000000 - 11000000, 2412000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH2
		{{ true, 2417000000 - 11000000, 2417000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH3
		{{ true, 2422000000 - 11000000, 2422000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH4
		{{ true, 2427000000 - 11000000, 2427000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH5
		{{ true, 2432000000 - 11000000, 2432000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH6
		{{ true, 2437000000 - 11000000, 2437000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH7
		{{ true, 2442000000 - 11000000, 2442000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH8
		{{ true, 2447000000 - 11000000, 2447000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH9
		{{ true, 2452000000 - 11000000, 2452000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH10
		{{ true, 2457000000 - 11000000, 2457000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH11
		{{ true, 2462000000 - 11000000, 2462000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH12
		{{ true, 2467000000 - 11000000, 2467000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		// WLAN 2.4G CH13
		{{ true, 2472000000 - 11000000, 2472000000 + 11000000},	// BW: 22MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},

	};
	
	bool jamming { false };
	
	Labels labels {
		{ { 3 * 8, 4 }, "Type:", Color::light_grey() },
		{ { 2 * 8, 20 }, "Speed:", Color::light_grey() },
		{ { 1 * 8, 36 }, "Preset:", Color::light_grey() },
		{ { 5 * 8, 52 }, "Hop:", Color::light_grey() }
	};
	
	OptionsField options_type {
		{ 9 * 8, 4 },
		5,
		{
			{ "FSK  ", 0 },
			{ "Tone ", 1 },
			{ "Sweep", 2 }
		}
	};
	
	Text text_range_number {
		{ 18 * 8, 4, 2 * 8, 16 },
		"--"
	};
	Text text_range_total {
		{ 20 * 8, 4, 3 * 8, 16 },
		"/--"
	};
	
	OptionsField options_speed {
		{ 9 * 8, 20 },
		6,
		{
			{ "10Hz  ", 10 },
			{ "100Hz ", 100 },
			{ "1kHz  ", 1000 },
			{ "10kHz ", 10000 },
			{ "100kHz", 100000 }
		}
	};
	
	OptionsField options_preset {
		{ 9 * 8, 36 },
		16,
		{
			{ "GSM Orange FR  ", 0 },
			{ "GSM SFR FR     ", 1 },
			{ "GSM Bouygues FR", 2 },
			{ "GSM Free FR    ", 3 },
			{ "GSM-R FR       ", 4 },
			{ "DECT           ", 5 },
			{ "Optifib        ", 6 },
			{ "ISM 433        ", 7 },
			{ "ISM 868        ", 8 },
			{ "GPS            ", 9 },
			{ "WLAN 2.4G CH1  ", 10 },
			{ "WLAN 2.4G CH2  ", 11 },
			{ "WLAN 2.4G CH3  ", 12 },
			{ "WLAN 2.4G CH4  ", 13 },
			{ "WLAN 2.4G CH5  ", 14 },
			{ "WLAN 2.4G CH6  ", 15 },
			{ "WLAN 2.4G CH7  ", 16 },
			{ "WLAN 2.4G CH8  ", 17 },
			{ "WLAN 2.4G CH9  ", 18 },
			{ "WLAN 2.4G CH10 ", 19 },
			{ "WLAN 2.4G CH11 ", 20 },
			{ "WLAN 2.4G CH12 ", 21 },
			{ "WLAN 2.4G CH13 ", 22 }
		}
	};
	
	OptionsField options_hop {
		{ 9 * 8, 52 },
		5,
		{
			{ "10ms ", 1 },
			{ "50ms ", 5 },
			{ "100ms", 10 },
			{ "1s   ", 100 },
			{ "2s   ", 200 },
			{ "5s   ", 500 },
			{ "10s  ", 1000 }
		}
	};

	std::array<Checkbox, 3> checkboxes { };
	std::array<Button, 6> buttons_freq { };
	std::array<Text, 3> texts_info { };
	
	Button button_transmit {
		{ 1 * 8, 16 * 16, 80, 48 },
		"START"
	};
	
	MessageHandlerRegistration message_handler_retune {
		Message::ID::Retune,
		[this](Message* const p) {
			const auto message = static_cast<const RetuneMessage*>(p);
			this->on_retune(message->freq, message->range);
		}
	};
};

} /* namespace ui */
