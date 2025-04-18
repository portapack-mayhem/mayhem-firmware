/*
 * Copyright (C) 2024 HTotoo
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

using namespace ui;

namespace ui::external_app::fmradio {

#define FMR_BTNGRID_TOP 60

class FmRadioView : public View {
   public:
    FmRadioView(NavigationView& nav);
    FmRadioView& operator=(const FmRadioView&) = delete;
    FmRadioView(const FmRadioView&) = delete;
    ~FmRadioView();

    void focus() override;

    std::string title() const override { return "FM radio"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    int16_t audio_spectrum[128]{0};
    bool audio_spectrum_update = false;
    AudioSpectrum* audio_spectrum_data{nullptr};
    rf::Frequency freq_fav_list[12] = {0};

    app_settings::SettingsManager settings_{
        "rx_fmradio",
        app_settings::Mode::RX,
        {{"favlist0"sv, &freq_fav_list[0]},
         {"favlist1"sv, &freq_fav_list[1]},
         {"favlist2"sv, &freq_fav_list[2]},
         {"favlist3"sv, &freq_fav_list[3]},
         {"favlist4"sv, &freq_fav_list[4]},
         {"favlist5"sv, &freq_fav_list[5]},
         {"favlist6"sv, &freq_fav_list[6]},
         {"favlist7"sv, &freq_fav_list[7]},
         {"favlist8"sv, &freq_fav_list[8]},
         {"favlist9"sv, &freq_fav_list[9]},
         {"favlist10"sv, &freq_fav_list[10]},
         {"favlist11"sv, &freq_fav_list[11]}}};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};
    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};

    TextField txt_save_help{
        {2, FMR_BTNGRID_TOP + 6 * 34 - 20, 12 * 8, 16},
        " "};

    Audio audio{
        {21 * 8, 10, 6 * 8, 4}};

    Waveform waveform{
        {0, 20, 30 * 8, 2 * 16},
        audio_spectrum,
        128,
        0,
        false,
        Theme::getInstance()->bg_darkest->foreground,
        true};

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
