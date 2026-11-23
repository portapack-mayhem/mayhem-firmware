/*
 * Copyright (C) 2026 OE1LBS
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

#ifndef __UI_HORUS_RX_H__
#define __UI_HORUS_RX_H__

#include "app_settings.hpp"
#include "message.hpp"
#include "radio_state.hpp"
#include "receiver_model.hpp"
#include "ui.hpp"
#include "ui_channel.hpp"
#include "ui_freq_field.hpp"
#include "ui_geomap.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_widget.hpp"

#include "horus_v3.hpp"

namespace ui::external_app::horus_rx {

class HorusRxView : public View {
   public:
    HorusRxView(NavigationView& nav);
    HorusRxView(const HorusRxView&) = delete;
    HorusRxView& operator=(const HorusRxView&) = delete;
    ~HorusRxView();

    void focus() override;
    std::string title() const override { return "Horus RX"; }

   private:
    /* Same front end as FT8 RX: the receiver runs wide and the baseband image narrows
     * the channel to +-5.5 kHz, inside which the demodulator finds the four tones. */
    static constexpr uint32_t sampling_rate = 3072000;
    static constexpr uint32_t baseband_bandwidth = 1750000;

    NavigationView& nav_;

    RxRadioState radio_state_{
        437600000 /* common 70 cm Horus Binary frequency */,
        baseband_bandwidth,
        sampling_rate};

    app_settings::SettingsManager settings_{
        "rx_horus",
        app_settings::Mode::RX};

    void on_packet(const HorusPacketMessage* message);
    void on_status(const HorusRxStatusMessage* message);
    void show_telemetry(const Telemetry& t, const HorusPacketMessage& m);

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    RFAmpField field_rf_amp{
        {UI_POS_X(11), UI_POS_Y(0)}};

    LNAGainField field_lna{
        {UI_POS_X(13), UI_POS_Y(0)}};

    VGAGainField field_vga{
        {UI_POS_X(16), UI_POS_Y(0)}};

    RSSI rssi{
        {UI_POS_X(19) - 4, UI_POS_Y(0), UI_POS_WIDTH(9) + 2, 4}};

    Channel channel{
        {UI_POS_X(19) - 4, UI_POS_Y(0) + 5, UI_POS_WIDTH(9) + 2, 4}};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    /* Laid out like the Radiosonde RX app (labels, values from column 9, GeoPos),
     * plus a short packet history at the bottom. */
    Labels labels{
        {{UI_POS_X(4), UI_POS_Y(2)}, "Type:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(6), UI_POS_Y(3)}, "ID:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(4), UI_POS_Y(4)}, "Time:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(3), UI_POS_Y(5)}, "Vbatt:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(3), UI_POS_Y(6)}, "Frame:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(4), UI_POS_Y(7)}, "Temp:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(8)}, "Humidity:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(9)}, "Pressure:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(10)}, "VSpeed:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(11)}, "Signal:", Theme::getInstance()->fg_light->foreground}};

    Text text_status{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(16), UI_POS_HEIGHT(1)},
        "Searching"};

    Text text_count{
        {UI_POS_X_RIGHT(13), UI_POS_Y(1), UI_POS_WIDTH(7), UI_POS_HEIGHT(1)},
        ""};

    // Opens the last position on the map; shown once a packet with a fix arrives.
    Button button_map{
        {UI_POS_X_RIGHT(5), UI_POS_Y(1), UI_POS_WIDTH(5), UI_POS_HEIGHT(1)},
        "Map"};

    Text text_type{{UI_POS_X(9), UI_POS_Y(2), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_id{{UI_POS_X(9), UI_POS_Y(3), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_time{{UI_POS_X(9), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_voltage{{UI_POS_X(9), UI_POS_Y(5), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_frame{{UI_POS_X(9), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_temp{{UI_POS_X(9), UI_POS_Y(7), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_humid{{UI_POS_X(9), UI_POS_Y(8), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_press{{UI_POS_X(9), UI_POS_Y(9), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_vspeed{{UI_POS_X(9), UI_POS_Y(10), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};
    Text text_link{{UI_POS_X(9), UI_POS_Y(11), UI_POS_WIDTH_REMAINING(9), UI_POS_HEIGHT(1)}, "..."};

    GeoPos geopos{
        {UI_POS_X(0), UI_POS_Y(12)},
        GeoPos::alt_unit::METERS,
        GeoPos::spd_unit::KMPH};

    // Up to two v3 custom sensor fields; empty for payloads that send none.
    Text text_extra{{UI_POS_X(0), UI_POS_Y(15), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)}, ""};

    /* Packet history, in the small font (see the constructor). The rect has to end at screen_height - 16 (the view sits below the
     * 16 px status bar), see the note in the FT8 RX app. */
    Console console{
        {UI_POS_X(0), UI_POS_Y(16), UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(17)}};

    uint32_t packets_total{0};
    uint32_t status_since_packet{0xFFFFFFFF};

    // last position, for the map; the map follows new packets while it is open
    GeoMapView* geomap_view{nullptr};
    std::string map_tag{};
    float map_lat{0.0f};
    float map_lon{0.0f};
    int32_t map_alt{0};
    int32_t map_speed{0};
    void open_map();

    MessageHandlerRegistration message_handler_packet{
        Message::ID::HorusPacket,
        [this](const Message* const p) {
            this->on_packet(static_cast<const HorusPacketMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_status{
        Message::ID::HorusRxStatus,
        [this](const Message* const p) {
            this->on_status(static_cast<const HorusRxStatusMessage*>(p));
        }};
};

}  // namespace ui::external_app::horus_rx

#endif  // __UI_HORUS_RX_H__
