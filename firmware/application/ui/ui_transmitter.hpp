/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_TRANSMITTER_H__
#define __UI_TRANSMITTER_H__

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_painter.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "rf_path.hpp"

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>

#define POWER_THRESHOLD_HIGH	47
#define POWER_THRESHOLD_MED		38
#define POWER_THRESHOLD_LOW		17

namespace ui {

class TXGainField : public NumberField {
public:
	std::function<void(void)> on_show_options { };

	TXGainField(Point parent_pos);
};

class TransmitterView : public View {
public:
	std::function<void(void)> on_edit_frequency { };
	std::function<void(void)> on_start { };
	std::function<void(void)> on_stop { };
	
	TransmitterView(const Coord y, const uint64_t frequency_step, const uint32_t channel_bandwidth, const bool lock);
	TransmitterView(
		const Coord y, const uint32_t frequency_step, const uint32_t channel_bandwidth
	) : TransmitterView { y, frequency_step, channel_bandwidth, false }
	{
	}
	
	~TransmitterView();
	
	void on_show() override;
	void paint(Painter& painter) override;
	void focus() override;
	
	void set_transmitting(const bool transmitting);

private:
	const Style style_start {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::green(),
	};
	const Style style_stop {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};
	const Style style_locked {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::dark_grey(),
	};
	const Style style_power_low {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::yellow(),
	};
	const Style style_power_med {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::orange(),
	};
	const Style style_power_high {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::red(),
	};

	bool lock_ { false };
	bool transmitting_ { false };
	
	FrequencyField field_frequency {
		{ 0, 1 * 8 }
	};
	
	Text text_gain {
		{ 0, 3 * 8, 5 * 8, 1 * 16 },
		"Gain:"
	};
	
	NumberField field_gain {
		{ 5 * 8, 3 * 8 },
		2,
		{ max2837::tx::gain_db_range.minimum, max2837::tx::gain_db_range.maximum },
		max2837::tx::gain_db_step,
		' '
	};

	Text text_bw {
		{ 11 * 8, 1 * 8, 9 * 8, 1 * 16 },
		"BW:   kHz"
	};
	NumberField field_bw {
		{ 14 * 8, 1 * 8 },
		3,
		{ 1, 150 },
		1,
		' '
	};
	
	Text text_amp {
		{ 11 * 8, 3 * 8, 5 * 8, 1 * 16 },
		"Amp:"
	};

	NumberField field_amp {
		{ 16 * 8, 3 * 8 },
		2,
		{ 0, 14 },
		14,
		' '
	};

	Button button_start {
		{ 21 * 8, 1 * 8, 9 * 8, 32 },
		"START"
	};

	void on_tuning_frequency_changed(rf::Frequency f);
	void on_channel_bandwidth_changed(uint32_t channel_bandwidth);
	void on_tx_gain_changed(int32_t tx_gain);
	void on_tx_amp_changed(bool rf_amp);

	void update_gainlevel_styles(void);
};

} /* namespace ui */

#endif/*__UI_TRANSMITTER_H__*/
