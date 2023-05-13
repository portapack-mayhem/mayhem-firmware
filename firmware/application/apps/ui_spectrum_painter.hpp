/*
 * Copyright (C) 2023 Bernd Herzog
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

#pragma once

#include "ui.hpp"
#include "ui_widget.hpp"

#include "ui_navigation.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "baseband_api.hpp"

#include "portapack.hpp"
#include "message.hpp"

#include "ui_spectrum_painter_image.hpp"
#include "ui_spectrum_painter_text.hpp"

namespace ui {


class SpectrumPainterView : public View {
public:
	SpectrumPainterView(NavigationView& nav);
	~SpectrumPainterView();

	SpectrumPainterView(const SpectrumPainterView&) = delete;
	SpectrumPainterView(SpectrumPainterView&&) = delete;
	SpectrumPainterView& operator=(const SpectrumPainterView&) = delete;
	SpectrumPainterView& operator=(SpectrumPainterView&&) = delete;

	void focus() override;
	void paint(Painter& painter) override;
	
	std::string title() const override { return "Spec.Painter"; };

private:

	void on_target_frequency_changed(rf::Frequency f);
	void set_target_frequency(const rf::Frequency new_value);
	rf::Frequency target_frequency() const;

	NavigationView& nav_;
	bool tx_active { false };
	uint16_t tx_current_line { 0 };
	uint16_t tx_current_max_lines { 0 };

	int32_t tx_gain { 47 };
	bool rf_amp { false };

	SpectrumInputImageView input_image { nav_ };
	SpectrumInputTextView input_text { nav_ };
	
	std::array<View*, 2> input_views { { &input_image, &input_text } };

	TabView tab_view {
		{ "Image", Color::white(), input_views[0] },
		{ "Text", Color::white(), input_views[1] }
	};

	ProgressBar progressbar {
		{ 16, 25 * 8, 208, 16 }
	};

	Labels labels {
		{ { 10 * 8, 16 * 16 }, "GAIN   A:", Color::light_grey() },
		{ { 1 * 8, 17 * 16 }, "BW:      Du:    P:", Color::light_grey() },
	};


	FrequencyField field_frequency {
		{ 0 * 8, 16 * 16 },
	};
	
	NumberField field_rfgain {
		{ 14 * 8, 16 * 16 },
		2,
		{ 0, 47 },
		1,
		' '	
	};

	NumberField field_rfamp {     // previously  I was using "RFAmpField field_rf_amp" but that is general Receiver amp setting.
		{ 19 * 8, 16 * 16 },
		2,
		{ 0, 14 },                // this time we will display GUI , 0 or 14 dBs same as Mic App
		14,
		' '
	};

	Checkbox check_loop {
		{ 21 * 8, 16 * 16 },
		4,
		"Loop",
		true
	};

	ImageButton button_play {
		{ 28 * 8, 16 * 16, 2 * 8, 1 * 16 },
		&bitmap_play,
		Color::green(),
		Color::black()
	};

	// add control for Bandwidth
	OptionsField option_bandwidth {
		{ 4 * 8, 17 * 16 },
		5,
		{
			{ "  8k5", 8500 },
			{ " 11k ", 11000 },
			{ " 16k ", 16000 },
			{ " 25k ", 25000 },
			{ " 50k ", 50000 },
			{ "100k ", 100000 },
			{ "250k ", 250000 },
			{ "500k ", 500000 },   // Previous Limit bandwith Option with perfect micro SD write .C16 format operaton.
			{ "600k ", 600000 },   // That extended option is still possible to record with FW version Mayhem v1.41 (< 2,5MB/sec)  
 			{ "750k ", 750000 },   // From that BW onwards, the LCD is ok, but the recorded file is auto decimated,(not real file size) 
			{ "1100k", 1100000 },
	       	{ "1750k", 1750000 },
			{ "2000k", 2000000 },
			{ "2500k", 2500000 },
			{ "2750k", 2750000 }    // That is our max Capture option , to keep using later / 8 decimation (22Mhz sampling  ADC)
		}
	};
	
	// add control for speed
	NumberField field_duration {
		{ 13 * 8, 17 * 16 },
		3,
		{ 0, 999 },
		1,
		' '	
	};

	// add control for pause when loop
	NumberField field_pause {
		{ 19 * 8, 17 * 16 },
		2,
		{ 0, 99 },
		1,
		' '	
	};

	SpectrumPainterFIFO* fifo { nullptr };
	void start_tx();
	void frame_sync();
	void stop_tx();

	MessageHandlerRegistration message_handler_fifo_signal {
		Message::ID::SpectrumPainterBufferResponseConfigure,
		[this](const Message* const p) {
			const auto message = static_cast<const SpectrumPainterBufferConfigureResponseMessage*>(p);
			this->fifo = message->fifo;
			this->start_tx();
		}
	};

	MessageHandlerRegistration message_handler_sample {
		Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			this->frame_sync();
		}
	};
};

}
