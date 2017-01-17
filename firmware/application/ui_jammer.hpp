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
#include "message.hpp"
#include "transmitter_model.hpp"

namespace ui {

class JammerView : public View {
public:
	JammerView(NavigationView& nav);
	~JammerView();
	
	void focus() override;
	
	std::string title() const override { return "Jammer"; };

private:
	// range_t from utility.hpp is const only
	typedef struct freq_range {
		bool enabled;
		rf::Frequency min;
		rf::Frequency max;
	} freq_range_t;

	freq_range_t frequency_range[3];
	
	void update_text(uint8_t id, rf::Frequency f);
	void on_retune(const rf::Frequency freq, const uint32_t range);
		
	// TODO: TDD UMTS, voir doc Arcep
	// TODO: BT: 2 400 et 2 483,5 MHz
	const freq_range_t range_presets[23][3] = {
		// Orange
		{{ true, 935000000, 945000000 },	// GSM 900
		{ true, 1808000000, 1832000000 },	// GSM 1800
		{ true, 2154900000, 2169700000 }},	// UMTS
		
		// SFR
		{{ true, 950000000, 960000000 },	// GSM 900
		{ true, 1832000000, 1853000000 },	// GSM 1800
		{ true, 2110500000, 2125300000 }},	// UMTS
		
		// Bouygues
		{{ true, 925000000, 935000000 },	// GSM 900
		{ true, 1858000000, 1880000000 },	// GSM 1800
		{ true, 2125300000, 2140100000 }},	// UMTS
		
		// Free
		{{ true, 945000000, 950000000 },	// GSM 900
		{ false, 0, 0 },					// GSM 1800 ?
		{ true, 2144900000, 2149900000 }},	// UMTS
		
		// GSM-R
		{{ true, 921000000, 925000000 },	// GSM 900
		{ false, 0, 0 },					// GSM 1800
		{ false, 0, 0 }},					// UMTS
		
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
		{{ true, 1575420000 - 1000000, 1575420000 + 1000000},	// BW: 2MHz
		{ true, 1227600000 - 1000000, 1227600000 + 1000000 },	// BW: 2MHz
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
	
	bool jamming = false;
	rf::Frequency f;
	
	Text text_type {
		{ 1 * 8, 1 * 16, 40, 16 },
		"Type:"
	};
	OptionsField options_modulation {
		{ 7 * 8, 1 * 16 },
		4,
		{
			{ "Ramp ", 0 },
			{ "FM   ", 1 },
			{ "Phase", 2 },
			{ "Tones", 3 }
		}
	};
	
	Text text_range_number {
		{ 14 * 8, 1 * 16, 2 * 8, 16 },
		"--"
	};
	
	Text text_sweep {
		{ 1 * 8, 2 * 16, 6 * 8, 16 },
		"Sweep:"
	};
	OptionsField options_sweep {
		{ 8 * 8, 2 * 16 },
		8,
		{
			{ "  1Hz", 0 },
			{ " 10Hz", 1 },
			{ "100Hz", 2 },
			{ " 1kHz", 3 },
			{ " 5kHz", 4 },
			{ "10kHz", 5 },
			{ "20kHz", 6 },
			{ "50kHz", 7 }
		}
	};
	
	Text text_hop {
		{ 1 * 8, 4 * 16, 10 * 8, 16 },
		"Range hop:"
	};
	OptionsField options_hop {
		{ 12 * 8, 4 * 16 },
		5,
		{
			{ " 10ms", 1 },
			{ "100ms", 10 },
			{ "   1s", 100 },
			{ "   5s", 500 },
			{ "  10s", 1000 }
		}
	};
	
	Text text_preset {
		{ 1 * 8, 3 * 16, 8 * 8, 16 },
		"Presets:"
	};
	OptionsField options_preset {
		{ 10 * 8, 3 * 16 },
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
	
	Checkbox checkbox_range1 {
		{ 1 * 8, 6 * 16},
		7,
		"Range 1"
	};
	Checkbox checkbox_range2 {
		{ 1 * 8, 9 * 16 + 4},
		7,
		"Range 2"
	};
	Checkbox checkbox_range3 {
		{ 1 * 8, 12 * 16 + 8 },
		7,
		"Range 3"
	};
	
	std::array<Button, 6> buttons_freq { };
	
	Text text_info1 {
		{ 3 * 8, 8 * 16 - 4 + 2, 25 * 8, 16 },
		"C:----.----M W:-----kHz"
	};
	
	Text text_info2 {
		{ 3 * 8, 11 * 16 + 2, 25 * 8, 16 },
		"C:----.----M W:-----kHz"
	};
	
	Text text_info3 {
		{ 3 * 8, 14 * 16 + 4 + 2, 25 * 8, 16 },
		"C:----.----M W:-----kHz"
	};
	
	Button button_transmit {
		{ 2 * 8, 16 * 16, 64, 32 },
		"START"
	};
	
	Button button_exit {
		{ 21 * 8, 16 * 16, 64, 32 },
		"Exit"
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
