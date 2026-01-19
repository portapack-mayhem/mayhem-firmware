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
        {UI_POS_X(13), UI_POS_Y(0)}};
    LNAGainField field_lna{
        {UI_POS_X(15), UI_POS_Y(0)}};
    VGAGainField field_vga{
        {UI_POS_X(18), UI_POS_Y(0)}};
    RSSI rssi{
        {UI_POS_X(21), UI_POS_Y(0), UI_POS_WIDTH(6), 4}};

    AudioVolumeField field_volume{
        {UI_POS_X_RIGHT(2), UI_POS_Y(0)}};

    RxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};

    // need to seperate because label shift need to hide independently
    Labels label_zoom{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Zoom: ", Theme::getInstance()->fg_light->foreground}};
    Labels label_shift{
        {{UI_POS_X(0), UI_POS_Y(2)}, "Shift: ", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_zoom{
        {UI_POS_X(7), UI_POS_Y(1)},
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

    NumberField number_shift{
        {UI_POS_X(7), UI_POS_Y(2)},
        5,
        {-MAXSIGNALBUFFER, MAXSIGNALBUFFER},
        1,
        ' '};

    Button button_reset{
        {UI_POS_X_RIGHT(12), UI_POS_Y(1), UI_POS_WIDTH(12), UI_POS_HEIGHT(1.5)},
        LanguageHelper::currentMessages[LANG_RESET]};

    Button button_pause{
        {UI_POS_X_RIGHT(12), UI_POS_Y(2.5), UI_POS_WIDTH(12), UI_POS_HEIGHT(1.5)},
        LanguageHelper::currentMessages[LANG_PAUSE]};

    Waveform waveform{
        {UI_POS_X(0), UI_POS_Y(4), UI_POS_MAXWIDTH, (UI_POS_HEIGHT_REMAINING(5) / 4) - 4},
        waveform_buffer,
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    Waveform waveform2{
        {UI_POS_X(0), UI_POS_Y(4) + (UI_POS_HEIGHT_REMAINING(5) / 4), UI_POS_MAXWIDTH, (UI_POS_HEIGHT_REMAINING(5) / 4) - 4},
        &waveform_buffer[MAXDRAWCNTPERWF],
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    Waveform waveform3{
        {UI_POS_X(0), UI_POS_Y(4) + 2 * (UI_POS_HEIGHT_REMAINING(5) / 4), UI_POS_MAXWIDTH, (UI_POS_HEIGHT_REMAINING(5) / 4) - 4},
        &waveform_buffer[MAXDRAWCNTPERWF * 2],
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    Waveform waveform4{
        {UI_POS_X(0), UI_POS_Y(4) + 3 * (UI_POS_HEIGHT_REMAINING(5) / 4), UI_POS_MAXWIDTH, (UI_POS_HEIGHT_REMAINING(5) / 4) - 4},
        &waveform_buffer[MAXDRAWCNTPERWF * 3],
        0,
        0,
        true,
        Theme::getInstance()->fg_yellow->foreground};

    bool needCntReset = false;
    bool paused = false;

    int16_t zoom = 1;  // one value in ms
    int16_t waveform_shift = 0;

    uint16_t cnt = 0;      // pointer to next element
    uint16_t drawcnt = 0;  // pointer to draw next element

    uint16_t timercnt = 0;  // screen refresh count
    uint16_t datacnt = 0;   // how many data i got. these are for track if there is no data, so need a cnt reset

    void add_time(int32_t time);
    void on_timer();
    void on_data(const ProtoViewDataMessage* message);
    void draw();
    void draw2();
    void set_pause(bool pause);
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

    MessageHandlerRegistration message_handler_freqchg{
        Message::ID::FreqChangeCommand,
        [this](Message* const p) {
            const auto message = static_cast<const FreqChangeCommandMessage*>(p);
            this->on_freqchg(message->freq);
        }};

    void on_freqchg(int64_t freq);
};

}  // namespace ui::external_app::protoview

#endif /*__UI_PROTOVIEW_H__*/
