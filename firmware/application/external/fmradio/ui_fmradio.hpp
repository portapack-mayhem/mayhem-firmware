/*
 * Copyright (C) 2024 HTotoo
 * Copyright (C) 2025 RocketGod
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

/*
    FUTURE TODO: implement search function.
*/

#ifndef __UI_fmradio_H__
#define __UI_fmradio_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"
#include "ui_freq_field.hpp"
#include "ui_spectrum.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"
#include "audio.hpp"
#include "freqman_db.hpp"
#include "ui_freqman.hpp"

using namespace ui;

namespace ui::external_app::fmradio {

#include "external/ui_grapheq.hpp"

#define FMR_BTNGRID_TOP 60

class FmRadioView : public View {
   public:
    FmRadioView(NavigationView& nav);
    FmRadioView& operator=(const FmRadioView&) = delete;
    FmRadioView(const FmRadioView&) = delete;
    ~FmRadioView();

    void focus() override;

    std::string title() const override { return "Radio"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    int16_t audio_spectrum[128]{0};
    bool audio_spectrum_update = false;
    ReceiverModel::Mode receiver_mode = ReceiverModel::Mode::WidebandFMAudio;
    AudioSpectrum* audio_spectrum_data{nullptr};
    struct Favorite {
        rf::Frequency frequency = 0;
        int32_t modulation = static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio);
        uint8_t bandwidth = 0;
    };
    Favorite freq_fav_list[12];
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;
    uint8_t radio_bw = 0;
    uint32_t current_theme{0};
    app_settings::SettingsManager settings_{
        "rx_fmradio",
        app_settings::Mode::RX,
        {{"favlist0_freq"sv, &freq_fav_list[0].frequency},
         {"favlist1_freq"sv, &freq_fav_list[1].frequency},
         {"favlist2_freq"sv, &freq_fav_list[2].frequency},
         {"favlist3_freq"sv, &freq_fav_list[3].frequency},
         {"favlist4_freq"sv, &freq_fav_list[4].frequency},
         {"favlist5_freq"sv, &freq_fav_list[5].frequency},
         {"favlist6_freq"sv, &freq_fav_list[6].frequency},
         {"favlist7_freq"sv, &freq_fav_list[7].frequency},
         {"favlist8_freq"sv, &freq_fav_list[8].frequency},
         {"favlist9_freq"sv, &freq_fav_list[9].frequency},
         {"favlist10_freq"sv, &freq_fav_list[10].frequency},
         {"favlist11_freq"sv, &freq_fav_list[11].frequency},
         {"favlist0_mod"sv, &freq_fav_list[0].modulation},
         {"favlist1_mod"sv, &freq_fav_list[1].modulation},
         {"favlist2_mod"sv, &freq_fav_list[2].modulation},
         {"favlist3_mod"sv, &freq_fav_list[3].modulation},
         {"favlist4_mod"sv, &freq_fav_list[4].modulation},
         {"favlist5_mod"sv, &freq_fav_list[5].modulation},
         {"favlist6_mod"sv, &freq_fav_list[6].modulation},
         {"favlist7_mod"sv, &freq_fav_list[7].modulation},
         {"favlist8_mod"sv, &freq_fav_list[8].modulation},
         {"favlist9_mod"sv, &freq_fav_list[9].modulation},
         {"favlist10_mod"sv, &freq_fav_list[10].modulation},
         {"favlist11_mod"sv, &freq_fav_list[11].modulation},
         {"favlist0_bw"sv, &freq_fav_list[0].bandwidth},
         {"favlist1_bw"sv, &freq_fav_list[1].bandwidth},
         {"favlist2_bw"sv, &freq_fav_list[2].bandwidth},
         {"favlist3_bw"sv, &freq_fav_list[3].bandwidth},
         {"favlist4_bw"sv, &freq_fav_list[4].bandwidth},
         {"favlist5_bw"sv, &freq_fav_list[5].bandwidth},
         {"favlist6_bw"sv, &freq_fav_list[6].bandwidth},
         {"favlist7_bw"sv, &freq_fav_list[7].bandwidth},
         {"favlist8_bw"sv, &freq_fav_list[8].bandwidth},
         {"favlist9_bw"sv, &freq_fav_list[9].bandwidth},
         {"favlist10_bw"sv, &freq_fav_list[10].bandwidth},
         {"favlist11_bw"sv, &freq_fav_list[11].bandwidth},
         {"radio_bw"sv, &radio_bw},
         {"theme"sv, &current_theme}}};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};
    AudioVolumeField field_volume{
        {screen_width - 2 * 8, 0 * 16}};

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    OptionsField field_bw{
        {10 * 8, FMR_BTNGRID_TOP + 6 * 34},
        6,
        {}};

    Text text_mode_label{
        {20 * 8, FMR_BTNGRID_TOP + 6 * 34, 5 * 8, 1 * 28},
        "MODE:"};

    OptionsField field_modulation{
        {26 * 8, FMR_BTNGRID_TOP + 6 * 34},
        4,
        {{"AM", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)},
         {"NFM", static_cast<int32_t>(ReceiverModel::Mode::NarrowbandFMAudio)},
         {"WFM", static_cast<int32_t>(ReceiverModel::Mode::WidebandFMAudio)},
         {"USB", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)},
         {"LSB", static_cast<int32_t>(ReceiverModel::Mode::AMAudio)}}};

    TextField txt_save_help{
        {2, FMR_BTNGRID_TOP + 6 * 34 - 20, 12 * 8, 16},
        " "};

    Audio audio{
        {21 * 8, 10, 6 * 8, 4}};

    Waveform waveform{
        {0, 20, UI_POS_MAXWIDTH, 2 * 16},
        audio_spectrum,
        128,
        0,
        false,
        Theme::getInstance()->bg_darkest->foreground,
        true};

    GraphEq gr{{2, FMR_BTNGRID_TOP, UI_POS_MAXWIDTH - 4, UI_POS_MAXHEIGHT - FMR_BTNGRID_TOP}, true};

    Button btn_fav_0{{2, FMR_BTNGRID_TOP + 0 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_1{{2 + 15 * 8, FMR_BTNGRID_TOP + 0 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_2{{2, FMR_BTNGRID_TOP + 1 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_3{{2 + 15 * 8, FMR_BTNGRID_TOP + 1 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_4{{2, FMR_BTNGRID_TOP + 2 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_5{{2 + 15 * 8, FMR_BTNGRID_TOP + 2 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_6{{2, FMR_BTNGRID_TOP + 3 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_7{{2 + 15 * 8, FMR_BTNGRID_TOP + 3 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_8{{2, FMR_BTNGRID_TOP + 4 * 34, 10 * 8, 28}, "---"};
    Button btn_fav_9{{2 + 15 * 8, FMR_BTNGRID_TOP + 4 * 34, 10 * 8, 28}, "---"};

    Button btn_fav_save{{2, FMR_BTNGRID_TOP + 6 * 34, 7 * 8, 1 * 28}, "Save"};
    bool save_fav = false;

    void on_btn_clicked(uint8_t i);
    void update_fav_btn_texts();
    std::string to_nice_freq(rf::Frequency freq);
    void on_audio_spectrum();
    void change_mode(int32_t mod);

    void show_hide_gfx(bool show);

    struct ColorTheme {
        Color base_color;
        Color peak_color;
    };

    const std::array<ColorTheme, 20> themes{
        ColorTheme{Color(255, 0, 255), Color(255, 255, 255)},
        ColorTheme{Color(0, 255, 0), Color(255, 0, 0)},
        ColorTheme{Color(0, 0, 255), Color(255, 255, 0)},
        ColorTheme{Color(255, 128, 0), Color(255, 0, 128)},
        ColorTheme{Color(128, 0, 255), Color(0, 255, 255)},
        ColorTheme{Color(255, 255, 0), Color(0, 255, 128)},
        ColorTheme{Color(255, 0, 0), Color(0, 128, 255)},
        ColorTheme{Color(0, 255, 128), Color(255, 128, 255)},
        ColorTheme{Color(128, 128, 128), Color(255, 255, 255)},
        ColorTheme{Color(255, 64, 0), Color(0, 255, 64)},
        ColorTheme{Color(0, 128, 128), Color(255, 192, 0)},
        ColorTheme{Color(0, 255, 0), Color(0, 128, 0)},
        ColorTheme{Color(32, 64, 32), Color(0, 255, 0)},
        ColorTheme{Color(64, 0, 128), Color(255, 0, 255)},
        ColorTheme{Color(0, 64, 0), Color(0, 255, 128)},
        ColorTheme{Color(255, 255, 255), Color(0, 0, 255)},
        ColorTheme{Color(128, 0, 0), Color(255, 128, 0)},
        ColorTheme{Color(0, 128, 255), Color(255, 255, 128)},
        ColorTheme{Color(64, 64, 64), Color(255, 0, 0)},
        ColorTheme{Color(255, 192, 0), Color(0, 64, 128)}};

    MessageHandlerRegistration message_handler_audio_spectrum{
        Message::ID::AudioSpectrum,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const AudioSpectrumMessage*>(p);
            audio_spectrum_data = message.data;
            audio_spectrum_update = true;
        }};
    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            if (audio_spectrum_update) {
                audio_spectrum_update = false;
                on_audio_spectrum();
            }
        }};
};

}  // namespace ui::external_app::fmradio

#endif /*__UI_fmradio_H__*/
