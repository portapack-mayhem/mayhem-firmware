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
    void set_display_freq(int64_t freq);
    void on_timer();

    uint8_t freq_index = 0;
    rf::Frequency freq_ = {433920000};
    int32_t beep_squelch = 0;
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;
    uint8_t freq_mode = 0;

    app_settings::SettingsManager settings_{
        "rx_detector",
        app_settings::Mode::RX,
        {
            {"beep_squelch"sv, &beep_squelch},
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

    OptionsField field_mode{
        {UI_POS_X(0), UI_POS_Y(1)},
        9,
        {
            {"TETRA UP", 0},
            {"Lora", 1},
            {"Remotes", 2},
        }};

    Text text_frequency{
        {UI_POS_X_RIGHT(20), UI_POS_Y(1), UI_POS_WIDTH(20), UI_POS_DEFAULT_HEIGHT},
        ""};

    Text text_beep_squelch{
        {UI_POS_X_RIGHT(9), UI_POS_Y(2), UI_POS_WIDTH(4), UI_POS_DEFAULT_HEIGHT},
        "Bip>"};

    NumberField field_beep_squelch{
        {UI_POS_X_RIGHT(5), UI_POS_Y(2)},
        4,
        {-100, 20},
        1,
        ' ',
    };

    // RSSI: XX/XX/XXX
    Text freq_stats_rssi{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT},
    };

    // Power: -XXX db
    Text freq_stats_db{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT},
    };

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

    const std::vector<uint32_t> remotes_monitoring_frequencies_hz = {
        // Around 315 MHz (common for older remotes, key fobs in some regions)
        // Window centered on 315 MHz, covers 314.625 - 315.375 MHz
        315000000,

        // Around 433.92 MHz (very common for remotes, sensors, key fobs globally)
        // Window centered on 433.92 MHz, covers 433.545 - 434.295 MHz
        433920000,
    };
    const std::vector<uint32_t> lora_monitoring_frequencies_hz = {
        // EU433 Band (Europe, typically 433.05 MHz to 434.79 MHz)
        // Scanning the approximate range 433.0 MHz to 434.8 MHz with 750kHz steps
        433375000,  // Covers 433.000 - 433.750 MHz
        434125000,  // Covers 433.750 - 434.500 MHz (includes 433.92 MHz)
        434875000,  // Covers 434.500 - 435.250 MHz (covers up to 434.79 MHz)

        // EU868 Band (Europe, typically 863 MHz to 870 MHz, specific channels around 868 MHz)
        // Targeting common LoRaWAN channel groups (approx 867.0 - 868.6 MHz) with 750kHz steps
        867375000,  // Covers 867.000 - 867.750 MHz
        868125000,  // Covers 867.750 - 868.500 MHz
        868875000,  // Covers 868.500 - 869.250 MHz (covers up to 868.6 MHz)

        // US915 Band (North America, typically 902 MHz to 928 MHz, specific channels around 915 MHz)
        // Providing a few sample windows around the 915 MHz area with 750kHz steps.
        // This band is wide; a full scan would require many more frequencies.
        914250000,  // Covers 913.875 - 914.625 MHz
        915000000,  // Covers 914.625 - 915.375 MHz (Centered on 915 MHz)
        915750000,  // Covers 915.375 - 916.125 MHz
    };
    const std::vector<uint32_t> tetra_uplink_monitoring_frequencies_hz = {
        // Band starts at 380,000,000 Hz, ends at 390,000,000 Hz.
        // First center: 380,000,000 + 375,000 = 380,375,000 Hz
        // Last center:  380,375,000 + 13 * 750,000 = 390,125,000 Hz (14 frequencies total for this band)
        380375000,  // Covers 380.000 - 380.750 MHz
        381125000,  // Covers 380.750 - 381.500 MHz
        381875000,  // Covers 381.500 - 382.250 MHz
        382625000,  // Covers 382.250 - 383.000 MHz
        383375000,  // Covers 383.000 - 383.750 MHz
        384125000,  // Covers 383.750 - 384.500 MHz
        384875000,  // Covers 384.500 - 385.250 MHz
        385625000,  // Covers 385.250 - 386.000 MHz
        386375000,  // Covers 386.000 - 386.750 MHz
        387125000,  // Covers 386.750 - 387.500 MHz
        387875000,  // Covers 387.500 - 388.250 MHz
        388625000,  // Covers 388.250 - 389.000 MHz
        389375000,  // Covers 389.000 - 389.750 MHz
        390125000,  // Covers 389.750 - 390.500 MHz
    };
};

}  // namespace ui::external_app::detector_rx

#endif
