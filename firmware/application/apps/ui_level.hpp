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
#include "ui_styles.hpp"

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
    app_settings::SettingsManager settings_{
        "rx_level", app_settings::Mode::RX};

    size_t change_mode(freqman_index_t mod_type);
    void on_statistics_update(const ChannelStatistics& statistics);
    void set_display_freq(int64_t freq);

    // TODO: needed?
    int32_t db{0};
    rf::Frequency freq_ = {0};

    Labels labels{
        {{0 * 8, 0 * 16}, "LNA:   VGA:   AMP:  VOL:     ", Color::light_grey()},
        {{0 * 8, 1 * 16}, "BW:       MODE:    S:   ", Color::light_grey()},
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

    OptionsField audio_mode{
        {21 * 8, 1 * 16},
        9,
        {
            {"audio off", 0},
            {"audio on", 1}
            //{"tone on", 2},
            //{"tone off", 3},
        }};

    Text text_ctcss{
        {22 * 8, 3 * 16 + 4, 8 * 8, 1 * 8},
        ""};

    // RSSI: XX/XX/XXX,dt: XX
    Text freq_stats_rssi{
        {0 * 8, 3 * 16 + 4, 22 * 8, 14},
    };

    // Power: -XXX db
    Text freq_stats_db{
        {0 * 8, 4 * 16 + 4, 14 * 8, 14},
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

    RSSIGraph rssi_graph{
        // 240x320  =>
        {0, 5 * 16 + 4, 240 - 5 * 8, 320 - (5 * 16 + 4)},
    };

    RSSI rssi{
        // 240x320  =>
        {240 - 5 * 8, 5 * 16 + 4, 5 * 8, 320 - (5 * 16 + 4)},
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
