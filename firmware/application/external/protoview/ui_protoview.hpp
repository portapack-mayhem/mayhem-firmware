/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_PROTOVIEW_H__
#define __UI_PROTOVIEW_H__

#define MAXSIGNALBUFFER 400
#define MAXDRAWCNT 600
#define MAXDRAWCNTPERWF 150

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "log_file.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::protoview {

class ProtoView : public View {
   public:
    ProtoView(NavigationView& nav);
    ~ProtoView();

    void focus() override;

    std::string title() const override { return "ProtoView"; };

   private:
    int16_t waveform_buffer[MAXDRAWCNT];
    int32_t time_buffer[MAXSIGNALBUFFER];

    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "rx_protoview", app_settings::Mode::RX};

    RFAmpField field_rf_amp{
        {13 * 8, 0 * 16}};
    LNAGainField field_lna{
        {15 * 8, 0 * 16}};
    VGAGainField field_vga{
        {18 * 8, 0 * 16}};
    RSSI rssi{
        {21 * 8, 0, 6 * 8, 4}};

    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};
    Labels labels{
        {{0 * 8, 1 * 16}, "Zoom: ", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_zoom{
        {7 * 8, 1 * 16},
        4,
        {{"1", 1},
         {"2", 2},
         {"5", 5},
         {"15", 15},
         {"30", 30},
         {"50", 50},
         {"100", 100},
         {"200", 200},
         {"500", 500},
         {"1000", 1000}}};

    Button button_reset{
        {screen_width - 12 * 8, 1 * 16, 96, 24},
        LanguageHelper::currentMessages[LANG_RESET]};

    Waveform waveform{
        {0, 5 * 8, 240, 50},
        waveform_buffer,
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    Waveform waveform2{
        {0, 5 * 8 + 55, 240, 50},
        &waveform_buffer[MAXDRAWCNTPERWF],
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    Waveform waveform3{
        {0, 5 * 8 + 110, 240, 50},
        &waveform_buffer[MAXDRAWCNTPERWF * 2],
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    Waveform waveform4{
        {0, 5 * 8 + 165, 240, 50},
        &waveform_buffer[MAXDRAWCNTPERWF * 3],
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    bool needCntReset = false;

    int16_t zoom = 1;  // one value in ms

    uint16_t cnt = 0;      // pointer to next element
    uint16_t drawcnt = 0;  // pointer to draw next element

    uint16_t timercnt = 0;  // screen refresh count
    uint16_t datacnt = 0;   // how many data i got. these are for track if there is no data, so need a cnt reset

    void add_time(int32_t time);
    void on_timer();
    void on_data(const ProtoViewDataMessage* message);
    void draw();
    void draw2();
    void reset();

    MessageHandlerRegistration message_handler_packet{
        Message::ID::ProtoViewData,
        [this](Message* const p) {
            const auto message = static_cast<const ProtoViewDataMessage*>(p);
            this->on_data(message);
        }};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_timer();
        }};
};

}  // namespace ui::external_app::protoview

#endif /*__UI_PROTOVIEW_H__*/
