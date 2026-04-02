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

#ifndef _UI_DETECTOR_RX
#define _UI_DETECTOR_RX

#include "analog_audio_app.hpp"
#include "app_settings.hpp"
#include "audio.hpp"
#include "baseband_api.hpp"
#include "file.hpp"
#include "freqman.hpp"
#include "freqman_db.hpp"
#include "portapack_persistent_memory.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "string_format.hpp"
#include "ui.hpp"
#include "ui_mictx.hpp"
#include "ui_receiver.hpp"
#include "ui_spectrum.hpp"

namespace ui::external_app::detector_rx {

#define DETECTOR_BW 750000

class DetectorRxView : public View {
   public:
    DetectorRxView(NavigationView& nav);
    ~DetectorRxView();

    void focus() override;

    std::string title() const override { return "Detector RX"; };

   private:
    NavigationView& nav_;

    RxRadioState radio_state_{};

    int32_t map(int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh);
    size_t change_mode();
    void on_statistics_update(const ChannelStatistics& statistics);
    void on_timer();
    void load_freqman();
    void init_current_entry();
    void update_entry_display();
    void update_freq_display();
    std::string format_freq_mhz(int64_t freq_hz);

    freqman_db frequency_list{};
    size_t current_index{0};
    int64_t current_freq{0};
    int64_t minfreq{0};
    int64_t maxfreq{0};
    int32_t current_step{DETECTOR_BW};
    std::string freq_file_stem{"DETECTOR"};
    bool auto_scan{true};
    bool auto_advance{false};
    uint8_t last_update_was_auto_range_type = -1;

    int32_t beep_squelch = 0;
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;

    app_settings::SettingsManager settings_{
        "rx_detector",
        app_settings::Mode::RX,
        {
            {"beep_squelch"sv, &beep_squelch},
            {"freq_file"sv, &freq_file_stem},
            {"auto_scan"sv, &auto_scan},
            {"auto_advance"sv, &auto_advance},
        }};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "LNA:   VGA:   AMP:  ", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_RIGHT(6), UI_POS_Y(0)}, "VOL:  ", Theme::getInstance()->fg_light->foreground},
    };

    LNAGainField field_lna{
        {UI_POS_X(4), UI_POS_Y(0)}};

    VGAGainField field_vga{
        {UI_POS_X(11), UI_POS_Y(0)}};

    RFAmpField field_rf_amp{
        {UI_POS_X(18), UI_POS_Y(0)}};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    // Row 1: filename + auto advance
    Button button_file{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(20), UI_POS_DEFAULT_HEIGHT},
        ""};

    Button button_auto_advance{
        {UI_POS_X_RIGHT(9), UI_POS_Y(1), UI_POS_WIDTH(9), UI_POS_DEFAULT_HEIGHT},
        "NO ADV"};

    // Row 2: index encoder + description + auto scan
    ButtonWithEncoder button_index{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(4), UI_POS_DEFAULT_HEIGHT},
        ""};

    Text text_entry_desc{
        {UI_POS_X(4), UI_POS_Y(2), UI_POS_WIDTH(17), UI_POS_DEFAULT_HEIGHT},
        ""};

    Button button_auto_scan{
        {UI_POS_X_RIGHT(9), UI_POS_Y(2), UI_POS_WIDTH(9), UI_POS_DEFAULT_HEIGHT},
        "AUTO SCAN"};

    // Row 3: frequency encoder + bip level
    ButtonWithEncoder button_freq{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(20), UI_POS_DEFAULT_HEIGHT},
        ""};

    Text text_beep_squelch{
        {UI_POS_X_RIGHT(9), UI_POS_Y(3), UI_POS_WIDTH(4), UI_POS_DEFAULT_HEIGHT},
        "Bip>"};

    NumberField field_beep_squelch{
        {UI_POS_X_RIGHT(5), UI_POS_Y(3)},
        4,
        {-100, 20},
        1,
        ' ',
    };

    // Row 4: Power + RSSI on same line
    Text freq_stats_db{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT},
    };

    Text freq_stats_rssi{
        {UI_POS_X(15), UI_POS_Y(4), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT},
    };

    // Row 5+: RSSI graph + vertical bar
    RSSIGraph rssi_graph{
        {UI_POS_X(0), UI_POS_Y(5), UI_POS_WIDTH_REMAINING(5), UI_POS_HEIGHT_REMAINING(6)},
    };

    RSSI rssi{
        {UI_POS_X_RIGHT(5), UI_POS_Y(5), UI_POS_WIDTH(5), UI_POS_HEIGHT_REMAINING(6)},
    };

    MessageHandlerRegistration message_handler_stats{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

}  // namespace ui::external_app::detector_rx

#endif
