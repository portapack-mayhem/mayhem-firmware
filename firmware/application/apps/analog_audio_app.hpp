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
#include "ui_freq_field.hpp"
#include "ui_spectrum.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "tone_key.hpp"

namespace ui {

class AMOptionsView : public View {
   public:
    AMOptionsView(Rect parent_rect, const Style* style);

   private:
    Text label_config{
        {0 * 8, 0 * 16, 2 * 8, 1 * 16},
        "BW",
    };

    OptionsField options_config{
        {3 * 8, 0 * 16},
        6,  // Max option length
        {
            // Using common messages from freqman_ui.cpp
        }};
};

class NBFMOptionsView : public View {
   public:
    NBFMOptionsView(Rect parent_rect, const Style* style);

   private:
    Text label_config{
        {0 * 8, 0 * 16, 2 * 8, 1 * 16},
        "BW",
    };
    OptionsField options_config{
        {3 * 8, 0 * 16},
        3,  // Max option length
        {
            // Using common messages from freqman_ui.cpp
        }};

    Text text_squelch{
        {7 * 8, 0 * 16, 8 * 8, 1 * 16},
        "SQ   /99"};
    NumberField field_squelch{
        {10 * 8, 0 * 16},
        2,
        {0, 99},
        1,
        ' ',
    };
};

class WFMOptionsView : public View {
   public:
    WFMOptionsView(Rect parent_rect, const Style* style);

   private:
    Text label_config{
        {0 * 8, 0 * 16, 2 * 8, 1 * 16},
        "BW",
    };
    OptionsField options_config{
        {3 * 8, 0 * 16},
        4,  // Max option length
        {
            // Using common messages from freqman_ui.cpp
        }};
};

class AnalogAudioView;

class SPECOptionsView : public View {
   public:
    SPECOptionsView(AnalogAudioView* view, Rect parent_rect, const Style* style);

   private:
    Text label_config{
        {0 * 8, 0 * 16, 2 * 8, 1 * 16},
        "BW",
    };
    OptionsField options_config{
        {3 * 8, 0 * 16},
        4,
        {
            {"20m ", 20000000},
            {"10m ", 10000000},
            {" 5m ", 5000000},
            {" 2m ", 2000000},
            {" 1m ", 1000000},
            {"500k", 500000},
            {"100k", 100000},
        }};

    Text text_speed{
        {9 * 8, 0 * 16, 8 * 8, 1 * 16},
        "SP   /63"};
    NumberField field_speed{
        {12 * 8, 0 * 16},
        2,
        {0, 63},
        1,
        ' ',
    };
    Text text_rx_cal{
        {19 * 8, 0 * 16, 11 * 8, 1 * 16},  // 18 (x col.) x char_size,  12 (length) x 8 blanking space to delete previous chars.
        "Rx_IQ_CAL  "};
    NumberField field_rx_iq_phase_cal{
        {28 * 8, 0 * 16},
        2,
        {0, 63},  // 5 or 6 bits IQ CAL phase adjustment (range updated later)
        1,
        ' ',
    };
};

class AnalogAudioView : public View {
   public:
    AnalogAudioView(NavigationView& nav);
    AnalogAudioView(NavigationView& nav, ReceiverModel::settings_t override);
    ~AnalogAudioView();

    void set_parent_rect(Rect new_parent_rect) override;
    void focus() override;

    std::string title() const override { return "Audio RX"; };

    size_t get_spec_bw_index();
    void set_spec_bw(size_t index, uint32_t bw);

    uint16_t get_spec_trigger();
    void set_spec_trigger(uint16_t trigger);

    uint8_t get_spec_iq_phase_calibration_value();
    void set_spec_iq_phase_calibration_value(uint8_t cal_value);

   private:
    static constexpr ui::Dim header_height = 3 * 16;

    NavigationView& nav_;
    RxRadioState radio_state_{};
    uint8_t iq_phase_calibration_value{15};  // initial default RX IQ phase calibration value , used for both max2837 & max2839
    app_settings::SettingsManager settings_{
        "rx_audio",
        app_settings::Mode::RX,
        {
            {"iq_phase_calibration"sv, &iq_phase_calibration_value},  // we are saving and restoring that CAL from Settings.
        }};

    const Rect options_view_rect{0 * 8, 1 * 16, 30 * 8, 1 * 16};
    const Rect nbfm_view_rect{0 * 8, 1 * 16, 18 * 8, 1 * 16};

    size_t spec_bw_index = 0;
    uint32_t spec_bw = 20000000;
    uint16_t spec_trigger = 63;

    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};

    Channel channel{
        {21 * 8, 5, 6 * 8, 4}};

    Audio audio{
        {21 * 8, 10, 6 * 8, 4}};

    RxFrequencyField field_frequency{
        {5 * 8, 0 * 16},
        nav_};

    LNAGainField field_lna{
        {15 * 8, 0 * 16}};

    VGAGainField field_vga{
        {18 * 8, 0 * 16}};

    OptionsField options_modulation{
        {0 * 8, 0 * 16},
        4,
        {
            {" AM ", toUType(ReceiverModel::Mode::AMAudio)},
            {"NFM ", toUType(ReceiverModel::Mode::NarrowbandFMAudio)},
            {"WFM ", toUType(ReceiverModel::Mode::WidebandFMAudio)},
            {"SPEC", toUType(ReceiverModel::Mode::SpectrumAnalysis)},
        }};

    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    Text text_ctcss{
        {16 * 8, 1 * 16, 14 * 8, 1 * 16},
        ""};

    std::unique_ptr<Widget> options_widget{};

    RecordView record_view{
        {0 * 8, 2 * 16, 30 * 8, 1 * 16},
        u"AUD",
        u"AUDIO",
        RecordView::FileType::WAV,
        4096,
        4};

    spectrum::WaterfallView waterfall{true};

    void on_baseband_bandwidth_changed(uint32_t bandwidth_hz);
    void on_modulation_changed(ReceiverModel::Mode modulation);
    void on_show_options_frequency();
    void on_show_options_rf_gain();
    void on_show_options_modulation();
    void on_frequency_step_changed(rf::Frequency f);
    void on_reference_ppm_correction_changed(int32_t v);

    void remove_options_widget();
    void set_options_widget(std::unique_ptr<Widget> new_widget);

    void update_modulation(ReceiverModel::Mode modulation);

    void handle_coded_squelch(uint32_t value);

    MessageHandlerRegistration message_handler_coded_squelch{
        Message::ID::CodedSquelch,
        [this](const Message* p) {
            const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
            this->handle_coded_squelch(message.value);
        }};
};

} /* namespace ui */

#endif /*__ANALOG_AUDIO_APP_H__*/
