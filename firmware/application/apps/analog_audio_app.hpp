/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
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
#include "ui_record_view.hpp"
#include "ui_font_fixed_8x16.hpp"
#include "app_settings.hpp"
#include "tone_key.hpp"


namespace ui {

constexpr Style style_options_group {
	.font = font::fixed_8x16,
	.background = Color::blue(),
	.foreground = Color::white(),
};

class AMOptionsView : public View {
public:
	AMOptionsView(const Rect parent_rect, const Style* const style);

private:
	Text label_config {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		"BW",
	};

	OptionsField options_config {
		{ 3 * 8, 0 * 16 },
		4,
		{
			{ "DSB ", 0 },
			{ "USB ", 0 },
			{ "LSB ", 0 },
			{ "CW  ", 0 },
		}
	};
};

class NBFMOptionsView : public View {
public:
	NBFMOptionsView(const Rect parent_rect, const Style* const style);

private:
	Text label_config {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		"BW",
	};
	OptionsField options_config {
		{ 3 * 8, 0 * 16 },
		4,
		{
			{ " 8k5", 0 },
			{ "11k ", 0 },
			{ "16k ", 0 },
		}
	};
	
	Text text_squelch {
		{ 9 * 8, 0 * 16, 8 * 8, 1 * 16 },
		"SQ   /99"
	};
	NumberField field_squelch {
		{ 12 * 8, 0 * 16 },
		2,
		{ 0, 99 },
		1,
		' ',
	};
};

class AnalogAudioView;

class SPECOptionsView : public View {
public:
	SPECOptionsView(AnalogAudioView* view, const Rect parent_rect, const Style* const style);

private:
	Text label_config {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		"BW",
	};
	OptionsField options_config {
		{ 3 * 8, 0 * 16 },
		4,
		{
			{ "20m ", 20000000 },
			{ "10m ", 10000000 },
			{ " 5m ", 5000000 },
			{ " 2m ", 2000000 },
			{ " 1m ", 1000000 },
			{ "500k", 500000 },
		}
	};
	
	Text text_speed {
		{ 9 * 8, 0 * 16, 8 * 8, 1 * 16 },
		"SP   /63"
	};
	NumberField field_speed {
		{ 12 * 8, 0 * 16 },
		2,
		{ 0, 63 },
		1,
		' ',
	};
};

class AnalogAudioView : public View {
public:
	AnalogAudioView(NavigationView& nav);
	~AnalogAudioView();

	void on_hide() override;

	void set_parent_rect(const Rect new_parent_rect) override;

	void focus() override;

	std::string title() const override { return "Audio RX"; };

	size_t get_spec_bw_index();
	void set_spec_bw(size_t index, uint32_t bw);

	uint16_t get_spec_trigger();
	void set_spec_trigger(uint16_t trigger);

private:
	static constexpr ui::Dim header_height = 3 * 16;

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	const Rect options_view_rect { 0 * 8, 1 * 16, 30 * 8, 1 * 16 };
	const Rect nbfm_view_rect { 0 * 8, 1 * 16, 18 * 8, 1 * 16 };

	size_t spec_bw_index = 0;
	uint32_t spec_bw = 20000000;
	uint16_t spec_trigger = 63;

	NavigationView& nav_;
	//bool exit_on_squelch { false };
	
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
	
	Text text_ctcss {
		{ 19 * 8, 1 * 16, 11 * 8, 1 * 16 },
		""
	};

	std::unique_ptr<Widget> options_widget { };

	RecordView record_view {
		{ 0 * 8, 2 * 16, 30 * 8, 1 * 16 },
		u"AUD",
		RecordView::FileType::WAV, 
		4096, 
		4
	};

	spectrum::WaterfallWidget waterfall { true };

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
	
	//void squelched();
	void handle_coded_squelch(const uint32_t value);
	
	/*MessageHandlerRegistration message_handler_squelch_signal {
		Message::ID::RequestSignal,
		[this](const Message* const p) {
			(void)p;
			this->squelched();
		}
	};*/
	
	MessageHandlerRegistration message_handler_coded_squelch {
		Message::ID::CodedSquelch,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
			this->handle_coded_squelch(message.value);
		}
	};
};

} /* namespace ui */

#endif/*__ANALOG_AUDIO_APP_H__*/
