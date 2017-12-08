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
#include "ui_tabview.hpp"
#include "transmitter_model.hpp"
#include "message.hpp"
#include "jammer.hpp"

using namespace jammer;

namespace ui {

class RangeView : public View {
public:
	RangeView(NavigationView& nav);
	
	void focus() override;
	void paint(Painter&) override;
	
	jammer_range_t frequency_range { false, 0, 0 };
	
private:
	void update_start(rf::Frequency f);
	void update_stop(rf::Frequency f);
	void update_center(rf::Frequency f);
	void update_width(uint32_t w);
	
	uint32_t width { };
	rf::Frequency center { };

	static constexpr Style style_info {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::grey(),
	};
	
	/*static constexpr jammer_range_t range_presets[] = {
		// GSM900 Orange
		{ true, 935000000, 945000000 },		// BW:10M
		// GSM1800 Orange
		{ true, 1808000000, 1832000000 },	// BW:24M
		
		// GSM900 SFR
		{ true, 950000000, 960000000 },		// BW:10M
		// GSM1800 SFR
		{ true, 1832000000, 1853000000 },	// BW:21M
		
		// GSM900 Bouygues
		{ true, 925000000, 935000000 },		// BW:10M
		// GSM1800 Bouygues
		{ true, 1858000000, 1880000000 },	// BW:22M
		
		// GSM900 Free
		{ true, 945000000, 950000000 },		// BW:5M
		
		// GSM-R
		{ true, 921000000, 925000000 },		// BW:4M
		
		// DECT
		{ true, 1880000000, 1900000000 },	// BW: 20MHz
		
		// PMV AFSK
		{ true, 162930000, 162970000 },		// BW: 40kHz
		
		// ISM 433
		{ true, 433050000, 434790000 },		// Center: 433.92MHz BW: 0.2%
		
		// ISM 868
		{ true, 868000000, 868200000 },		// Center: 868.2MHz BW: 40kHz
		
		// GPS L1
		{ true, 1575420000 - 500000, 1575420000 + 500000 },		// BW: 1MHz
		// GPS L2
		{ true, 1227600000 - 1000000, 1227600000 + 1000000 },	// BW: 2MHz
		
		// WLAN 2.4G CH1
		{ true, 2412000000 - 11000000, 2412000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH2
		{ true, 2417000000 - 11000000, 2417000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH3
		{ true, 2422000000 - 11000000, 2422000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH4
		{ true, 2427000000 - 11000000, 2427000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH5
		{ true, 2432000000 - 11000000, 2432000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH6
		{ true, 2437000000 - 11000000, 2437000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH7
		{ true, 2442000000 - 11000000, 2442000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH8
		{ true, 2447000000 - 11000000, 2447000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH9
		{ true, 2452000000 - 11000000, 2452000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH10
		{ true, 2457000000 - 11000000, 2457000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH11
		{ true, 2462000000 - 11000000, 2462000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH12
		{ true, 2467000000 - 11000000, 2467000000 + 11000000},	// BW: 22MHz
		// WLAN 2.4G CH13
		{ true, 2472000000 - 11000000, 2472000000 + 11000000},	// BW: 22MHz
	};*/
	
	Labels labels {
		{ { 2 * 8, 9 * 8 + 4 }, "Start", Color::light_grey() },
		{ { 23 * 8, 9 * 8 + 4 }, "Stop", Color::light_grey() },
		{ { 12 * 8, 6 * 8 }, "Center", Color::light_grey() },
		{ { 12 * 8 + 4, 14 * 8 }, "Width", Color::light_grey() }
	};
	
	Checkbox check_enabled {
		{ 1 * 8, 4 },
		12,
		"Enable range"
	};
	
	Button button_load_range {
		{ 18 * 8, 4, 12 * 8, 24 },
		"Load range"
	};
	
	Button button_start {
		{ 0 * 8, 6 * 16, 11 * 8, 28 },
		""
	};
	Button button_stop {
		{ 19 * 8, 6 * 16, 11 * 8, 28 },
		""
	};
	Button button_center {
		{ 76, 4 * 16, 11 * 8, 28 },
		""
	};
	Button button_width {
		{ 76, 8 * 16, 11 * 8, 28 },
		""
	};
};

class JammerView : public View {
public:
	JammerView(NavigationView& nav);
	~JammerView();
	
	JammerView(const JammerView&) = delete;
	JammerView(JammerView&&) = delete;
	JammerView& operator=(const JammerView&) = delete;
	JammerView& operator=(JammerView&&) = delete;
	
	void focus() override;
	
	std::string title() const override { return "Jammer"; };
	
private:
	NavigationView& nav_;
	
	void start_tx();
	void stop_tx();
	void set_jammer_channel(uint32_t i, uint32_t width, uint64_t center, uint32_t duration);
	void on_retune(const rf::Frequency freq, const uint32_t range);
	
	JammerChannel * jammer_channels = (JammerChannel*)shared_memory.bb_data.data;
	bool jamming { false };
	
	static constexpr Style style_val {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	static constexpr Style style_cancel {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	
	RangeView view_range_a { nav_ };
	RangeView view_range_b { nav_ };
	RangeView view_range_c { nav_ };
	
	std::array<RangeView*, 3> range_views { { &view_range_a, &view_range_b, &view_range_c } };
	
	TabView tab_view {
		{ "Range 1", Color::white(), range_views[0] },
		{ "Range 2", Color::white(), range_views[1] },
		{ "Range 3", Color::white(), range_views[2] },
	};
	
	Labels labels {
		{ { 3 * 8, 12 * 16 }, "Type:", Color::light_grey() },
		{ { 2 * 8, 13 * 16 }, "Speed:", Color::light_grey() },
		{ { 5 * 8, 14 * 16 }, "Hop:", Color::light_grey() }
	};
	
	OptionsField options_type {
		{ 9 * 8, 12 * 16 },
		8,
		{
			{ "Rand FSK", 0 },
			{ "FM tone", 1 },
			{ "CW sweep", 2 },
			{ "Rand CW", 3 },
		}
	};
	
	Text text_range_number {
		{ 22 * 8, 12 * 16, 2 * 8, 16 },
		"--"
	};
	Text text_range_total {
		{ 24 * 8, 12 * 16, 3 * 8, 16 },
		"/--"
	};
	
	OptionsField options_speed {
		{ 9 * 8, 13 * 16 },
		6,
		{
			{ "10Hz  ", 10 },
			{ "100Hz ", 100 },
			{ "1kHz  ", 1000 },
			{ "10kHz ", 10000 },
			{ "100kHz", 100000 }
		}
	};
	
	OptionsField options_hop {
		{ 9 * 8, 14 * 16 },
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
	
	Button button_transmit {
		{ 9 * 8, 16 * 16, 96, 48 },
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
