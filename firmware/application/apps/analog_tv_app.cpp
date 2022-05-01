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

#include "analog_tv_app.hpp"

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

/* AnalogTvView *******************************************************/

AnalogTvView::AnalogTvView(
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
		&tv
	});


	// load app settings
	auto rc = settings.load("rx_tv", &app_settings);
	if(rc == SETTINGS_OK) {
		field_lna.set_value(app_settings.lna);
		field_vga.set_value(app_settings.vga);
		receiver_model.set_rf_amp(app_settings.rx_amp);
		field_frequency.set_value(app_settings.rx_frequency);
	}
	else field_frequency.set_value(receiver_model.tuning_frequency());


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
	options_modulation.set_by_value(toUType(ReceiverModel::Mode::WidebandFMAudio));
	options_modulation.on_change = [this](size_t, OptionsField::value_t v) {
		this->on_modulation_changed(static_cast<ReceiverModel::Mode>(v));
	};
	options_modulation.on_show_options = [this]() {
		this->on_show_options_modulation();
	};

	field_volume.set_value(0);
	field_volume.on_change = [this](int32_t v) {
		this->on_headphone_volume_changed(v);
	};
	
	tv.on_select = [this](int32_t offset) {
		field_frequency.set_value(receiver_model.tuning_frequency() + offset);
	};

	update_modulation(static_cast<ReceiverModel::Mode>(modulation));
        on_modulation_changed(ReceiverModel::Mode::WidebandFMAudio);
}

AnalogTvView::~AnalogTvView() {

	// save app settings
	app_settings.rx_frequency = field_frequency.value();
	settings.save("rx_tv", &app_settings);

	// TODO: Manipulating audio codec here, and in ui_receiver.cpp. Good to do
	// both?
	audio::output::stop();
	receiver_model.disable();
	baseband::shutdown();
}

void AnalogTvView::on_hide() {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	tv.on_hide();
	View::on_hide();
}

void AnalogTvView::set_parent_rect(const Rect new_parent_rect) {
	View::set_parent_rect(new_parent_rect);
	
	const ui::Rect tv_rect { 0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height };
	tv.set_parent_rect(tv_rect);
}

void AnalogTvView::focus() {
	field_frequency.focus();
}

void AnalogTvView::on_tuning_frequency_changed(rf::Frequency f) {
	receiver_model.set_tuning_frequency(f);
}

void AnalogTvView::on_baseband_bandwidth_changed(uint32_t bandwidth_hz) {
	receiver_model.set_baseband_bandwidth(bandwidth_hz);
}

void AnalogTvView::on_modulation_changed(const ReceiverModel::Mode modulation) {
	// TODO: Terrible kludge because widget system doesn't notify Waterfall that
	// it's being shown or hidden.
	tv.on_hide();
	update_modulation(modulation);
	on_show_options_modulation();
	tv.on_show();
}

void AnalogTvView::remove_options_widget() {
	if( options_widget ) {
		remove_child(options_widget.get());
		options_widget.reset();
	}
	
	field_lna.set_style(nullptr);
	options_modulation.set_style(nullptr);
	field_frequency.set_style(nullptr);
}

void AnalogTvView::set_options_widget(std::unique_ptr<Widget> new_widget) {
	remove_options_widget();

	if( new_widget ) {
		options_widget = std::move(new_widget);
	} else {
		// TODO: Lame hack to hide options view due to my bad paint/damage algorithm.
		options_widget = std::make_unique<Rectangle>(options_view_rect, style_options_group_new.background);
	}
	add_child(options_widget.get());
}

void AnalogTvView::on_show_options_frequency() {
	auto widget = std::make_unique<FrequencyOptionsView>(options_view_rect, &style_options_group_new);

	widget->set_step(receiver_model.frequency_step());
	widget->on_change_step = [this](rf::Frequency f) {
		this->on_frequency_step_changed(f);
	};
	widget->set_reference_ppm_correction(persistent_memory::correction_ppb() / 1000);
	widget->on_change_reference_ppm_correction = [this](int32_t v) {
		this->on_reference_ppm_correction_changed(v);
	};

	set_options_widget(std::move(widget));
	field_frequency.set_style(&style_options_group_new);
}

void AnalogTvView::on_show_options_rf_gain() {
	auto widget = std::make_unique<RadioGainOptionsView>(options_view_rect, &style_options_group_new);

	set_options_widget(std::move(widget));
	field_lna.set_style(&style_options_group_new);
}

void AnalogTvView::on_show_options_modulation() {
	std::unique_ptr<Widget> widget;

	static_cast<ReceiverModel::Mode>(receiver_model.modulation());
	tv.show_audio_spectrum_view(true);
	
	set_options_widget(std::move(widget));
	options_modulation.set_style(&style_options_group_new);
}

void AnalogTvView::on_frequency_step_changed(rf::Frequency f) {
	receiver_model.set_frequency_step(f);
	field_frequency.set_step(f);
}

void AnalogTvView::on_reference_ppm_correction_changed(int32_t v) {
	persistent_memory::set_correction_ppb(v * 1000);
}

void AnalogTvView::on_headphone_volume_changed(int32_t v) {
	(void)v; //avoid warning
	//tv::TVView::set_headphone_volume(this,v);
}

void AnalogTvView::update_modulation(const ReceiverModel::Mode modulation) {
	audio::output::mute();

	baseband::shutdown();

	portapack::spi_flash::image_tag_t image_tag;
	image_tag = portapack::spi_flash::image_tag_am_tv;	

	baseband::run_image(image_tag);

	receiver_model.set_modulation(modulation);
        receiver_model.set_sampling_rate(2000000);
        receiver_model.set_baseband_bandwidth(2000000);
	receiver_model.enable();
}

} /* namespace ui */
