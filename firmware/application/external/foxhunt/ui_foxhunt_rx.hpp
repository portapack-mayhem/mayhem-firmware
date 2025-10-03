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

#ifndef __UI_FOXHUNT_RX_H__
#define __UI_FOXHUNT_RX_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::foxhunt_rx {

class FoxhuntRxView : public View {
   public:
    FoxhuntRxView(NavigationView& nav);
    ~FoxhuntRxView();

    void focus() override;

    std::string title() const override { return "Fox hunt"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_foxhunt", app_settings::Mode::RX};

    RFAmpField field_rf_amp{
        {13 * 8, UI_POS_Y(0)}};
    LNAGainField field_lna{
        {15 * 8, UI_POS_Y(0)}};
    VGAGainField field_vga{
        {18 * 8, UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), 0, UI_POS_WIDTH_REMAINING(24), 4}};
    AudioVolumeField field_volume{
        {screen_width - 2 * 8, UI_POS_Y(0)}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    // Power: -XXX db
    Text freq_stats_db{
        {UI_POS_X(0), 2 * 16 + 4, 14 * 8, 14},
    };
    RSSIGraph rssi_graph{
        {0, 50, screen_width, 30},
    };

    Button clear_markers{
        {10 * 8, 18, 8 * 8, 16},
        LanguageHelper::currentMessages[LANG_CLEAR]};

    Button add_current_marker{
        {2, 18, 7 * 8, 16},
        "Mark"};

    GeoMap geomap{{0, 80, screen_width, screen_height - 80}};

    MessageHandlerRegistration message_handler_gps{
        Message::ID::GPSPosData,
        [this](Message* const p) {
            const auto message = static_cast<const GPSPosDataMessage*>(p);
            this->on_gps(message);
        }};
    MessageHandlerRegistration message_handler_orientation{
        Message::ID::OrientationData,
        [this](Message* const p) {
            const auto message = static_cast<const OrientationDataMessage*>(p);
            this->on_orientation(message);
        }};
    MessageHandlerRegistration message_handler_stats{
        Message::ID::ChannelStatistics,
        [this](const Message* const p) {
            this->on_statistics_update(static_cast<const ChannelStatisticsMessage*>(p)->statistics);
        }};
    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    void on_freqchg(int64_t freq);

    float my_lat = 200;
    float my_lon = 200;
    uint16_t my_orientation = 400;

    void on_gps(const GPSPosDataMessage* msg);
    void on_orientation(const OrientationDataMessage* msg);
    void on_statistics_update(const ChannelStatistics& statistics);
};

}  // namespace ui::external_app::foxhunt_rx

#endif /*__UI_FOXHUNT_RX_H__*/
