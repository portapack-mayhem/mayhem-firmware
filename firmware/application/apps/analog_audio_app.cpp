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

#include "audio.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "portapack.hpp"
#include "portapack_persistent_memory.hpp"
#include "string_format.hpp"
#include "ui_freqman.hpp"
#include "utility.hpp"
#include "radio.hpp"

using namespace portapack;
using namespace tonekey;

namespace ui {

/* AMOptionsView *********************************************************/

AMOptionsView::AMOptionsView(
    Rect parent_rect,
    const Style* style)
    : View{parent_rect} {
    set_style(style);

    add_children({
        &label_config,
        &options_config,
    });

    freqman_set_bandwidth_option(AM_MODULATION, options_config);  // adding the common message from freqman.cpp to the options_config
    options_config.set_by_value(receiver_model.am_configuration());
    options_config.on_change = [this](size_t, OptionsField::value_t n) {
        receiver_model.set_am_configuration(n);
    };
}

/* NBFMOptionsView *******************************************************/

NBFMOptionsView::NBFMOptionsView(
    Rect parent_rect,
    const Style* style)
    : View{parent_rect} {
    set_style(style);

    add_children({&label_config,
                  &options_config,
                  &text_squelch,
                  &field_squelch});

    freqman_set_bandwidth_option(NFM_MODULATION, options_config);  // adding the common message from freqman.cpp to the options_config
    options_config.set_by_value(receiver_model.nbfm_configuration());
    options_config.on_change = [this](size_t, OptionsField::value_t n) {
        receiver_model.set_nbfm_configuration(n);
    };

    field_squelch.set_value(receiver_model.squelch_level());
    field_squelch.on_change = [this](int32_t v) {
        receiver_model.set_squelch_level(v);
    };
}

/* WFMOptionsView *******************************************************/

WFMOptionsView::WFMOptionsView(
    Rect parent_rect,
    const Style* style)
    : View{parent_rect} {
    set_style(style);

    add_children({
        &label_config,
        &options_config,
    });

    freqman_set_bandwidth_option(WFM_MODULATION, options_config);  // adding the common message from freqman.cpp to the options_config
    options_config.set_by_value(receiver_model.wfm_configuration());
    options_config.on_change = [this](size_t, OptionsField::value_t n) {
        receiver_model.set_wfm_configuration(n);
    };
}

/* SPECOptionsView *******************************************************/

SPECOptionsView::SPECOptionsView(
    AnalogAudioView* view,
    Rect parent_rect,
    const Style* style)
    : View{parent_rect} {
    set_style(style);

    add_children({
        &label_config,
        &options_config,
        &text_speed,
        &field_speed,
        &text_rx_cal,
        &field_rx_iq_phase_cal,
    });

    options_config.set_selected_index(view->get_spec_bw_index());
    options_config.on_change = [this, view](size_t n, OptionsField::value_t bw) {
        view->set_spec_bw(n, bw);
    };

    field_speed.set_value(view->get_spec_trigger());
    field_speed.on_change = [this, view](int32_t v) {
        view->set_spec_trigger(v);
    };

    field_rx_iq_phase_cal.set_range(0, hackrf_r9 ? 63 : 31);                       // max2839 has 6 bits [0..63],  max2837 has 5 bits [0..31]
    field_rx_iq_phase_cal.set_value(view->get_spec_iq_phase_calibration_value());  // using  accessor function of AnalogAudioView to read iq_phase_calibration_value from rx_audio.ini
    field_rx_iq_phase_cal.on_change = [this, view](int32_t v) {
        view->set_spec_iq_phase_calibration_value(v);  // using  accessor function of AnalogAudioView to write inside SPEC submenu, register value to max283x and save it to rx_audio.ini
    };

    view->set_spec_iq_phase_calibration_value(view->get_spec_iq_phase_calibration_value());  // initialize iq_phase_calibration in radio
}

/* AnalogAudioView *******************************************************/

AnalogAudioView::AnalogAudioView(
    NavigationView& nav)
    : nav_(nav) {
    // A baseband image _must_ be running before add waterfall view.
    baseband::run_image(portapack::spi_flash::image_tag_wideband_spectrum);

    add_children({&rssi,
                  &channel,
                  &audio,
                  &field_frequency,
                  &field_lna,
                  &field_vga,
                  &options_modulation,
                  &field_volume,
                  &text_ctcss,
                  &record_view,
                  &waterfall});

    // Filename Datetime and Frequency
    record_view.set_filename_date_frequency(true);

    field_frequency.on_show_options = [this]() {
        this->on_show_options_frequency();
    };

    field_lna.on_show_options = [this]() {
        this->on_show_options_rf_gain();
    };

    field_vga.on_show_options = [this]() {
        this->on_show_options_rf_gain();
    };

    auto modulation = receiver_model.modulation();
    // This app doesn't handle "Capture" mode.
    if (modulation > ReceiverModel::Mode::SpectrumAnalysis)
        modulation = ReceiverModel::Mode::SpectrumAnalysis;

    options_modulation.set_by_value(toUType(modulation));
    options_modulation.on_change = [this](size_t, OptionsField::value_t v) {
        this->on_modulation_changed(static_cast<ReceiverModel::Mode>(v));
    };
    options_modulation.on_show_options = [this]() {
        this->on_show_options_modulation();
    };

    record_view.on_error = [&nav](std::string message) {
        nav.display_modal("Error", message);
    };

    waterfall.on_select = [this](int32_t offset) {
        field_frequency.set_value(receiver_model.target_frequency() + offset);
    };

    audio::output::start();

    // This call starts the correct baseband image to run
    // and sets the radio up as necessary for the given modulation.
    on_modulation_changed(modulation);
}

AnalogAudioView::AnalogAudioView(
    NavigationView& nav,
    ReceiverModel::settings_t override)
    : AnalogAudioView(nav) {
    // Settings to override when launched from another app (versus from AppSettings .ini file)
    // TODO: Which other settings make sense to override?
    field_frequency.set_value(override.frequency_app_override);
    on_frequency_step_changed(override.frequency_step);
    options_modulation.set_by_value(toUType(override.mode));
}

size_t AnalogAudioView::get_spec_bw_index() {
    return spec_bw_index;
}

void AnalogAudioView::set_spec_bw(size_t index, uint32_t bw) {
    spec_bw_index = index;
    spec_bw = bw;

    baseband::set_spectrum(bw, spec_trigger);
    receiver_model.set_sampling_rate(bw);
    receiver_model.set_baseband_bandwidth(bw / 2);
}

uint8_t AnalogAudioView::get_spec_iq_phase_calibration_value() {  // define accessor functions inside AnalogAudioView to read & write real iq_phase_calibration_value
    return iq_phase_calibration_value;
}

void AnalogAudioView::set_spec_iq_phase_calibration_value(uint8_t cal_value) {  // define accessor functions
    iq_phase_calibration_value = cal_value;
    radio::set_rx_max283x_iq_phase_calibration(iq_phase_calibration_value);
}

uint16_t AnalogAudioView::get_spec_trigger() {
    return spec_trigger;
}

void AnalogAudioView::set_spec_trigger(uint16_t trigger) {
    spec_trigger = trigger;

    baseband::set_spectrum(spec_bw, spec_trigger);
}

AnalogAudioView::~AnalogAudioView() {
    audio::output::stop();
    receiver_model.disable();
    baseband::shutdown();
}

void AnalogAudioView::set_parent_rect(Rect new_parent_rect) {
    View::set_parent_rect(new_parent_rect);

    ui::Rect waterfall_rect{0, header_height, new_parent_rect.width(), new_parent_rect.height() - header_height};
    waterfall.set_parent_rect(waterfall_rect);
}

void AnalogAudioView::focus() {
    field_frequency.focus();
}

void AnalogAudioView::on_baseband_bandwidth_changed(uint32_t bandwidth_hz) {
    receiver_model.set_baseband_bandwidth(bandwidth_hz);
}

void AnalogAudioView::on_modulation_changed(ReceiverModel::Mode modulation) {
    // This app doesn't know what to do with "Capture" mode.
    if (modulation > ReceiverModel::Mode::SpectrumAnalysis)
        modulation = ReceiverModel::Mode::SpectrumAnalysis;

    baseband::spectrum_streaming_stop();
    update_modulation(modulation);
    on_show_options_modulation();
    baseband::spectrum_streaming_start();
}

void AnalogAudioView::remove_options_widget() {
    if (options_widget) {
        remove_child(options_widget.get());
        options_widget.reset();
    }

    field_lna.set_style(nullptr);
    options_modulation.set_style(nullptr);
    field_frequency.set_style(nullptr);
}

void AnalogAudioView::set_options_widget(std::unique_ptr<Widget> new_widget) {
    remove_options_widget();

    if (new_widget) {
        options_widget = std::move(new_widget);
    } else {
        // TODO: Lame hack to hide options view due to my bad paint/damage algorithm.
        options_widget = std::make_unique<Rectangle>(options_view_rect, Theme::getInstance()->option_active->background);
    }
    add_child(options_widget.get());
}

void AnalogAudioView::on_show_options_frequency() {
    auto widget = std::make_unique<FrequencyOptionsView>(options_view_rect, Theme::getInstance()->option_active);

    widget->set_step(receiver_model.frequency_step());
    widget->on_change_step = [this](rf::Frequency f) {
        this->on_frequency_step_changed(f);
    };
    widget->set_reference_ppm_correction(persistent_memory::correction_ppb() / 1000);
    widget->on_change_reference_ppm_correction = [this](int32_t v) {
        this->on_reference_ppm_correction_changed(v);
    };

    set_options_widget(std::move(widget));
    field_frequency.set_style(Theme::getInstance()->option_active);
}

void AnalogAudioView::on_show_options_rf_gain() {
    auto widget = std::make_unique<RadioGainOptionsView>(options_view_rect, Theme::getInstance()->option_active);

    set_options_widget(std::move(widget));
    field_lna.set_style(Theme::getInstance()->option_active);
}

void AnalogAudioView::on_show_options_modulation() {
    std::unique_ptr<Widget> widget;

    const auto modulation = receiver_model.modulation();
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            widget = std::make_unique<AMOptionsView>(options_view_rect, Theme::getInstance()->option_active);
            waterfall.show_audio_spectrum_view(false);
            text_ctcss.hidden(true);
            break;

        case ReceiverModel::Mode::NarrowbandFMAudio:
            widget = std::make_unique<NBFMOptionsView>(nbfm_view_rect, Theme::getInstance()->option_active);
            waterfall.show_audio_spectrum_view(false);
            text_ctcss.hidden(false);
            break;

        case ReceiverModel::Mode::WidebandFMAudio:
            widget = std::make_unique<WFMOptionsView>(options_view_rect, Theme::getInstance()->option_active);
            waterfall.show_audio_spectrum_view(true);
            text_ctcss.hidden(true);
            break;

        case ReceiverModel::Mode::SpectrumAnalysis:
            widget = std::make_unique<SPECOptionsView>(this, nbfm_view_rect, Theme::getInstance()->option_active);
            waterfall.show_audio_spectrum_view(false);
            text_ctcss.hidden(true);
            break;

        default:
            chDbgPanic("Unhandled Mode");
            break;
    }

    set_options_widget(std::move(widget));
    options_modulation.set_style(Theme::getInstance()->option_active);
}

void AnalogAudioView::on_frequency_step_changed(rf::Frequency f) {
    receiver_model.set_frequency_step(f);
    field_frequency.set_step(f);
}

void AnalogAudioView::on_reference_ppm_correction_changed(int32_t v) {
    persistent_memory::set_correction_ppb(v * 1000);
}

void AnalogAudioView::update_modulation(ReceiverModel::Mode modulation) {
    audio::output::mute();
    record_view.stop();

    baseband::shutdown();

    portapack::spi_flash::image_tag_t image_tag;
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            image_tag = portapack::spi_flash::image_tag_am_audio;
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            image_tag = portapack::spi_flash::image_tag_nfm_audio;
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            image_tag = portapack::spi_flash::image_tag_wfm_audio;
            break;
        case ReceiverModel::Mode::SpectrumAnalysis:
            image_tag = portapack::spi_flash::image_tag_wideband_spectrum;
            break;
        default:
            chDbgPanic("Unhandled Mode");
            break;
    }

    baseband::run_image(image_tag);

    if (modulation == ReceiverModel::Mode::SpectrumAnalysis) {
        baseband::set_spectrum(spec_bw, spec_trigger);
    }

    const auto is_wideband_spectrum_mode = (modulation == ReceiverModel::Mode::SpectrumAnalysis);
    receiver_model.set_modulation(modulation);

    receiver_model.set_sampling_rate(is_wideband_spectrum_mode ? spec_bw : 3072000);
    receiver_model.set_baseband_bandwidth(is_wideband_spectrum_mode ? spec_bw / 2 : 1750000);

    receiver_model.enable();

    // TODO: This doesn't belong here! There's a better way.
    size_t sampling_rate = 0;
    switch (modulation) {
        case ReceiverModel::Mode::AMAudio:
            sampling_rate = 12000;
            break;
        case ReceiverModel::Mode::NarrowbandFMAudio:
            sampling_rate = 24000;
            break;
        case ReceiverModel::Mode::WidebandFMAudio:
            sampling_rate = 48000;
            break;
        default:
            break;
    }
    record_view.set_sampling_rate(sampling_rate);

    if (!is_wideband_spectrum_mode) {
        audio::output::unmute();
    }
}

void AnalogAudioView::handle_coded_squelch(uint32_t value) {
    text_ctcss.set(tone_key_string_by_value(value, text_ctcss.parent_rect().width() / 8));
}

} /* namespace ui */
