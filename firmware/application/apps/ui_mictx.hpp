/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 Mark Thompson
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

// TODO: Consolidate Modulation/Bandwidth modes/settings with freqman/receiver_model.

#ifndef __UI_MICTX_H__
#define __UI_MICTX_H__

#include "app_settings.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "transmitter_model.hpp"
#include "message.hpp"
#include "receiver_model.hpp"
#include "ui_transmitter.hpp"

namespace ui {

class MicTXView : public View {
   public:
    MicTXView(NavigationView& nav);
    MicTXView(NavigationView& nav, ReceiverModel::settings_t override);
    ~MicTXView();

    MicTXView(const MicTXView&) = delete;
    MicTXView(MicTXView&&) = delete;
    MicTXView& operator=(const MicTXView&) = delete;
    MicTXView& operator=(MicTXView&&) = delete;

    void focus() override;

    // PTT: Enable through KeyEvent (only works with presses), disable by polling :(
    // This is the old "RIGHT BUTTON" method.
    /*
        bool on_key(const KeyEvent key) {
                if ((key == KeyEvent::Right) && (!va_enabled) && ptt_enabled) {
                        set_tx(true);
                        return true;
                } else
                        return false;
        };
        */

    std::string title() const override { return "Microphone"; };

   private:
    static constexpr uint32_t sampling_rate = 1536000U;
    static constexpr uint32_t lcd_frame_duration = (256 * 1000UL) / 60;  // 1 frame @ 60fps in ms .8 fixed point  /60

    void update_vumeter();
    bool tx_button_held();
    void do_timing();
    void set_tx(bool enable);
    void on_tx_progress(const bool done);
    void update_tx_icon();
    uint8_t shift_bits(void);
    void configure_baseband();
    void set_rxbw_options(void);
    void set_rxbw_defaults(bool use_app_settings);
    void update_receiver_rxbw(void);
    void rxaudio(bool is_on);

    RxRadioState rx_radio_state_{};
    TxRadioState tx_radio_state_{
        0 /* frequency */,
        1750000 /* bandwidth */,
        sampling_rate /* sampling rate */
    };

    enum Mic_Modulation : uint32_t {
        MIC_MOD_NFM = 0,
        MIC_MOD_WFM = 1,
        MIC_MOD_AM = 2,
        MIC_MOD_USB = 3,
        MIC_MOD_LSB = 4,
        MIC_MOD_DSB = 5,
    };

    // Settings
    uint32_t mic_mod_index{0};
    uint32_t rxbw_index{0};
    bool va_enabled{false};
    bool rogerbeep_enabled{false};
    bool mic_to_HP_enabled{false};
    bool bool_same_F_tx_rx_enabled{false};
    rf::Frequency rx_frequency{0};
    bool rx_enabled{false};
    uint32_t tone_key_index{0};
    uint32_t mic_gain_x10{1};
    uint8_t ak4951_alc_and_wm8731_boost_GUI{0};
    uint32_t va_level{40};
    uint32_t attack_ms{500};
    uint32_t decay_ms{1000};
    uint8_t iq_phase_calibration_value{15};
    app_settings::SettingsManager settings_{
        "tx_mic",
        app_settings::Mode::RX_TX,
        {
            {"mic_mod_index"sv, &mic_mod_index},
            {"rxbw_index"sv, &rxbw_index},
            {"same_F_tx_rx"sv, &bool_same_F_tx_rx_enabled},
            {"mic_rx_frequency"sv, &rx_frequency},
            {"rx_enabled"sv, &rx_enabled},
            {"mic_gain_x10"sv, &mic_gain_x10},
            {"mic_to_HP"sv, &mic_to_HP_enabled},
            {"alc_and_boost"sv, &ak4951_alc_and_wm8731_boost_GUI},
            {"va_level"sv, &va_level},
            {"attack_ms"sv, &attack_ms},
            {"decay_ms"sv, &decay_ms},
            {"vox"sv, &va_enabled},
            {"rogerbeep"sv, &rogerbeep_enabled},
            {"tone_key_index"sv, &tone_key_index},
            {"iq_phase_calibration"sv, &iq_phase_calibration_value},
        }};

    rf::Frequency tx_frequency{0};
    bool transmitting{false};
    uint32_t audio_level{0};
    uint32_t attack_timer{0};
    uint32_t decay_timer{0};
    int32_t tx_gain{47};
    bool rf_amp{false};
    int32_t focused_ui{2};
    bool button_touch{false};

    Labels labels_both{
        {{3 * 8, 1 * 8}, "MIC-GAIN:", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, 3 * 8}, "F:", Theme::getInstance()->fg_light->foreground},
        {{15 * 8, 3 * 8}, "FM TXBW:    kHz", Theme::getInstance()->fg_light->foreground},  // to be more symetric and consistent to the below FM RXBW
        {{18 * 8, (5 * 8)}, "Mode:", Theme::getInstance()->fg_light->foreground},          // now, no need to handle GAIN, Amp here It is handled by ui_transmitter.cpp
        {{4 * 8, 10 * 8}, "LVL:", Theme::getInstance()->fg_light->foreground},             // we delete  { {11 * 8, 5 * 8 }, "Amp:", Theme::getInstance()->fg_light->foreground },
        {{12 * 8, 10 * 8}, "ATT:", Theme::getInstance()->fg_light->foreground},
        {{20 * 8, 10 * 8}, "DEC:", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, (13 * 8) - 5}, "TONE KEY:", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, (18 * 8) - 1}, "======== Receiver ========", Theme::getInstance()->fg_green->foreground},
        {{5 * 8, (23 * 8) + 2}, "VOL:", Theme::getInstance()->fg_light->foreground},
        {{14 * 8, (23 * 8) + 2}, "RXBW:", Theme::getInstance()->fg_light->foreground},  // we remove the label "FM" because we will display all MOD types RX_BW.
        {{20 * 8, (25 * 8) + 2}, "SQ:", Theme::getInstance()->fg_light->foreground},
        {{5 * 8, (25 * 8) + 2}, "F_RX:", Theme::getInstance()->fg_light->foreground},
        {{5 * 8, (27 * 8) + 2}, "LNA:", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, (27 * 8) + 2}, "VGA:", Theme::getInstance()->fg_light->foreground},
        {{19 * 8, (27 * 8) + 2}, "AMP:", Theme::getInstance()->fg_light->foreground},
        {{21 * 8, (31 * 8)}, "TX-IQ-CAL:", Theme::getInstance()->fg_light->foreground}};
    Labels labels_WM8731{
        {{17 * 8, 1 * 8}, "Boost", Theme::getInstance()->fg_light->foreground}};
    Labels labels_AK4951{
        {{17 * 8, 1 * 8}, "ALC", Theme::getInstance()->fg_light->foreground}};

    VuMeter vumeter{
        {0 * 8, 1 * 8, 2 * 8, 33 * 8},
        12,
        true};

    OptionsField options_gain{
        {12 * 8, 1 * 8},
        4,
        {{"x0.5", 5},
         {"x1.0", 10},
         {"x1.5", 15},
         {"x2.0", 20}}};

    OptionsField options_ak4951_alc_mode{
        {20 * 8, 1 * 8},
        10,  // Label has 10 chars
        {
            {" OFF-12kHz", 0},   // Nothing changed from ORIGINAL, keeping ALL programmable AK4951 Digital Block->OFF, sampling 24Khz)
            {"+12dB-6kHz", 1},   // ALC-> on, (+12dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"+09dB-6kHz", 2},   // ALC-> on, (+09dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"+06dB-6kHz", 3},   // ALC-> on, (+06dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"+03dB-2kHz", 4},   // ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 3,5k + Pre-amp Mic (+21dB=original)+ EQ boosting ~<2kHz (f0~1k1, fb:1,7K, k=1,8)
            {"+03dB-4kHz", 5},   // ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 4kHz + Pre-amp Mic (+21dB=original)+ EQ boosting ~<3kHz (f0~1k4, fb~2,4k, k=1,8)
            {"+03dB-6kHz", 6},   // ALC-> on, (+03dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"+00dB-6kHz", 7},   // ALC-> on, (+00dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"-03dB-6kHz", 8},   // ALC-> on, (-03dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"-06dB-6kHz", 9},   // ALC-> on, (-06dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz + Pre-amp Mic (+21dB=original)
            {"-09dB-6kHz", 10},  // ALC-> on, (-09dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz - Pre-amp MIC -3dB (18dB's)
            {"-12dB-6kHz", 11},  // ALC-> on, (-12dB's) Auto Vol max + Wind Noise cancel + LPF 6kHz - Pre-amp MIC -6dB (15dB's)
        }};

    OptionsField options_wm8731_boost_mode{
        {22 * 8, 1 * 8},
        8,  // Label has 8 chars
        {
            {"ON +12dB", 0},  // WM8731 Mic Boost ON, original+12dBs condition, easy to saturate ADC sat in high voice, relative G = +12 dB's respect ref level
            {"ON +06dB", 1},  // WM8731 Mic Boost ON, original+6 dBs condition, easy to saturate ADC sat in high voice, relative G = +06 dB's respect ref level
            {"OFF+04dB", 2},  // WM8731 Mic Boost OFF to avoid ADC sat in high voice, relative G = +04 dB's (respect ref level), always effective sampling 24khz
            {"OFF-02dB", 3},  // WM8731 Mic Boost OFF to avoid ADC sat in high voice, relative G = -02 dB's (respect ref level)
            {"OFF-08dB", 4},  // WM8731 Mic Boost OFF to avoid ADC sat in high voice, relative G = -12 dB's (respect ref level)
        }};

    // TODO: Use TxFrequencyField
    FrequencyField field_frequency{
        {5 * 8, 3 * 8},
    };
    NumberField field_bw{
        {23 * 8, 3 * 8},
        3,
        {0, 150},
        1,
        ' '};

    TransmitterView2 tx_view{
        {3 * 8, 5 * 8},
        /*short_ui*/ false};

    OptionsField options_mode{
        {24 * 8, 5 * 8},
        6,
        {
            {"NFM/FM", MIC_MOD_NFM},
            {" WFM  ", MIC_MOD_WFM},
            {"  AM  ", MIC_MOD_AM},  // in fact that TX mode = AM -DSB with carrier.
            {" USB  ", MIC_MOD_USB},
            {" LSB  ", MIC_MOD_LSB},
            {"DSB-SC", MIC_MOD_DSB}  // We are TX Double Side AM Band with suppressed carrier, and allowing in RX both indep SSB lateral band (USB/LSB).
        }};

    Checkbox check_va{
        {3 * 8, 8 * 7},
        10,
        "VOX enable",
        false};

    NumberField field_va_level{
        {8 * 8, 10 * 8},
        3,
        {0, 255},
        2,
        ' '};
    NumberField field_va_attack{
        {16 * 8, 10 * 8},
        3,
        {0, 999},
        20,
        ' '};
    NumberField field_va_decay{
        {24 * 8, 10 * 8},
        4,
        {0, 9999},
        100,
        ' '};

    OptionsField options_tone_key{
        {12 * 8, (13 * 8) - 5},
        18,
        {}};

    Checkbox check_rogerbeep{
        {3 * 8, (14 * 8) + 4},
        10,
        "Roger beep",
        false};

    Checkbox check_mic_to_HP{
        {18 * 8, (14 * 8) + 4},
        10,
        "Hear Mic",
        false};

    Checkbox check_rxactive{
        {3 * 8, (21 * 8) - 7},
        8,  // it was 18, but if it is string size should be 8
        "RX audio",
        false};

    Checkbox check_common_freq_tx_rx{
        {18 * 8, (21 * 8) - 7},
        8,
        "F  RX=TX",
        false};

    AudioVolumeField field_volume{
        {9 * 8, (23 * 8) + 2}};

    OptionsField field_rxbw{
        {19 * 8, (23 * 8) + 2},
        7,
        {
            {" 8k5  ", 0},  // Initial dynamic values when we start Mic App.
            {" 11k  ", 1},
            {" 16k  ", 2},
        }};

    NumberField field_squelch{
        {23 * 8, (25 * 8) + 2},
        2,
        {0, 99},
        1,
        ' ',
    };

    // TODO: Use RxFrequencyField
    FrequencyField field_rxfrequency{
        {10 * 8, (25 * 8) + 2},
    };

    NumberField field_rxlna{
        {9 * 8, (27 * 8) + 2},
        2,
        {0, 40},
        8,
        ' ',
    };

    NumberField field_rxvga{
        {16 * 8, (27 * 8) + 2},
        2,
        {0, 62},
        2,
        ' ',
    };

    NumberField field_rxamp{
        {24 * 8, (27 * 8) + 2},
        1,
        {0, 1},
        1,
        ' ',
    };

    NumberField field_tx_iq_phase_cal{
        {24 * 8, (33 * 8)},
        2,
        {0, 63},  // 5 or 6 bits IQ CAL phase adjustment (range updated later)
        1,
        ' ',
    };

    Button tx_button{
        {10 * 8, 30 * 8, 10 * 8, 5 * 8},
        "PTT TX",
        true};

    Image tx_icon{
        {6 * 8, 31 * 8 + 4, 16, 16},
        &bitmap_icon_microphone,
        Theme::getInstance()->bg_darkest->background,
        Theme::getInstance()->bg_darkest->background};

    MessageHandlerRegistration message_handler_lcd_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->do_timing();
            this->update_vumeter();
        }};

    MessageHandlerRegistration message_handler_audio_level{
        Message::ID::AudioLevelReport,
        [this](const Message* const p) {
            const auto message = static_cast<const AudioLevelReportMessage*>(p);
            this->audio_level = message->value;
        }};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};
};

} /* namespace ui */

#endif /*__UI_MICTX_H__*/
