/*
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

#ifndef __EPIRB_TX_H__
#define __EPIRB_TX_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "portapack.hpp"
#include "message.hpp"
#include "tonesets.hpp"

#define BEACON_HEXA_SIZE        36
#define BEACON_HEXA_HALF_SIZE   18
#define BEACON_SIZE             18

namespace ui::external_app::epirb_tx {

enum class BeaconType
{
    EPIRB = 0,
    ELT = 1,
    PLB = 2
};


struct Location
{
    std::string locator;
    bool south;
    uint16_t lat_deg;
    uint8_t lat_min;
    uint8_t lat_sec;
    float latitude;
    bool west;
    uint16_t long_deg;
    uint8_t long_min;
    uint8_t long_sec;
    float longitude;
};

struct BeaconParams
{
    BeaconType type;
    bool is_test;
    bool is_internal;
    bool has_121_5;
    Location location;
};

class EPIRBTXAppView : public View {
   public:
    EPIRBTXAppView(NavigationView& nav);
    ~EPIRBTXAppView();

    void focus() override;

    std::string title() const override { return "EPIRB TX"; };

   private:
    void start_tx();
    void stop_tx();
    void update_config();
    void on_tx_progress(const uint32_t progress, const bool done);
    void on_timer();
    void load_beacons();
    void set_tx_button_state(bool active);
    std::string frame_to_hex_string(bool start);
    void generate_frame(BeaconParams params);
    void update_frame();

    struct Beacon {
        std::string title{};
        std::string description{};
        std::string frame{};
    };
    std::vector<Beacon> beacons{};
    Beacon default_beacon {"Self test","Serial User Location Protocol","FFFED0D6E6202820000C29FF51041775302D"};

    BeaconParams beacon_params { BeaconType::ELT , true, true, true, {"JN03RO",false,0,0,0,0,false,0,0,0,0}};

    size_t  selected_beacon{0};

    rf::Frequency am_frequency{121500000};
    rf::Frequency bpsk_frequency{406025000};

    TxRadioState radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        TONES_SAMPLERATE /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_epirb", app_settings::Mode::TX};

    bool loop{false};
    uint32_t last_frame_time{0};
    bool transmitting{false};
    bool mode_file{true};

    EPIRBTXDataMessage epirb_tx_message{};

    const size_t max_text_width = UI_POS_WIDTH_REMAINING(6)/UI_POS_DEFAULT_WIDTH;
    const size_t max_text_width_ext = UI_POS_WIDTH_REMAINING(0)/UI_POS_DEFAULT_WIDTH;

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "Source:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(5)}, "Frame:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(9)}, "Next frame in   s.", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(11)}, "AM frequency          MHz", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(17), UI_POS_Y(8)}, "s.", Theme::getInstance()->fg_light->foreground}};

    // For file mode
    Text text_beacon {
        { UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(7), UI_POS_DEFAULT_HEIGHT},
        "Beacon:"};        
    Text text_description_label {
        { UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(12), UI_POS_DEFAULT_HEIGHT},
        "Description:"};        

    // For manual mode
    Text text_beacon_type {
        { UI_POS_X(0), UI_POS_Y(1), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Type:"};        
    Text text_beacon_locator {
        { UI_POS_X(0), UI_POS_Y(2), UI_POS_WIDTH(8), UI_POS_DEFAULT_HEIGHT},
        "Locator:"};        
    OptionsField options_beacon_type{
        {UI_POS_X(9), UI_POS_Y(1)},
        30,
        {{"EPIRB", 0},
         {"ELT", 1},
         {"PLB", 2}}};
    TextField text_field_beacon_locator {
        { UI_POS_X(9), UI_POS_Y(2), UI_POS_WIDTH(10), UI_POS_DEFAULT_HEIGHT},
        "JN03RO"};        

    OptionsField options_mode{
        {UI_POS_X(7), UI_POS_Y(0)},
        30,
        {{"File (BEACONS.TXT)", 0},
         {"Manual (Editor)", 1}}};

    Text text_description {
        { UI_POS_X(0), UI_POS_Y(3), UI_POS_WIDTH_REMAINING(0), UI_POS_DEFAULT_HEIGHT},
        ""};        

    Text text_description_end {
        { UI_POS_X(0), UI_POS_Y(4), UI_POS_WIDTH_REMAINING(0), UI_POS_DEFAULT_HEIGHT},
        ""};        

    Text text_frame {
        { UI_POS_X(6), UI_POS_Y(5), UI_POS_WIDTH_REMAINING(6), UI_POS_DEFAULT_HEIGHT},
        ""};        
    Text text_frame_end {
        { UI_POS_X(6), UI_POS_Y(6), UI_POS_WIDTH_REMAINING(6), UI_POS_DEFAULT_HEIGHT},
        ""};        

    Text text_timeout {
        { UI_POS_X(14), UI_POS_Y(9), UI_POS_WIDTH(2), UI_POS_DEFAULT_HEIGHT},
        ""};        

    Checkbox checkbox_loop{
        {UI_POS_X(0), UI_POS_Y(8)},
        10,
        "Resend every",true};

    NumberField field_delay{
        {UI_POS_X(15), UI_POS_Y(8)},
        2,
        {1, 99},
        1,
        ' '};

    Checkbox checkbox_am{
        {UI_POS_X(0), UI_POS_Y(10)},
        10,
        "AM signal",true};

    FrequencyField field_am_frequency{
        {UI_POS_X(13), UI_POS_Y(11)}};

    OptionsField options_frame{
        {UI_POS_X(7), UI_POS_Y(1)},
        30,
        {}};

    Button button_tx{
        { UI_POS_X_RIGHT(9), UI_POS_Y(8), UI_POS_WIDTH(9), UI_POS_HEIGHT(2)},
        "START"
    };        
    const Style& style_tx_start = *Theme::getInstance()->fg_green;
    const Style& style_tx_stop = *Theme::getInstance()->fg_red;

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        10000,
        12};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

}  // namespace ui::external_app::epirb_tx

#endif /*__EPIRB_TX_H__*/
