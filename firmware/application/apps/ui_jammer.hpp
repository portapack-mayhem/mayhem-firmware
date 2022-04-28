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
#include "lfsr_random.hpp"

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
	
	Labels labels {
		{ { 2 * 8, 8 * 8 + 4 }, "Start", Color::light_grey() },
		{ { 23 * 8, 8 * 8 + 4 }, "Stop", Color::light_grey() },
		{ { 12 * 8, 5 * 8 - 4}, "Center", Color::light_grey() },
		{ { 12 * 8 + 4, 13 * 8 }, "Width", Color::light_grey() }
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
		{ 0 * 8, 11 * 8, 11 * 8, 28 },
		""
	};
	Button button_stop {
		{ 19 * 8, 11 * 8, 11 * 8, 28 },
		""
	};
	Button button_center {
		{ 76, 4 * 15 - 4, 11 * 8, 28 },
		""
	};
	Button button_width {
		{ 76, 8 * 15, 11 * 8, 28 },
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
	
	std::string title() const override { return "Jammer TX"; };
	
private:
	NavigationView& nav_;
	
	void start_tx();
	void on_timer();
	void stop_tx();
	void set_jammer_channel(uint32_t i, uint32_t width, uint64_t center, uint32_t duration);
	void on_retune(const rf::Frequency freq, const uint32_t range);
	
	JammerChannel * jammer_channels = (JammerChannel*)shared_memory.bb_data.data;
	bool jamming { false };
	bool cooling { false };		//euquiq: Indicates jammer in cooldown
	uint16_t seconds = 0;		//euquiq: seconds counter for toggling tx / cooldown
	int16_t mscounter = 0;		//euquiq: Internal ms counter for do_timer()
	lfsr_word_t lfsr_v = 1;		//euquiq: Used to generate "random" Jitter
	
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
		{ { 2 * 8, 23 * 8 }, "Type:", Color::light_grey() },
		{ { 1 * 8, 25 * 8 }, "Speed:", Color::light_grey() },
		{ { 3 * 8, 27 * 8 }, "Hop:", Color::light_grey() },
		{ { 4 * 8, 29 * 8 }, "TX:", Color::light_grey() },
		{ { 1 * 8, 31 * 8 }, "Sle3p:", Color::light_grey() }, 	//euquiq: Token of appreciation to TheSle3p, which made this ehnancement a reality with his bounty.
		{ { 0 * 8, 33 * 8 }, "Jitter:", Color::light_grey() },	//Maybe the repository curator can keep the "mystype" for some versions. 
		{ { 11 * 8, 29 * 8 }, "Secs.", Color::light_grey() }, 
		{ { 11 * 8, 31 * 8 }, "Secs.", Color::light_grey() },
		{ { 11 * 8, 33 * 8 }, "/60", Color::light_grey() }
	};
	
	OptionsField options_type {
		{ 7 * 8, 23 * 8 },
		8,
		{
			{ "Rand FSK", 0 },
			{ "FM tone", 1 },
			{ "CW sweep", 2 },
			{ "Rand CW", 3 },
		}
	};
	
	Text text_range_number {
		{ 16 * 8, 23 * 8, 2 * 8, 16 },
		"--"
	};
	Text text_range_total {
		{ 18 * 8, 23 * 8, 3 * 8, 16 },
		"/--"
	};
	
	OptionsField options_speed {
		{ 7 * 8, 25 * 8 },
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
		{ 7 * 8, 27 * 8 },
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

	NumberField field_timetx {
		{ 7 * 8, 29 * 8 },
		3,
		{ 1, 180 },
		1,
		' ',
	};

	NumberField field_timepause {
		{ 8 * 8, 31 * 8 },
		2,
		{ 1, 60 },
		1,
		' ',
	};
	
	NumberField field_jitter {
		{ 8 * 8, 33 * 8 },
		2,
		{ 1, 60 },
		1,
		' ',
	};

	Button button_transmit {
		{ 148, 212, 80, 80},
		"START"
	};
	
	MessageHandlerRegistration message_handler_retune {
		Message::ID::Retune,
		[this](Message* const p) {
			const auto message = static_cast<const RetuneMessage*>(p);
			this->on_retune(message->freq, message->range);
		}
	};

		MessageHandlerRegistration message_handler_frame_sync {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->on_timer();
		}
	};
};

} /* namespace ui */

