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

#include "analog_audio_app.hpp"

#include "portapack.hpp"
#include "portapack_shared_memory.hpp"
using namespace portapack;

#include "utility.hpp"

namespace ui {

/* AMOptionsView *********************************************************/

AMOptionsView::AMOptionsView(
	const Rect parent_rect, const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({ {
		&label_config,
		&options_config,
	} });

	options_config.on_change = [this](size_t n, OptionsField::value_t) {
		if( on_config_changed ) {
			this->on_config_changed(n);
		}
	};
}

/* NBFMOptionsView *******************************************************/

NBFMOptionsView::NBFMOptionsView(
	const Rect parent_rect, const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({ {
		&label_config,
		&options_config,
	} });

	options_config.on_change = [this](size_t n, OptionsField::value_t) {
		if( on_config_changed ) {
			this->on_config_changed(n);
		}
	};
}

/* AnalogAudioView *******************************************************/

AnalogAudioView::AnalogAudioView(
	NavigationView& nav
) {
	add_children({ {
		&rssi,
		&channel,
		&audio,
		&field_frequency,
		&field_lna,
		&field_vga,
		&options_modulation,
		&field_volume,
		&waterfall,
	} });

	field_frequency.set_value(receiver_model.tuning_frequency());
	field_frequency.set_step(receiver_model.frequency_step());
	field_frequency.on_change = [this](rf::Frequency f) {
		this->on_tuning_frequency_changed(f);
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->on_tuning_frequency_changed(f);
			this->field_frequency.set_value(f);
		};
	};

	field_frequency.on_show_options = [this]() {
		this->on_show_options_frequency();
	};

	field_lna.set_value(receiver_model.lna());
	field_lna.on_change = [this](int32_t v) {
		this->on_lna_changed(v);
	};

	field_lna.on_show_options = [this]() {
		this->on_show_options_rf_gain();
	};

	field_vga.set_value(receiver_model.vga());
	field_vga.on_change = [this](int32_t v_db) {
		this->on_vga_changed(v_db);
	};

	const auto modulation = receiver_model.modulation();
	options_modulation.set_by_value(modulation);
	options_modulation.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_modulation_changed(static_cast<ReceiverModel::Mode>(v));
	};
	options_modulation.on_show_options = [this]() {
		this->on_show_options_modulation();
	};

	field_volume.set_value((receiver_model.headphone_volume() - wolfson::wm8731::headphone_gain_range.max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};

	update_modulation(static_cast<ReceiverModel::Mode>(modulation));
}

AnalogAudioView::~AnalogAudioView() {
	// TODO: Manipulating audio codec here, and in ui_receiver.cpp. Good to do
	// both?
	audio_codec.headphone_mute();

	receiver_model.disable();
}

void AnalogAudioView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	View::on_hide();
}

void AnalogAudioView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);

	const ui::Rect waterfall_rect { 0, header_height, new_parent_rect.width(), static_cast<ui::Dim>(new_parent_rect.height() - header_height) };
	waterfall.set_parent_rect(waterfall_rect);
}

void AnalogAudioView::focus() {
	field_frequency.focus();
}

void AnalogAudioView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

void AnalogAudioView::on_baseband_bandwidth_changed(uint32_t bandwidth_hz) {
	receiver_model.set_baseband_bandwidth(bandwidth_hz);
}

void AnalogAudioView::on_rf_amp_changed(bool v) {
	receiver_model.set_rf_amp(v);
}

void AnalogAudioView::on_lna_changed(int32_t v_db) {
	receiver_model.set_lna(v_db);
}

void AnalogAudioView::on_vga_changed(int32_t v_db) {
	receiver_model.set_vga(v_db);
}

void AnalogAudioView::on_modulation_changed(const ReceiverModel::Mode modulation) {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	update_modulation(modulation);
	on_show_options_modulation();
	waterfall.on_show();
}

void AnalogAudioView::remove_options_widget() {
	if( options_widget ) {
		remove_child(options_widget.get());
		options_widget.reset();
	}

	field_lna.set_style(nullptr);
	options_modulation.set_style(nullptr);
	field_frequency.set_style(nullptr);
}

void AnalogAudioView::set_options_widget(std::unique_ptr<Widget> new_widget) {
	if( new_widget ) {
		options_widget = std::move(new_widget);
		add_child(options_widget.get());
	}
}

void AnalogAudioView::on_show_options_frequency() {
	// TODO: This approach of managing options views is error-prone and unsustainable!
	remove_options_widget();

	field_frequency.set_style(&style_options_group);

	auto widget = std::make_unique<FrequencyOptionsView>(
		Rect { 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	);

	widget->set_step(receiver_model.frequency_step());
	widget->on_change_step = [this](rf::Frequency f) {
		this->on_frequency_step_changed(f);
	};
	widget->set_reference_ppm_correction(receiver_model.reference_ppm_correction());
	widget->on_change_reference_ppm_correction = [this](int32_t v) {
		this->on_reference_ppm_correction_changed(v);
	};

	set_options_widget(std::move(widget));
}

void AnalogAudioView::on_show_options_rf_gain() {
	// TODO: This approach of managing options views is error-prone and unsustainable!
	remove_options_widget();

	field_lna.set_style(&style_options_group);

	auto widget = std::make_unique<RadioGainOptionsView>(
		Rect { 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
		&style_options_group
	);

	widget->set_rf_amp(receiver_model.rf_amp());
	widget->on_change_rf_amp = [this](bool enable) {
		this->on_rf_amp_changed(enable);
	};

	set_options_widget(std::move(widget));
}

void AnalogAudioView::on_show_options_modulation() {
	// TODO: This approach of managing options views is error-prone and unsustainable!
	remove_options_widget();

	const auto modulation = static_cast<ReceiverModel::Mode>(receiver_model.modulation());
	if( modulation == ReceiverModel::Mode::AMAudio ) {
		options_modulation.set_style(&style_options_group);
		auto widget = std::make_unique<AMOptionsView>(
			Rect { 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
			&style_options_group
		);
		widget->on_config_changed = [this](size_t n) {
			this->on_am_config_index_changed(n);
		};
		set_options_widget(std::move(widget));
	}
	if( modulation == ReceiverModel::Mode::NarrowbandFMAudio ) {
		options_modulation.set_style(&style_options_group);
		auto widget = std::make_unique<NBFMOptionsView>(
			Rect { 0 * 8, 1 * 16, 30 * 8, 1 * 16 },
			&style_options_group
		);
		widget->on_config_changed = [this](size_t n) {
			this->on_nbfm_config_index_changed(n);
		};
		set_options_widget(std::move(widget));
	}
}

void AnalogAudioView::on_frequency_step_changed(rf::Frequency f) {
	receiver_model.set_frequency_step(f);
	field_frequency.set_step(f);
}

void AnalogAudioView::on_reference_ppm_correction_changed(int32_t v) {
	receiver_model.set_reference_ppm_correction(v);
}

void AnalogAudioView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + wolfson::wm8731::headphone_gain_range.max;
	receiver_model.set_headphone_volume(new_volume);
}

void AnalogAudioView::on_am_config_index_changed(size_t n) {
	receiver_model.set_am_configuration(n);
}

void AnalogAudioView::on_nbfm_config_index_changed(size_t n) {
	receiver_model.set_nbfm_configuration(n);
}

void AnalogAudioView::update_modulation(const ReceiverModel::Mode modulation) {
	const auto is_wideband_spectrum_mode = (modulation == ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_baseband_configuration({
		.mode = toUType(modulation),
		.sampling_rate = is_wideband_spectrum_mode ? 20000000U : 3072000U,
		.decimation_factor = 1,
	});
	receiver_model.set_baseband_bandwidth(is_wideband_spectrum_mode ? 12000000 : 1750000);
	receiver_model.enable();
}

} /* namespace ui */
