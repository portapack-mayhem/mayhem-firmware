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
	
	void updfreq(uint8_t id, rf::Frequency f);
	void focus() override;
	
	std::string title() const override { return "Jammer"; };

private:
	void on_retune(const int64_t freq);
	
	rf::Frequency range1_min;
	rf::Frequency range1_max;
	rf::Frequency range2_min;
	rf::Frequency range2_max;
	rf::Frequency range3_min;
	rf::Frequency range3_max;
	
	rf::Frequency range1_center;
	rf::Frequency range1_width;
	rf::Frequency range2_center;
	rf::Frequency range2_width;
	rf::Frequency range3_center;
	rf::Frequency range3_width;
	
	typedef struct rangepreset {
		bool active;
		rf::Frequency min;
		rf::Frequency max;
	} rangepreset;
	
	const rangepreset range_presets[10][3] = {
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
		{ false, 0, 0 },					// GSM 1800
		{ true, 2144900000, 2149900000 }},	// UMTS
		
		// GSM-R
		{{ true, 921000000, 925000000 },	// GSM 900
		{ false, 0, 0 },					// GSM 1800
		{ false, 0, 0 }},					// UMTS
		
		// TODO: TDD UMTS, voir doc Arcep
		// TODO: Wifi, BT: 2 400 et 2 483,5 MHz
		
		// DECT
		{{ true, 1880000000, 1900000000 },	// BW: 20MHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// PMV AFSK
		{{ true, 162930000, 162970000 },	// BW: 40kHz
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// ISM 433
		{{ true, 433050000, 434790000 },	// BW: 0.2%
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// Sigfox
		{{ true, 868150000, 868250000 },	// BW: 40kHz (50kHz)
		{ false, 0, 0 },
		{ false, 0, 0 }},
		
		// GPS L1 & L2
		{{ true, 1575420000 - 50000, 1575420000 + 50000},	// BW: 100kHz
		{ true, 1227600000 - 50000, 1227600000 + 50000 },	// BW: 100kHz
		{ false, 0, 0 }}

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
			{ "PSK  ", 2 },
			{ "Tones", 3 }
		}
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
		6,
		{
			{ "  10s", 0 },
			{ "   5s", 1 },
			{ "   1s", 2 },
			{ " 0.1s", 3 },
			{ " 10ms", 4 },
			{ "  1ms", 5 }
		}
	};
	
	Text text_preset {
		{ 1 * 8, 3 * 16, 8 * 8, 16 },
		"Presets:"
	};
	OptionsField options_preset {
		{ 10 * 8, 3 * 16 },
		8,
		{
			{ "Orange  ", 0 },
			{ "SFR     ", 1 },
			{ "Bouygues", 2 },
			{ "Free    ", 3 },
			{ "GSM-R   ", 4 },
			{ "DECT    ", 5 },
			{ "Optifib ", 6 },
			{ "ISM 433 ", 7 },
			{ "Sigfox  ", 8 },
			{ "GPS     ", 9 }
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
	
	Button button_setfreq1_min {
		{ 13 * 8, 6 * 16 - 4 - 1, 11 * 8, 18 },
		"----.----M"
	};
	Button button_setfreq1_max {
		{ 13 * 8, 7 * 16 - 4, 11 * 8, 18 },
		"----.----M"
	};
	Text text_info1 {
		{ 3 * 8, 8 * 16 - 4 + 2, 25 * 8, 16 },
		"C:----.----M W:-----kHz"
	};
	
	Button button_setfreq2_min {
		{ 13 * 8, 9 * 16 - 1, 11 * 8, 18 },
		"----.----M"
	};
	Button button_setfreq2_max {
		{ 13 * 8, 10 * 16, 11 * 8, 18 },
		"----.----M"
	};
	Text text_info2 {
		{ 3 * 8, 11 * 16 + 2, 25 * 8, 16 },
		"C:----.----M W:-----kHz"
	};
	
	Button button_setfreq3_min {
		{ 13 * 8, 12 * 16 + 4 - 1, 11 * 8, 18 },
		"----.----M"
	};
	Button button_setfreq3_max {
		{ 13 * 8, 13 * 16 + 4, 11 * 8, 18 },
		"----.----M"
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
			this->on_retune(message->freq);
		}
	};
};

} /* namespace ui */
