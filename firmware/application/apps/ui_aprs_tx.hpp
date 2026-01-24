/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui.hpp"
#include "ui_textentry.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "message.hpp"
#include "modems.hpp"
#include "transmitter_model.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"

namespace ui {

class APRSTXView : public View {
   public:
    APRSTXView(NavigationView& nav);
    ~APRSTXView();

    void focus() override;

    std::string title() const override { return "APRS TX"; };

   private:
    TxRadioState radio_state_{
        144390000 /* frequency */,
        1750000 /* bandwidth */,
        AFSK_TX_SAMPLERATE /* sampling rate */
    };

    std::string symsrc = "";
    std::string symdst = "";
    std::string payload{""};
    std::string path_cache = "";
    int32_t ssidsrc = 0;
    int32_t ssiddst = 0;

    bool manual_gps_mode = false;
    float last_lat = 0.0f;
    float last_lon = 0.0f;

    app_settings::SettingsManager settings_{
        "tx_aprs",
        app_settings::Mode::TX,
        {{"symsrc"sv, &symsrc},
         {"symdst"sv, &symdst},
         {"ssidsrc"sv, &ssidsrc},
         {"ssiddst"sv, &ssiddst},
         {"payload"sv, &payload},
         {"last_lat"sv, &last_lat},
         {"last_lon"sv, &last_lon},
         {"path"sv, &path_cache}}};

    void start_tx();
    void generate_frame();
    void generate_frame_pos();
    void on_tx_progress(const uint32_t progress, const bool done);
    void on_gps(const GPSPosDataMessage* message);
    void process_coordinates(float lat, float lon);

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Source:       SSID:", Theme::getInstance()->fg_light->foreground},  // 6 alphanum + SSID
        {{UI_POS_X(0), UI_POS_Y(1)}, " Dest.:       SSID:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "Path: (ex.: WIDE1-1,WIDE2-1)", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(6)}, "Info field:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(10)}, "GPS:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_CENTER(22), UI_POS_Y(14)}, "Use ?GPS? in the info", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X_CENTER(17), UI_POS_Y(15)}, "as a placeholder.", Theme::getInstance()->fg_light->foreground},
    };

    Text text_path{
        {UI_POS_X(0), UI_POS_Y(3), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)},
        "WIDE1-1",
    };
    Text gps_is_manual{
        {UI_POS_X(6), UI_POS_Y(10), UI_POS_WIDTH(10), UI_POS_HEIGHT(1)},
        "",
    };

    SymField sym_source{
        {UI_POS_X(7), UI_POS_Y(0)},
        6,
        SymField::Type::Alpha};

    NumberField num_ssid_source{
        {UI_POS_X(19), UI_POS_Y(0)},
        2,
        {0, 15},
        1,
        ' '};

    SymField sym_dest{
        {UI_POS_X(7), UI_POS_Y(1)},
        6,
        SymField::Type::Alpha};

    NumberField num_ssid_dest{
        {UI_POS_X(19), UI_POS_Y(1)},
        2,
        {0, 15},
        1,
        ' '};

    Text text_payload{
        {UI_POS_X(0), UI_POS_Y(7), UI_POS_MAXWIDTH, UI_POS_HEIGHT(1)},
        ""};
    Button button_set{
        {UI_POS_X(0), UI_POS_Y(8), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)},
        "Set"};

    Text text_gps_coord{
        {UI_POS_X(0), UI_POS_Y(11), UI_POS_WIDTH(20), UI_POS_HEIGHT(1)},
        "-"};

    Button button_mangps{
        {UI_POS_X(0), UI_POS_Y(12), UI_POS_WIDTH(14), UI_POS_HEIGHT(2)},
        "Manual GPS"};
    Button button_setpath{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH(14), UI_POS_HEIGHT(2)},
        "Set Path"};
    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        5000,
        0  // disable setting bandwith, since APRS used fixed 10k bandwidth
    };

    MessageHandlerRegistration message_handler_gps{
        Message::ID::GPSPosData,
        [this](Message* const p) {
            const auto message = static_cast<const GPSPosDataMessage*>(p);
            this->on_gps(message);
        }};
    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

} /* namespace ui */
