/*
 * Copyright (C) 2024 EPIRB Decoder Implementation
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

#ifndef __UI_EPIRB_RX_H__
#define __UI_EPIRB_RX_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_geomap.hpp"
#include "ui_spectrum.hpp"
#include "ui_tabview.hpp"

#include "event_m0.hpp"
#include "message.hpp"
#include "log_file.hpp"

#include "baseband_packet.hpp"

#include "audio.hpp"

#include "beacon.hpp"
#include "beacon_db.hpp"
#include "ui_beaconlist.hpp"

namespace ui::external_app::epirb_rx {

enum class PacketStatus : uint8_t {
    Valid = 0,
    Corrected = 1,
    Error = 2
};

#define EPIRB_TAB_POS_Y (UI_POS_Y(4) + 3 * 8)
#define EPIRB_TAB_HEIGTH (screen_height - EPIRB_TAB_POS_Y)

/*class EPIRBLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(Beacon& beacon);

   private:
    LogFile log_file{};
};*/
/*
class EPIRBListView : public View {
   public:
    EPIRBListView(Rect parent_rect);
    void set_db(BeaconDB& db);
    void refresh();

   private:
    // Beacon list
    BeaconUIList beaconlist_view{
        {0, 0, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGTH}};
*/
/*ui::Button button_map{
    {0, 180, 60, 24},
    "Map"};

ui::Button button_clear{
    {64, 180, 60, 24},
    "Clear"};

ui::Button button_log{
    {128, 180, 60, 24},
    "Log"};*/
/*};*/

class EPIRBDetailView : public View {
   public:
    EPIRBDetailView(Rect parent_rect);

   private:
    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Source:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Frame:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(10)}, "Next frame in   s.", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(12)}, "AM frequency:         MHz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(14)}, "AM   chan.:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(15)}, "BPSK chan.:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(17), UI_POS_Y(9)}, "s.", Theme::getInstance()->fg_light->foreground}};
};

#define EPIRB_RX_DEFAULT_LATITUDE 43.604f
#define EPIRB_RX_DEFAULT_LONGITUDE 1.458f

class EPIRBMapView : public View {
   public:
    EPIRBMapView(Rect parent_rect);
    void on_show() override;
    void clear_main_marker();
    void set_main_marker(const std::string& label, float lat, float lon);
    void clear_markers();
    void add_marker(GeoMarker& marker);

   private:
    const std::string NO_BEACON{"No beacon"};
    GeoMap geomap{{0, 0, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGTH}};
    float lat_{EPIRB_RX_DEFAULT_LATITUDE};
    float lon_{EPIRB_RX_DEFAULT_LONGITUDE};
};

// Forward declaration
class EPIRBAppView;

class EPIRBRxView : public View {
   public:
    EPIRBRxView(EPIRBAppView& parent, Rect parent_rect);
    void on_show() override;
    void on_hide() override;

   private:
    spectrum::WaterfallView waterfall{};
    EPIRBAppView& app_view;
};

/*
class EPIRBBeaconDetailView : public ui::View {
   public:
    std::function<void(void)> on_close{};

    EPIRBBeaconDetailView(ui::NavigationView& nav);
    EPIRBBeaconDetailView(const EPIRBBeaconDetailView&) = delete;
    EPIRBBeaconDetailView& operator=(const EPIRBBeaconDetailView&) = delete;

    void set_beacon(const Beacon& beacon);
    const Beacon& beacon() const { return beacon_; }

    void focus() override;
    void paint(ui::Painter&) override;

    ui::GeoMapView* get_geomap_view() { return geomap_view; }

   private:
    Beacon beacon_{};

    ui::Button button_done{
        {125, 224, 96, 24},
        "Done"};
    ui::Button button_see_map{
        {19, 224, 96, 24},
        "See on map"};

    ui::GeoMapView* geomap_view{nullptr};

    ui::Rect draw_field(
        ui::Painter& painter,
        const ui::Rect& draw_rect,
        const ui::Style& style,
        const std::string& label,
        const std::string& value);
};
*/
class EPIRBAppView final : public ui::View {
   public:
    EPIRBAppView(ui::NavigationView& nav);
    ~EPIRBAppView();

    void set_parent_rect(const ui::Rect new_parent_rect) override;
    void paint(ui::Painter&) override;
    void focus() override;

    // Message to configure rx baseband
    EPIRBRXConfig epirb_rx_config_message{};
    void send_config();

    std::string title() const override { return "EPIRB RX"; }

   private:
    app_settings::SettingsManager settings_{
        "rx_epirb", app_settings::Mode::RX};

    ui::NavigationView& nav_;

    BeaconDB beacon_db{};

    // EPIRBLogger logger{};

    static constexpr auto header_height = 4 * 16;

    ui::Text label_frequency{
        {UI_POS_X(0), UI_POS_Y(0), 4 * 8, 1 * 16},
        "Freq"};

    ui::OptionsField options_frequency{
        {UI_POS_X(5), UI_POS_Y(0)},
        7,
        {
            {"406.028", 406028000},
            {"406.025", 406025000},
            {"406.037", 406037000},
            {"433.025", 433025000},
            {"144.875", 144875000},
        }};

    ui::RFAmpField field_rf_amp{
        {UI_POS_X(13), UI_POS_Y(0)}};

    ui::LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};

    ui::VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};

    ui::RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(24), 4}};

    ui::Channel channel{
        {UI_POS_X(21), UI_POS_Y(0) + 5, UI_POS_WIDTH_REMAINING(24), 4}};

    ui::AudioVolumeField field_volume{
        {UI_POS_WIDTH_REMAINING(2), UI_POS_Y(0)}};

    // Status display
    ui::Text label_status{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(15), UI_POS_HEIGHT(1)},
        "Listening..."};

    ui::Text label_beacons_count{
        {UI_POS_X(16), UI_POS_Y(1), UI_POS_WIDTH(14), UI_POS_HEIGHT(1)},
        "Beacons: 0"};

    ui::Text label_packet_stats{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(29), UI_POS_HEIGHT(1)},
        ""};

    // Latest beacon info display
    ui::Text label_latest{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH(8), UI_POS_HEIGHT(1)},
        "Latest:"};

    ui::Text text_latest_info{
        {UI_POS_X(8), UI_POS_Y(3),
         UI_POS_WIDTH_REMAINING(8 - 4 /*Make width larger than actual screen space as a workaround to https://github.com/portapack-mayhem/mayhem-firmware/issues/3144 */), UI_POS_HEIGHT(1)},
        ""};

    // Tab View
    Rect view_rect = {0, EPIRB_TAB_POS_Y, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGTH};

    // EPIRBListView view_list{view_rect};
    BeaconUIList view_list{view_rect};
    EPIRBDetailView view_detail{view_rect};
    EPIRBMapView view_map{view_rect};
    EPIRBRxView view_rx{*this, view_rect};

    TabView tab_view{
        {"List", Theme::getInstance()->fg_cyan->foreground, &view_list},
        {"Detail", Theme::getInstance()->fg_green->foreground, &view_detail},
        {"Map", Theme::getInstance()->fg_yellow->foreground, &view_map},
        {"RX", Theme::getInstance()->fg_orange->foreground, &view_rx}};

    SignalToken signal_token_tick_second{};
    uint32_t beacons_received = 0;
    uint32_t packets_valid = 0;
    uint32_t packets_corrected = 0;
    uint32_t packets_error = 0;

    MessageHandlerRegistration message_handler_packet{
        Message::ID::EPIRBPacket,
        [this](Message* const p) {
            const auto message = static_cast<const EPIRBPacketMessage*>(p);
            this->on_packet(message->packet);
        }};

    static void decode_packet(const baseband::Packet& packet, Beacon& beacon);
    void on_packet(const baseband::Packet& packet);
    void update_map();
    void on_clear_beacons();
    void on_toggle_log();
    void on_tick_second();

    void update_display();
    // std::string beacon_to_hex_string(const baseband::Packet& packet);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __UI_EPIRB_RX_H__