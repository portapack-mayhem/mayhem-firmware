/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2018 Furrtek
 * Copyright (C) 2023 gullradriel, Nilorea Studio Inc.
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

#ifndef _UI_LEVEL
#define _UI_LEVEL

#include "analog_audio_app.hpp"
#include "app_settings.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "freqman_db.hpp"
#include "portapack_persistent_memory.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "string_format.hpp"
#include "ui.hpp"
#include "ui_mictx.hpp"
#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"

namespace ui {

class LevelView : public View {
   public:
    LevelView(NavigationView& nav);
    ~LevelView();

    void focus() override;

    std::string title() const override { return "Level"; };

   private:
    NavigationView& nav_;

    RxRadioState radio_state_{};

    int32_t map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh);
    size_t change_mode(freqman_index_t mod_type);
    void on_statistics_update(const ChannelStatistics& statistics);
    void set_display_freq(int64_t freq);
    void m4_manage_stat_update();  // to finely adjust the RxSaturation usage

    rf::Frequency freq_ = {0};
    bool beep = false;
    uint8_t radio_mode = 0;
    uint8_t radio_bw = 0;
    uint8_t audio_mode = 0;
    int32_t beep_squelch = 0;
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;

    app_settings::SettingsManager settings_{
        "rx_level",
        app_settings::Mode::RX,
        {
            {"beep_squelch"sv, &beep_squelch},
            {"audio_mode"sv, &audio_mode},
            {"radio_mode"sv, &radio_mode},
            {"radio_bw"sv, &radio_bw},
        }};

    Labels labels{
        {{0 * 8, 0 * 16}, "LNA:   VGA:   AMP:  VOL:     ", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 1 * 16}, "BW:       MODE:    S:   ", Theme::getInstance()->fg_light->foreground},
    };

    LNAGainField field_lna{
        {4 * 8, 0 * 16}};

    VGAGainField field_vga{
        {11 * 8, 0 * 16}};

    RFAmpField field_rf_amp{
        {18 * 8, 0 * 16}};

    AudioVolumeField field_volume{
        {24 * 8, 0 * 16}};

    OptionsField field_bw{
        {3 * 8, 1 * 16},
        6,
        {}};

    OptionsField field_mode{
        {15 * 8, 1 * 16},
        4,
        {}};

    OptionsField step_mode{
        {16 * 8, 2 * 16 + 4},
        12,
        {}};

    ButtonWithEncoder button_frequency{
        {0 * 8, 2 * 16 + 8, 15 * 8, 1 * 8},
        ""};

    OptionsField field_audio_mode{
        {21 * 8, 1 * 16},
        9,
        {{"audio off", 0},
         {"audio on", 1}}};

    Text text_beep_squelch{
        {21 * 8, 3 * 16 + 4, 4 * 8, 1 * 8},
        "Bip>"};

    NumberField field_beep_squelch{
        {25 * 8, 3 * 16 + 4},
        4,
        {-100, 20},
        1,
        ' ',
    };

    // RSSI: XX/XX/XXX
    Text freq_stats_rssi{
        {0 * 8, 3 * 16 + 4, 15 * 8, 1 * 16},
    };

    // Power: -XXX db
    Text freq_stats_db{
        {0 * 8, 4 * 16 + 4, 15 * 8, 1 * 16},
    };

    OptionsField peak_mode{
        {40 + 10 * 8, 4 * 16 + 4},
        10,
        {
            {"peak:none", 0},
            {"peak:0.25s", 250},
            {"peak:0.5s", 500},
            {"peak:1s", 1000},
            {"peak:3s", 3000},
            {"peak:5s", 5000},
            {"peak:10s", 10000},
        }};

    OptionsField rssi_resolution{
        {44 + 20 * 8, 4 * 16 + 4},
        4,
        {
            {"16x", 16},
            {"32x", 32},
            {"64x", 64},
            {"128x", 128},
            {"240x", 240},
        }};

    // RxSat: XX%
    Text freq_stats_rx{
        {0 * 8, 5 * 16 + 4, 10 * 8, 1 * 16},
    };

    Text text_ctcss{
        {12 * 8, 5 * 16 + 4, 8 * 8, 1 * 8},
        ""};

    RSSIGraph rssi_graph{
        // 240x320  =>
        {0, 6 * 16 + 8, 240 - 5 * 8, 320 - (6 * 16)},
    };

    RSSI rssi{
        // 240x320  =>
        {240 - 5 * 8, 6 * 16 + 8, 5 * 8, 320 - (6 * 16)},
    };

    void handle_coded_squelch(const uint32_t value);

    MessageHandlerRegistration message_handler_coded_squelch{
        Message::ID::CodedSquelch,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const CodedSquelchMessage*>(p);
            this->handle_coded_squelch(message.value);
        }};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};
};

} /* namespace ui */

#endif
