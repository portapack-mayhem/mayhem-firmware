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
#include "horus_messages.hpp"
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

    Text text_status{
        {UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(20), UI_POS_HEIGHT(1)},
        "Searching"};

    Text text_count{
        {UI_POS_X_RIGHT(8), UI_POS_Y(1), UI_POS_WIDTH(8), UI_POS_HEIGHT(1)},
        ""};

    Text text_link{
        {UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)},
        ""};

    // Last packet, one field group per line.
    Text text_call{{UI_POS_X(0), UI_POS_Y(3) + 4, UI_POS_WIDTH_REMAINING(6), UI_POS_HEIGHT(1)}, ""};

    // Opens the last position on the map; shown once a packet with a fix arrives.
    Button button_map{
        {UI_POS_X_RIGHT(5), UI_POS_Y(3) + 2, UI_POS_WIDTH(5), UI_POS_HEIGHT(1) + 4},
        "Map"};
    Text text_time{{UI_POS_X(0), UI_POS_Y(4) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
    Text text_pos{{UI_POS_X(0), UI_POS_Y(5) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
    Text text_alt{{UI_POS_X(0), UI_POS_Y(6) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
    Text text_env{{UI_POS_X(0), UI_POS_Y(7) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
    Text text_env2{{UI_POS_X(0), UI_POS_Y(8) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
    Text text_extra1{{UI_POS_X(0), UI_POS_Y(9) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
    Text text_extra2{{UI_POS_X(0), UI_POS_Y(10) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};

#if HORUS_RX_DIAGNOSTICS
    // Decoder diagnostics: symbol rate, unique words seen, worst baseband load.
    Text text_debug{{UI_POS_X(0), UI_POS_Y(11) + 4, UI_POS_WIDTH_REMAINING(0), UI_POS_HEIGHT(1)}, ""};
#endif

    /* Packet history. The rect has to end at screen_height - 16 (the view sits below the
     * 16 px status bar), see the note in the FT8 RX app. */
    Console console{
        {UI_POS_X(0), UI_POS_Y(12) + 8, UI_POS_MAXWIDTH, UI_POS_HEIGHT_REMAINING(13) - 8}};

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
        HorusPacketMessage::message_id,
        [this](const Message* const p) {
            this->on_packet(static_cast<const HorusPacketMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_status{
        HorusRxStatusMessage::message_id,
        [this](const Message* const p) {
            this->on_status(static_cast<const HorusRxStatusMessage*>(p));
        }};
};

}  // namespace ui::external_app::horus_rx

#endif  // __UI_HORUS_RX_H__
