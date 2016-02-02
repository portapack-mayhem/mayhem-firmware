/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __ANALOG_AUDIO_APP_H__
#define __ANALOG_AUDIO_APP_H__

#include "receiver_model.hpp"

#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"

#include "ui_font_fixed_8x16.hpp"

namespace ui {

constexpr Style style_options_group {
	.font = font::fixed_8x16,
	.background = Color::blue(),
	.foreground = Color::white(),
};

class AnalogAudioView : public View {
public:
	AnalogAudioView(NavigationView& nav);
	~AnalogAudioView();

	void on_hide() override;

	void set_parent_rect(const Rect new_parent_rect) override;

	void focus() override;

private:
	static constexpr ui::Dim header_height = 2 * 16;

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

	NumberField field_vga {
		{ 18 * 8, 0 * 16},
		2,
		{ max2837::vga::gain_db_range.minimum, max2837::vga::gain_db_range.maximum },
		max2837::vga::gain_db_step,
		' ',
	};

	OptionsField options_modulation {
		{ 0 * 8, 0 * 16 },
		4,
		{
			{ " AM ", toUType(ReceiverModel::Mode::AMAudio) },
			{ "NFM ", toUType(ReceiverModel::Mode::NarrowbandFMAudio) },
			{ "WFM ", toUType(ReceiverModel::Mode::WidebandFMAudio) },
			{ "SPEC", toUType(ReceiverModel::Mode::SpectrumAnalysis) },
		}
	};

	NumberField field_volume {
		{ 28 * 8, 0 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};

	FrequencyOptionsView view_frequency_options {
		{ 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	};

	RadioGainOptionsView view_rf_gain_options {
		{ 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	};
/*
	AmplitudeModulationOptionsView view_am_modulation_options {
		{ 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	};

	FrequencyModulationOptionsView view_fm_modulation_options {
		{ 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	};
*/
	spectrum::WaterfallWidget waterfall;


	void on_tuning_frequency_changed(rf::Frequency f);
	void on_baseband_bandwidth_changed(uint32_t bandwidth_hz);
	void on_rf_amp_changed(bool v);
	void on_lna_changed(int32_t v_db);
	void on_vga_changed(int32_t v_db);
	void on_modulation_changed(const ReceiverModel::Mode modulation);
	void on_show_options_frequency();
	void on_show_options_rf_gain();
	void on_frequency_step_changed(rf::Frequency f);
	void on_reference_ppm_correction_changed(int32_t v);
	void on_headphone_volume_changed(int32_t v);
	void on_edit_frequency();

	void on_am_config_index_changed(size_t n);
	void on_nbfm_config_index_changed(size_t n);

	void update_modulation(const ReceiverModel::Mode modulation);
};

} /* namespace ui */

#endif/*__ANALOG_AUDIO_APP_H__*/
