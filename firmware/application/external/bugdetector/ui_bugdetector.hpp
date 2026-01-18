/*
 * Bug Detector App for PortaPack Mayhem
 * Header file with UI declarations
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

#ifndef _UI_BUGDETECTOR
#define _UI_BUGDETECTOR

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

namespace ui::external_app::bugdetector {

#define BUGDETECTOR_BW 1000000

class BugDetectorView : public View {
   public:
    BugDetectorView(NavigationView& nav);
    ~BugDetectorView();

    void focus() override;

    std::string title() const override { return "Bug Detector"; };

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
    int32_t beep_threshold = -60;
    int32_t alert_threshold = -40;
    audio::Rate audio_sampling_rate = audio::Rate::Hz_48000;
    uint8_t freq_mode = 0;

    app_settings::SettingsManager settings_{
        "bugdetector",
        app_settings::Mode::RX,
        {
            {"beep_threshold"sv, &beep_threshold},
            {"alert_threshold"sv, &alert_threshold},
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
        12,
        {
            {"All Bugs", 0},
            {"RF Bugs", 1},
            {"Cameras", 2},
            {"WiFi/BT", 3},
            {"GSM", 4},
        }};

    Text text_frequency{
        {UI_POS_X_RIGHT(20), UI_POS_Y(1), UI_POS_WIDTH(20), UI_POS_DEFAULT_HEIGHT},
        ""};

    Text text_alert{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH_REMAINING(10), UI_POS_DEFAULT_HEIGHT},
        ""};

    Text text_beep_threshold{
        {UI_POS_X_RIGHT(9), UI_POS_Y(3), UI_POS_WIDTH(5), UI_POS_DEFAULT_HEIGHT},
        "Beep>"};

    NumberField field_beep_threshold{
        {UI_POS_X_RIGHT(4), UI_POS_Y(3)},
        4,
        {-100, 20},
        1,
        ' ',
    };

    Text text_alert_threshold{
        {UI_POS_X_RIGHT(9), UI_POS_Y(4), UI_POS_WIDTH(5), UI_POS_DEFAULT_HEIGHT},
        "Alrt>"};

    NumberField field_alert_threshold{
        {UI_POS_X_RIGHT(4), UI_POS_Y(4)},
        4,
        {-100, 20},
        1,
        ' ',
    };

    // RSSI: XX/XX/XXX
    Text freq_stats_rssi{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT},
    };

    // Power: -XXX db
    Text freq_stats_db{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(15), UI_POS_DEFAULT_HEIGHT},
    };

    RSSIGraph rssi_graph{
        {UI_POS_X(0), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(5), UI_POS_HEIGHT_REMAINING(6)},
    };

    RSSI rssi{
        {UI_POS_X_RIGHT(5), UI_POS_Y(6), UI_POS_WIDTH(5), UI_POS_HEIGHT_REMAINING(6)},
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

    // Common bug/surveillance device frequencies
    const std::vector<uint32_t> all_bug_frequencies_hz = {
        // RF Bugs & Wireless Cameras (Sub-GHz)
        315000000,   // 315 MHz - Common RF bugs, wireless cameras
        433920000,   // 433.92 MHz - Very common RF bugs, cameras, transmitters

        // GSM/Cellular bands (simplified for common regions)
        900000000,   // GSM 900 (uplink)
        915000000,   // GSM 900 MHz band
        930000000,   // GSM 900 (downlink)

        // Wireless Cameras (1.2 GHz range)
        1200000000,  // 1.2 GHz wireless cameras
        1250000000,  // 1.25 GHz cameras
        1300000000,  // 1.3 GHz cameras

        // WiFi & Bluetooth (2.4 GHz)
        2400000000,  // 2.4 GHz WiFi Channel 1
        2425000000,  // 2.4 GHz WiFi Channel 6
        2450000000,  // 2.4 GHz WiFi Channel 11
        2475000000,  // 2.4 GHz WiFi Channel 14
    };

    const std::vector<uint32_t> rf_bug_frequencies_hz = {
        315000000,   // 315 MHz
        433920000,   // 433.92 MHz
        868000000,   // 868 MHz (EU)
    };

    const std::vector<uint32_t> camera_frequencies_hz = {
        315000000,   // 315 MHz cameras
        433920000,   // 433.92 MHz cameras
        900000000,   // 900 MHz cameras
        1200000000,  // 1.2 GHz
        1250000000,  // 1.25 GHz
        1300000000,  // 1.3 GHz
        2400000000,  // 2.4 GHz WiFi cameras
    };

    const std::vector<uint32_t> wifi_bt_frequencies_hz = {
        2400000000,  // WiFi Ch 1
        2412000000,  // WiFi Ch 1 (exact)
        2425000000,  // WiFi Ch 6
        2437000000,  // WiFi Ch 6 (exact)
        2450000000,  // WiFi Ch 11
        2462000000,  // WiFi Ch 11 (exact)
        2475000000,  // WiFi Ch 14
    };

    const std::vector<uint32_t> gsm_frequencies_hz = {
        880000000,   // GSM 900 uplink start
        900000000,   // GSM 900 uplink
        915000000,   // GSM 900
        925000000,   // GSM 900 downlink start
        960000000,   // GSM 900 downlink end
        1710000000,  // GSM 1800 uplink start
        1805000000,  // GSM 1800 downlink start
        1850000000,  // GSM 1900 uplink
        1930000000,  // GSM 1900 downlink
    };
};

}  // namespace ui::external_app::bugdetector

#endif
