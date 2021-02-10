/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2020 Shao
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

#ifndef __ANALOG_TV_APP_H__
#define __ANALOG_TV_APP_H__

#include "receiver_model.hpp"

#include "ui_receiver.hpp"
#include "ui_tv.hpp"
#include "ui_record_view.hpp"

#include "ui_font_fixed_8x16.hpp"

#include "tone_key.hpp"

namespace ui {

constexpr Style style_options_group_new {
	.font = font::fixed_8x16,
	.background = Color::blue(),
	.foreground = Color::white(),
};

class AnalogTvView : public View {
public:
	AnalogTvView(NavigationView& nav);
	~AnalogTvView();

	void on_hide() override;

	void set_parent_rect(const Rect new_parent_rect) override;

	void focus() override;

	std::string title() const override { return "Analog TV"; };
	
private:
	static constexpr ui::Dim header_height = 3 * 16;

	const Rect options_view_rect { 0 * 8, 1 * 16, 30 * 8, 1 * 16 };
	const Rect nbfm_view_rect { 0 * 8, 1 * 16, 18 * 8, 1 * 16 };

	NavigationView& nav_;
	
	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};

	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};

	Audio audio {
		{ 21 * 8, 10, 6 * 8, 4 },
	};

	FrequencyField field_frequency {
		{ 5 * 8, 0 * 16 },
	};

	LNAGainField field_lna {
		{ 15 * 8, 0 * 16 }
	};

	VGAGainField field_vga {
		{ 18 * 8, 0 * 16 }
	};

	OptionsField options_modulation {
		{ 0 * 8, 0 * 16 },
		4,
		{
			{ "TV ", toUType(ReceiverModel::Mode::WidebandFMAudio) },
			{ "TV ", toUType(ReceiverModel::Mode::WidebandFMAudio) },
			{ "TV ", toUType(ReceiverModel::Mode::WidebandFMAudio) },
		}
	};

	NumberField field_volume {
		{ 27 * 8, 0 * 16 },
		3,
		{ 0, 255 },
		1,
		' ',
	};

	std::unique_ptr<Widget> options_widget { };

	tv::TVWidget tv { true };

	void on_tuning_frequency_changed(rf::Frequency f);
	void on_baseband_bandwidth_changed(uint32_t bandwidth_hz);
	void on_modulation_changed(const ReceiverModel::Mode modulation);
	void on_show_options_frequency();
	void on_show_options_rf_gain();
	void on_show_options_modulation();
	void on_frequency_step_changed(rf::Frequency f);
	void on_reference_ppm_correction_changed(int32_t v);
	void on_headphone_volume_changed(int32_t v);
	void on_edit_frequency();

	void remove_options_widget();
	void set_options_widget(std::unique_ptr<Widget> new_widget);

	void update_modulation(const ReceiverModel::Mode modulation);
	

};

} /* namespace ui */

#endif/*__ANALOG_TV_APP_H__*/
