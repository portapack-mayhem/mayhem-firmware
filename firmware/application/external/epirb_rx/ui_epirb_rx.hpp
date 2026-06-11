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

// Specan is disable to keep application size below the 32k limit
// #define SPECAN

// Comment to disable timout reset on select and save approx 200 bytes of flash
#ifndef PRALINE
// Application does not fit on Praline with RESET_TIMER enabled
#define RESET_TIMER
#endif
// Comment to disable squelch control
#define SQUELCH
// Comment to disable beacon selection by encoder on detail tab
#define DETAIL_TAB_BEACON_SEL
// #define LOGGER

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

/**
 * Status of a packet
 */
enum class PacketStatus : uint8_t {
    Valid = 0,
    Corrected = 1,
    Error = 2
};

// Position of tabs in tab view
#define EPIRB_TAB_POS_Y (UI_POS_Y(4) + 3 * 8)
// Height of tabs in tab view
#define EPIRB_TAB_HEIGHT (screen_height - EPIRB_TAB_POS_Y - UI_POS_HEIGHT(1))

#ifdef LOGGER
class EPIRBLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void on_packet(Beacon& beacon);

   private:
    LogFile log_file{};
};
#endif

/**
 * Dedicated TextArea component used to optimize application code size
 */
class TextArea : public Widget {
   public:
    TextArea(Rect parent_rect);

#ifdef RESET_TIMER
    std::function<void(TextArea&)> on_select{};
    bool on_key(const KeyEvent key) override;
#endif

    void set_content(std::string_view value);
    void paint(Painter& painter) override;

   private:
    std::string content{};
};

// Forward declaration
class EPIRBAppView;

/**
 * View for beacon detail tab
 */
class EPIRBDetailView : public View {
   public:
    EPIRBDetailView(Rect parent_rect, EPIRBAppView& parent);
    void set_beacon(Beacon& beacon);
#ifdef DETAIL_TAB_BEACON_SEL
    bool on_encoder(EncoderEvent delta) override;
#endif

   private:
    TextArea text_beacon{{UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, EPIRB_TAB_HEIGHT}};
    EPIRBAppView& parent_app;
};

#define EPIRB_RX_DEFAULT_LATITUDE 43.604f
#define EPIRB_RX_DEFAULT_LONGITUDE 1.458f

/**
 * View for beacon map tab
 */
class EPIRBMapView : public View {
   public:
    EPIRBMapView(Rect parent_rect);
    void paint(Painter& painter) override;
    void on_show() override;
    void set_main_marker(const std::string& label, float lat, float lon);
    void clear_markers();
    void add_marker(GeoMarker& marker);
    void hide_map(bool hide);
    void repaint();

   private:
    GeoMap geomap{{0, 0, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGHT}};
    float lat_{EPIRB_RX_DEFAULT_LATITUDE};
    float lon_{EPIRB_RX_DEFAULT_LONGITUDE};
    bool map_hidden{true};
};

#define QR_WIDTH 126
#define QR_HEIGHT 127

/**
 * Vieaw for Beacon QR tab
 */
class EPRIBQRView : public View {
   public:
    EPRIBQRView(Rect parent_rect);
    EPRIBQRView(const EPRIBQRView&) = delete;
    EPRIBQRView& operator=(const EPRIBQRView&) = delete;

    void set_beacon(Beacon* beacon);
    void update_qr();
    void update_display();

   private:
    bool show_map{true};
    Beacon* current_beacon{nullptr};
    char qr_url[128];

    OptionsField options_qr{
        {UI_POS_X(5), UI_POS_Y(1)},
        6,
        {{"Map", 0},
         {"Detail", 1}}};

    QRCodeImage qr_code{
        {UI_POS_MAXWIDTH - QR_WIDTH - UI_POS_X(1), UI_POS_Y(1), QR_WIDTH, QR_HEIGHT}};

    TextArea text_data{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, EPIRB_TAB_HEIGHT - UI_POS_Y(1)}};
};

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

    void focus() override;
    void refresh();

    // Message to configure rx baseband
    EPIRBRXConfig epirb_rx_config_message{};
    void send_config();
    // Beacons database
    BeaconDB beacon_db{};
    // Update display when beacon selection changed0
    void on_beacon_change();

    std::string title() const override { return "EPIRB RX"; }

   private:
    uint8_t squelch{50};
    // The delay between each frame
    uint32_t countdown{50};
    app_settings::SettingsManager settings_{
        "rx_epirb",
        app_settings::Mode::RX,
        {
            {"epirb_squelch"sv, &squelch},
            {"countdown"sv, &countdown},
        }};

    ui::NavigationView& nav_;

#ifdef LOGGER
    std::unique_ptr<EPIRBLogger> logger{};
#endif

    OptionsField options_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        7,
        {}};

    ui::RFAmpField field_rf_amp{
        {UI_POS_X(8), UI_POS_Y(0)}};

    ui::LNAGainField field_lna{
        {UI_POS_X(10), UI_POS_Y(0)}};

    ui::VGAGainField field_vga{
        {UI_POS_X(13), UI_POS_Y(0)}};

    ui::RSSI rssi{
        {UI_POS_X(16), UI_POS_Y(0), UI_POS_WIDTH_REMAINING(22), 4}};

    ui::Channel channel{
        {UI_POS_X(16), UI_POS_Y(0) + 5, UI_POS_WIDTH_REMAINING(22), 4}};

    // ui::Audio audio{
    //     {UI_POS_X(16), UI_POS_Y(0) + 10, UI_POS_WIDTH_REMAINING(22), 4}};

    ui::AudioVolumeField field_volume{
        {UI_POS_WIDTH_REMAINING(2), UI_POS_Y(0)}};

#ifdef SQUELCH
    NumberField field_squelch{
        {UI_POS_WIDTH_REMAINING(5), UI_POS_Y(0)},
        2,
        {0, 99},
        1,
        ' '};
#endif

    // Status display
    TextArea text_status{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, UI_POS_HEIGHT(3)}};
    TextArea text_timeout{
        {UI_POS_X(13), UI_POS_Y(1), UI_POS_WIDTH(3), UI_POS_HEIGHT(1)}};
    SignalToken signal_token_tick_second{};
    // Timeout string
    int16_t timeout{0};

    // Tab View
    Rect view_rect = {0, EPIRB_TAB_POS_Y, UI_POS_MAXWIDTH, EPIRB_TAB_HEIGHT};

    BeaconUIList view_list{view_rect};
    EPIRBDetailView view_detail{view_rect, (*this)};
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
        {"QR", Theme::getInstance()->fg_orange->foreground, &view_qr}};

    uint16_t beacons_received = 0;
    uint16_t packets_valid = 0;
    uint16_t packets_corrected = 0;
    uint16_t packets_error = 0;

    MessageHandlerRegistration message_handler_packet{
        Message::ID::EPIRBPacket,
        [this](Message* const p) { on_packet(p); }};

    static void decode_packet(const baseband::Packet& packet, Beacon& beacon);
    void on_packet(Message* const p);
    void update_map();
    void on_tick_second();

    void update_display();
};

}  // namespace ui::external_app::epirb_rx

#endif  // __UI_EPIRB_RX_H__