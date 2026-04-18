/*
 * Copyright (C) 2024 EPIRB Decoder Implementation
 * Copyright (C) 2026 Frederic BORRY - ADRASEC 31
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

#ifdef SPECAN
#include "ui_spectrum.hpp"
#endif

#include "ui_tabview.hpp"

#include "ui_qrcode.hpp"

#include "event_m0.hpp"
#include "message.hpp"
#include "log_file.hpp"

#include "baseband_packet.hpp"

#include "audio.hpp"

#include "beacon.hpp"
#include "beacon_db.hpp"
#include "ui_beaconlist.hpp"
#include "resources.hpp"

namespace ui::external_app::epirb_rx {

enum class PacketStatus : uint8_t {
    Valid = 0,
    Corrected = 1,
    Error = 2
};

#define EPIRB_TAB_POS_Y (UI_POS_Y(4) + 3 * 8)
#define EPIRB_TAB_HEIGTH (screen_height - EPIRB_TAB_POS_Y - UI_POS_HEIGHT(1))

/*class EPIRBLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(Beacon& beacon);

   private:
    LogFile log_file{};
};*/

class TextArea : public Widget {
   public:
    TextArea(Rect parent_rect);

    void set_content(std::string_view value);
    void paint(Painter& painter) override;

   private:
    std::string content{};
};


class EPIRBDetailView : public View {
   public:
    EPIRBDetailView(Rect parent_rect, ResourceManager& rm);
    void set_beacon(Beacon& beacon);

   private:
    ResourceManager& resource_manager;
    TextArea text_beacon{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, EPIRB_TAB_HEIGTH}};
    /*ui::Text text_beacon{
        {UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH + 10, EPIRB_TAB_HEIGTH},
        ""};*/
    /*Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Beacon:", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(0), UI_POS_Y(1)}, "Protocol:", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(0), UI_POS_Y(4)}, "Location:", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(0), UI_POS_Y(7)}, "Control:", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(0), UI_POS_Y(8)}, "Hex ID:", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(0), UI_POS_Y(9)}, "S/N:", Theme::getInstance()->fg_cyan->foreground},
        {{UI_POS_X(0), UI_POS_Y(10)}, "Data:", Theme::getInstance()->fg_cyan->foreground}};

    ui::Text text_title{
        {UI_POS_X(0), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10 - 4), UI_POS_HEIGHT(1)},
        ""};
    ui::Text text_protocol_title{
        {UI_POS_X(1), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10), UI_POS_HEIGHT(1)},
        ""};
    ui::Text text_protocol_desc{
        {UI_POS_X(2), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(2)},
        ""};
    ui::Text text_location_locator{
        {UI_POS_X(4), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10), UI_POS_HEIGHT(1)},
        ""};
    ui::Text text_location_desc{
        {UI_POS_X(5), UI_POS_Y(0), UI_POS_MAXWIDTH, UI_POS_HEIGHT(2)},
        ""};
    ui::Text text_control{
        {UI_POS_X(7), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10-8), UI_POS_HEIGHT(1)},
        ""};
    ui::Text text_hex_id{
        {UI_POS_X(8), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10), UI_POS_HEIGHT(1)},
        ""};
    ui::Text text_serial{
        {UI_POS_X(9), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10), UI_POS_HEIGHT(1)},
        ""};
    ui::Text text_data{
        {UI_POS_X(10), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(10), UI_POS_HEIGHT(1)},
        ""};*/
};

#define EPIRB_RX_DEFAULT_LATITUDE 43.604f
#define EPIRB_RX_DEFAULT_LONGITUDE 1.458f

class EPIRBMapView : public View {
   public:
    EPIRBMapView(Rect parent_rect);
    void paint(Painter& painter) override;
    void on_show() override;
    void clear_main_marker();
    void set_main_marker(const std::string& label, float lat, float lon);
    void clear_markers();
    void add_marker(GeoMarker& marker);
    void repaint();

   private:
    const std::string NO_BEACON{"No beacon"};
    GeoMap geomap{{0, 0, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGTH}};
    float lat_{EPIRB_RX_DEFAULT_LATITUDE};
    float lon_{EPIRB_RX_DEFAULT_LONGITUDE};
};

class EPRIBQRView : public View {
   public:
    EPRIBQRView(Rect parent_rect);

    void set_url(const char* url);

   private:
    QRCodeImage qr_code{
        {UI_POS_X_CENTER(16), 63, 100, 100}};

};

// Forward declaration
class EPIRBAppView;

#ifdef SPECAN
class EPIRBRxView : public spectrum::WaterfallView {
   public:
    EPIRBRxView(EPIRBAppView& parent, Rect parent_rect);
    void on_show() override;
    void on_hide() override;

   private:
    EPIRBAppView& app_view;
};
#endif

class EPIRBAppView final : public ui::View {
   public:
    EPIRBAppView(ui::NavigationView& nav);
    ~EPIRBAppView();

    void set_parent_rect(const ui::Rect new_parent_rect) override;
    void focus() override;
    void refresh();

    // Message to configure rx baseband
    EPIRBRXConfig epirb_rx_config_message{};
    void send_config();

    std::string title() const override { return "EPIRB RX"; }

   private:
    app_settings::SettingsManager settings_{
        "rx_epirb", app_settings::Mode::RX};

    ui::NavigationView& nav_;

    BeaconDB beacon_db{};

    ResourceManager resource_manager{};

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
    TextArea text_status{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(3)}};
    ui::Text text_timeout{
        {UI_POS_X(13), UI_POS_Y(1), UI_POS_WIDTH(2), UI_POS_HEIGHT(1)},
        ""};
    SignalToken signal_token_tick_second{};
    // Timeout string
    int16_t timeout{0};
    // The delay between each frame
    uint16_t timeout_delay{50};

    // Tab View
    Rect view_rect = {0, EPIRB_TAB_POS_Y, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGTH};

    BeaconUIList view_list{view_rect};
    EPIRBDetailView view_detail{view_rect,resource_manager};
    EPIRBMapView view_map{view_rect};
#ifdef SPECAN
    EPIRBRxView view_rx{*this, view_rect};
#endif

    EPRIBQRView view_qr{view_rect};

    TabView tab_view{
        {"List", Theme::getInstance()->fg_cyan->foreground, &view_list},
        {"Detail", Theme::getInstance()->fg_green->foreground, &view_detail},
        {"Map", Theme::getInstance()->fg_yellow->foreground, &view_map},
#ifdef SPECAN
        {"RX", Theme::getInstance()->fg_orange->foreground, &view_rx},
#endif
        {"QR", Theme::getInstance()->fg_orange->foreground, &view_qr}
    };

    uint16_t beacons_received = 0;
    uint16_t packets_valid = 0;
    uint16_t packets_corrected = 0;
    uint16_t packets_error = 0;

    char qr_url[128];

    MessageHandlerRegistration message_handler_packet{
        Message::ID::EPIRBPacket,
        [this](Message* const p) { on_packet(p); }};

    static void decode_packet(const baseband::Packet& packet, Beacon& beacon);
    void on_packet(Message* const p);
    void update_map();
    void on_clear_beacons();
    void on_toggle_log();
    void on_tick_second();


    void update_display();
    // std::string beacon_to_hex_string(const baseband::Packet& packet);
};

}  // namespace ui::external_app::epirb_rx

#endif  // __UI_EPIRB_RX_H__