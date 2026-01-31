/*
 * Copyright (C) 2026
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

#ifndef __UI_CWRADIO_H__
#define __UI_CWRADIO_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "message.hpp"
#include "transmitter_model.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"

namespace ui {

class CWRadioView : public View {
   public:
    CWRadioView(NavigationView& nav);
    ~CWRadioView();

    void focus() override;
    void on_show() override;

    std::string title() const override { return "CW Radio"; };

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};
    app_settings::SettingsManager settings_{
        "tx_cwradio", app_settings::Mode::TX};

    void start_tx();
    void stop_tx();
    void update_tx();
    void on_button_change(bool pressed);
    void on_encoder_change(int32_t delta);
    void on_tx_key_change(bool key_down);
    void update_button_state();

    bool transmitting_{false};
    bool key_is_down_{false};
    bool button_held_{false};
    uint8_t selected_modulation_{0};  // 0=AM, 1=FM, 2=DSB, 3=USB, 4=LSB

    static constexpr uint32_t default_frequency{7'040'000};  // 7.040 MHz (40m CW)
    static constexpr uint32_t default_tone{700};  // 700 Hz sidetone
    static constexpr uint32_t default_fm_delta{5000};  // FM deviation

    Labels labels{
        {{0 * 8, 0 * 8}, "CW Practice Transmitter", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 9 * 8}, "Mode:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 12 * 8}, "Tone:", Theme::getInstance()->fg_light->foreground},
        {{14 * 8, 12 * 8}, "Hz", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 14 * 8}, "Power:", Theme::getInstance()->fg_light->foreground}};

    FrequencyField field_frequency{
        {0 * 8, 2 * 8}};

    TransmitterView tx_view{
        {0 * 8, 4 * 8},
        /* frequency_step */ 50,  // 50 Hz steps for CW
        /* channel_bandwidth */ 150'000};  // 150 kHz

    OptionsField options_mode{
        {6 * 8, 9 * 8},
        6,
        {{"AM", 0},
         {"FM", 1},
         {"DSB", 2},
         {"USB", 3},
         {"LSB", 4}}};

    NumberField field_tone{
        {6 * 8, 12 * 8},
        4,
        {300, 1200},
        10,
        ' '};

    NumberField field_fm_delta{
        {0 * 8, 15 * 8},
        5,
        {1000, 25000},
        100,
        ' ',
        false};

    Text text_fm_delta_label{
        {6 * 8, 15 * 8, 8 * 8, 16},
        "FM Dev."};

    Button button_key{
        {UI_POS_X_CENTER(12), UI_POS_Y(20), UI_POS_WIDTH(12), UI_POS_HEIGHT(3)},
        "PRESS TO KEY"};

    Text text_status{
        {0 * 8, UI_POS_Y(24), UI_POS_MAXWIDTH, 16},
        "Ready"};

    Text text_instructions{
        {0 * 8, UI_POS_Y(26), UI_POS_MAXWIDTH, 32},
        "Hold button or use encoder\nto transmit CW"};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            (void)p;
            // Handle any progress updates if needed
        }};
};

} /* namespace ui */

#endif /*__UI_CWRADIO_H__*/
