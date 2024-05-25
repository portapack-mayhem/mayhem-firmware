/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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
#include "adsb.hpp"
#include "ui_textentry.hpp"
#include "ui_geomap.hpp"
#include "ui_tabview.hpp"
#include "ui_transmitter.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"

using namespace adsb;

namespace ui {

class Compass : public Widget {
   public:
    Compass(const Point parent_pos);

    void set_value(uint32_t new_value);

    void paint(Painter&) override;

   private:
    const range_t<uint32_t> range{0, 359};
    uint32_t value_{0};
};

class ADSBPositionView : public OptionTabView {
   public:
    ADSBPositionView(NavigationView& nav, Rect parent_rect);

    void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

   private:
    GeoPos geopos{
        {0, 2 * 16},
        GeoPos::FEET,
        GeoPos::HIDDEN};

    Button button_set_map{
        {8 * 8, 6 * 16, 14 * 8, 2 * 16},
        "Set from map"};
};

class ADSBCallsignView : public OptionTabView {
   public:
    ADSBCallsignView(NavigationView& nav, Rect parent_rect);

    void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

   private:
    std::string callsign = "TEST1234";

    Labels labels_callsign{
        {{2 * 8, 5 * 8}, "Callsign:", Theme::getInstance()->fg_light->foreground}};

    Button button_callsign{
        {12 * 8, 2 * 16, 10 * 8, 2 * 16},
        ""};
};

class ADSBSpeedView : public OptionTabView {
   public:
    ADSBSpeedView(Rect parent_rect);

    void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

   private:
    Labels labels_speed{
        {{1 * 8, 6 * 16}, "Speed:    kn  Bearing:    *", Theme::getInstance()->fg_light->foreground}};

    Labels labels_vert_rate{
        {{1 * 8, 8 * 16}, "Vert. rate:    ft/min, (+/-)", Theme::getInstance()->fg_light->foreground}};

    Compass compass{
        {21 * 8, 2 * 16}};

    NumberField field_angle{
        {21 * 8 + 20, 6 * 16},
        3,
        {0, 359},
        1,
        ' ',
        true};

    NumberField field_speed{
        {8 * 8, 6 * 16},
        3,
        {0, 999},
        5,
        ' '};

    NumberField field_vert_rate{
        {11 * 8, 8 * 16},
        5,
        {-4096, 4096},
        64,
        ' '  // Let's limit to +/-5k aprox , Ex. max safe descent vertical rate aprox -1000 ft/min on an instrument approach. , std step is 64
    };
};

class ADSBSquawkView : public OptionTabView {
   public:
    ADSBSquawkView(Rect parent_rect);

    void collect_frames(const uint32_t ICAO_address, std::vector<ADSBFrame>& frame_list);

   private:
    Labels labels_squawk{
        {{2 * 8, 2 * 16}, "Squawk:", Theme::getInstance()->fg_light->foreground}};

    SymField field_squawk{
        {10 * 8, 2 * 16},
        4,
        SymField::Type::Oct};
};

class ADSBTXThread {
   public:
    ADSBTXThread(std::vector<ADSBFrame> frames);
    ~ADSBTXThread();

    ADSBTXThread(const ADSBTXThread&) = delete;
    ADSBTXThread(ADSBTXThread&&) = delete;
    ADSBTXThread& operator=(const ADSBTXThread&) = delete;
    ADSBTXThread& operator=(ADSBTXThread&&) = delete;

   private:
    std::vector<ADSBFrame> frames_{};
    Thread* thread{nullptr};

    static msg_t static_fn(void* arg);

    void run();
};

class ADSBTxView : public View {
   public:
    ADSBTxView(NavigationView& nav);
    ~ADSBTxView();

    void focus() override;

    std::string title() const override { return "ADS-B TX"; };

   private:
    /*enum tx_modes {
                IDLE = 0,
                SINGLE,
                SEQUENCE
        };*/

    /*const float plane_lats[12] = {
                0,
                -1,
                -2,
                -3,
                -4,
                -5,
                -4.5,
                -5,
                -4,
                -3,
                -2,
                -1
        };
        const float plane_lons[12] = {
                0,
                1,
                1,
                1,
                2,
                1,
                0,
                -1,
                -2,
                -1,
                -1,
                -1
        };*/

    TxRadioState radio_state_{
        1090000000 /* frequency */,
        6000000 /* bandwidth */,
        4000000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_adsb", app_settings::Mode::TX};

    // tx_modes tx_mode = IDLE;
    NavigationView& nav_;
    std::vector<ADSBFrame> frames{};

    void start_tx();
    void generate_frames();

    Rect view_rect = {0, 7 * 8, 240, 192};

    ADSBPositionView view_position{nav_, view_rect};
    ADSBCallsignView view_callsign{nav_, view_rect};
    ADSBSpeedView view_speed{view_rect};
    ADSBSquawkView view_squawk{view_rect};

    TabView tab_view{
        {"Position", Theme::getInstance()->fg_cyan->foreground, &view_position},
        {"Callsign", Theme::getInstance()->fg_green->foreground, &view_callsign},
        {"Speed", Theme::getInstance()->fg_yellow->foreground, &view_speed},
        {"Squawk", Theme::getInstance()->fg_orange->foreground, &view_squawk}};

    Labels labels{
        {{2 * 8, 4 * 8}, "ICAO24:", Theme::getInstance()->fg_light->foreground}};

    SymField sym_icao{
        {10 * 8, 4 * 8},
        6,
        SymField::Type::Hex};

    Text text_frame{
        {1 * 8, 29 * 8, 14 * 8, 16},
        "-"};

    TransmitterView tx_view{
        16 * 16,
        1000000,
        0};

    std::unique_ptr<ADSBTXThread> tx_thread{};
};

} /* namespace ui */
