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

#include "analog_audio_app.hpp"

#include "baseband_api.hpp"

#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
using namespace portapack;
using namespace tonekey;

#include "audio.hpp"
#include "file.hpp"

#include "utility.hpp"

#include "string_format.hpp"

namespace ui {

/* AMOptionsView *********************************************************/

AMOptionsView::AMOptionsView(
	const Rect parent_rect, const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({
		&label_config,
		&options_config,
	});

	options_config.set_selected_index(receiver_model.am_configuration());
	options_config.on_change = [this](size_t n, OptionsField::value_t) {
		receiver_model.set_am_configuration(n);
	};
}

/* NBFMOptionsView *******************************************************/

NBFMOptionsView::NBFMOptionsView(
	const Rect parent_rect, const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({
		&label_config,
		&options_config,
		&text_squelch,
		&field_squelch
	});

	options_config.set_selected_index(receiver_model.nbfm_configuration());
	options_config.on_change = [this](size_t n, OptionsField::value_t) {
		receiver_model.set_nbfm_configuration(n);
	};
	
	field_squelch.set_value(receiver_model.squelch_level());
	field_squelch.on_change = [this](int32_t v) {
		receiver_model.set_squelch_level(v);
	};
}

/* SPECOptionsView *******************************************************/

SPECOptionsView::SPECOptionsView(
	AnalogAudioView* view, const Rect parent_rect, const Style* const style
) : View { parent_rect }
{
	set_style(style);

	add_children({
		&label_config,
		&options_config,
		&text_speed,
		&field_speed
	});

	options_config.set_selected_index(view->get_spec_bw_index());
	options_config.on_change = [this, view](size_t n, OptionsField::value_t bw) {
		view->set_spec_bw(n, bw);
	};

	field_speed.set_value(view->get_spec_trigger());
	field_speed.on_change = [this, view](int32_t v) {
		view->set_spec_trigger(v);
	};
}

/* AnalogAudioView *******************************************************/

AnalogAudioView::AnalogAudioView(
	NavigationView& nav
) : nav_ (nav)
{
	add_children({
		&rssi,
		&channel,
		&audio,
		&field_frequency,
		&field_lna,
		&field_vga,
		&options_modulation,
		&field_volume,
		&text_ctcss,
		&record_view,
		&waterfall
	});

	// load app settings
	auto rc = settings.load("rx_audio", &app_settings);
	if(rc == SETTINGS_OK) {
		field_lna.set_value(app_settings.lna);
		field_vga.set_value(app_settings.vga);
		receiver_model.set_rf_amp(app_settings.rx_amp);
		field_frequency.set_value(app_settings.rx_frequency);
	}
	else field_frequency.set_value(receiver_model.tuning_frequency());
	
	//Filename Datetime and Frequency
	record_view.set_filename_date_frequency(true);

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

	field_lna.on_show_options = [this]() {
		this->on_show_options_rf_gain();
	};

	field_vga.on_show_options = [this]() {
		this->on_show_options_rf_gain();
	};

	const auto modulation = receiver_model.modulation();
	options_modulation.set_by_value(toUType(modulation));

	options_modulation.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_modulation_changed(static_cast<ReceiverModel::Mode>(v));
	};
	options_modulation.on_show_options = [this]() {
		this->on_show_options_modulation();
	};

	field_volume.set_value((receiver_model.headphone_volume() - audio::headphone::volume_range().max).decibel() + 99);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};

	record_view.on_error = [&nav](std::string message) {
		nav.display_modal("Error", message);
	};
	
	waterfall.on_select = [this](int32_t offset) {
		field_frequency.set_value(receiver_model.tuning_frequency() + offset);
	};

	audio::output::start();

	update_modulation(static_cast<ReceiverModel::Mode>(modulation));
    	on_modulation_changed(static_cast<ReceiverModel::Mode>(modulation));
}

size_t AnalogAudioView::get_spec_bw_index() {
    return spec_bw_index;
}

void AnalogAudioView::set_spec_bw(size_t index, uint32_t bw) {
    spec_bw_index = index;
    spec_bw = bw;

    baseband::set_spectrum(bw, spec_trigger);
    receiver_model.set_sampling_rate(bw);
    receiver_model.set_baseband_bandwidth(bw/2);
}

uint16_t AnalogAudioView::get_spec_trigger() {
    return spec_trigger;
}

void AnalogAudioView::set_spec_trigger(uint16_t trigger) {
    spec_trigger = trigger;

    baseband::set_spectrum(spec_bw, spec_trigger);
}

AnalogAudioView::~AnalogAudioView() {

	// save app settings
	app_settings.rx_frequency = field_frequency.value();
	settings.save("rx_audio", &app_settings);

	// TODO: Manipulating audio codec here, and in ui_receiver.cpp. Good to do
	// both?
	audio::output::stop();

	receiver_model.set_sampling_rate(3072000); 	// Just a hack to avoid hanging other apps if the last modulation was SPEC
	receiver_model.disable();

	baseband::shutdown();
}

void AnalogAudioView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	waterfall.on_hide();
	View::on_hide();
}

void AnalogAudioView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	
	const ui::Rect waterfall_rect { 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height };
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
	remove_options_widget();

	if( new_widget ) {
		options_widget = std::move(new_widget);
	} else {
		// TODO: Lame hack to hide options view due to my bad paint/damage algorithm.
		options_widget = std::make_unique<Rectangle>(options_view_rect, style_options_group.background);
	}
	add_child(options_widget.get());
}

void AnalogAudioView::on_show_options_frequency() {
	auto widget = std::make_unique<FrequencyOptionsView>(options_view_rect, &style_options_group);

	widget->set_step(receiver_model.frequency_step());
	widget->on_change_step = [this](rf::Frequency f) {
		this->on_frequency_step_changed(f);
	};
	widget->set_reference_ppm_correction(persistent_memory::correction_ppb() / 1000);
	widget->on_change_reference_ppm_correction = [this](int32_t v) {
		this->on_reference_ppm_correction_changed(v);
	};

	set_options_widget(std::move(widget));
	field_frequency.set_style(&style_options_group);
}

void AnalogAudioView::on_show_options_rf_gain() {
	auto widget = std::make_unique<RadioGainOptionsView>(options_view_rect, &style_options_group);

	set_options_widget(std::move(widget));
	field_lna.set_style(&style_options_group);
}

void AnalogAudioView::on_show_options_modulation() {
	std::unique_ptr<Widget> widget;

	const auto modulation = static_cast<ReceiverModel::Mode>(receiver_model.modulation());
	switch(modulation) {
	case ReceiverModel::Mode::AMAudio:
		widget = std::make_unique<AMOptionsView>(options_view_rect, &style_options_group);
		waterfall.show_audio_spectrum_view(false);
		text_ctcss.hidden(true);
		break;

	case ReceiverModel::Mode::NarrowbandFMAudio:
		widget = std::make_unique<NBFMOptionsView>(nbfm_view_rect, &style_options_group);
		waterfall.show_audio_spectrum_view(false);
		text_ctcss.hidden(false);
		break;
	
	case ReceiverModel::Mode::WidebandFMAudio:
		waterfall.show_audio_spectrum_view(true);
		text_ctcss.hidden(true);
		break;
	
	case ReceiverModel::Mode::SpectrumAnalysis:
		widget = std::make_unique<SPECOptionsView>(this, nbfm_view_rect, &style_options_group);
		waterfall.show_audio_spectrum_view(false);
		text_ctcss.hidden(true);
		break;
		
	default:
		break;
	}

	set_options_widget(std::move(widget));
	options_modulation.set_style(&style_options_group);
}

void AnalogAudioView::on_frequency_step_changed(rf::Frequency f) {
	receiver_model.set_frequency_step(f);
	field_frequency.set_step(f);
}

void AnalogAudioView::on_reference_ppm_correction_changed(int32_t v) {
	persistent_memory::set_correction_ppb(v * 1000);
}

void AnalogAudioView::on_headphone_volume_changed(int32_t v) {
	const auto new_volume = volume_t::decibel(v - 99) + audio::headphone::volume_range().max;
	receiver_model.set_headphone_volume(new_volume);
}

void AnalogAudioView::update_modulation(const ReceiverModel::Mode modulation) {
	audio::output::mute();
	record_view.stop();

	baseband::shutdown();

	portapack::spi_flash::image_tag_t image_tag;
	switch(modulation) {
	case ReceiverModel::Mode::AMAudio:				image_tag = portapack::spi_flash::image_tag_am_audio;			break;
	case ReceiverModel::Mode::NarrowbandFMAudio:	image_tag = portapack::spi_flash::image_tag_nfm_audio;			break;
	case ReceiverModel::Mode::WidebandFMAudio:		image_tag = portapack::spi_flash::image_tag_wfm_audio;			break;
	case ReceiverModel::Mode::SpectrumAnalysis:		image_tag = portapack::spi_flash::image_tag_wideband_spectrum;	break;
	default:
		return;
	}

	baseband::run_image(image_tag);

	if (modulation == ReceiverModel::Mode::SpectrumAnalysis) {
		baseband::set_spectrum(spec_bw, spec_trigger);
	}

	const auto is_wideband_spectrum_mode = (modulation == ReceiverModel::Mode::SpectrumAnalysis);
	receiver_model.set_modulation(modulation);

	receiver_model.set_sampling_rate(is_wideband_spectrum_mode ? spec_bw : 3072000);
	receiver_model.set_baseband_bandwidth(is_wideband_spectrum_mode ? spec_bw/2 : 1750000);

	receiver_model.enable();

	// TODO: This doesn't belong here! There's a better way.
	size_t sampling_rate = 0;
	switch(modulation) {
	case ReceiverModel::Mode::AMAudio:				sampling_rate = 12000; break;
	case ReceiverModel::Mode::NarrowbandFMAudio:	sampling_rate = 24000; break;
	case ReceiverModel::Mode::WidebandFMAudio:		sampling_rate = 48000; break;
	default:
		break;
	}
	record_view.set_sampling_rate(sampling_rate);

	if( !is_wideband_spectrum_mode ) {
		audio::output::unmute();
	}
}


void AnalogAudioView::handle_coded_squelch(const uint32_t value) {
	float diff, min_diff = value;
	size_t min_idx { 0 };
	size_t c;
	
	// Find nearest match
	for (c = 0; c < tone_keys.size(); c++) {
		diff = abs(((float)value / 100.0) - tone_keys[c].second);
		if (diff < min_diff) {
			min_idx = c;
			min_diff = diff;
		}
	}
	
	// Arbitrary confidence threshold
	if (min_diff < 40)
		text_ctcss.set("CTCSS " + tone_keys[min_idx].first);
	else
		text_ctcss.set("???");
}

} /* namespace ui */
