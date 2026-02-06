/*
 * Copyright (C) 2024 HTotoo
 * copyleft 2024 zxkmm AKA zix aka sommermorgentraum
 *
 * This file is part of PortaPack.
 *
 */

#ifndef __UI_OOKBRUTE_H__
#define __UI_OOKBRUTE_H__

#include "ui.hpp"
#include "ui_language.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "ui_record_view.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "utility.hpp"

using namespace ui;

namespace ui::external_app::ookbrute {

#define OOK_SAMPLERATE 2280000U

class OOKBruteView : public View {
   public:
    OOKBruteView(NavigationView& nav);
    ~OOKBruteView();

    void focus() override;

    std::string title() const override {
        return "OOKBrute";
    };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{
        433920000 /* frequency */,
        1750000 /* bandwidth */,
        OOK_SAMPLERATE /* sampling rate */
    };

    TxFrequencyField field_frequency{
        {UI_POS_X(0), UI_POS_Y(0)},
        nav_};
    TransmitterView2 tx_view{
        {11 * 8, UI_POS_Y(0)},
        /*short_ui*/ true};
    app_settings::SettingsManager settings_{
        "tx_ookbrute", app_settings::Mode::TX};

    Labels labels{
        {{UI_POS_X(0), 2 * 16}, "Start Position:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 7 * 16}, "Stop Position:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), 13 * 16}, "Encoder Type:", Theme::getInstance()->fg_light->foreground}};

    Button button_startstop{
        {8, screen_height - 48 - 16, screen_width - 2 * 8, 48},
        LanguageHelper::currentMessages[LANG_START]};

    Button button_input_start_position{
        {8, 4 * 16, screen_width - 2 * 8, 24},
        "Input Start Pos"};

    Button button_input_stop_position{
        {8, 9 * 16, screen_width - 2 * 8, 24},
        "Input Stop Pos"};

    NumberField field_start{
        {UI_POS_X(0), 3 * 16},
        8,
        {0, 2500},
        1,
        ' ',
        true};

    NumberField field_stop{
        {0, 8 * 16},
        9,
        {0, 2500},
        1,
        ' ',
        true};

    // NB: when add new encoder here you should also change the char count limit for input range buttons by it's digits in DEC
    OptionsField options_atkmode{
        {0, 14 * 16},
        12,
        {{"Came12", 0},
         {"Came24", 1},
         {"Nice12", 2},
         {"Nice24", 3},
         {"Holtek12", 4},
         {"Princeton24", 5}}};

    bool is_running{false};

    uint32_t counter = 0;  // for packet change

    std::string text_input_buffer{};  // this is needed by the text_prompt func

    void start();
    void stop();

    void on_tx_progress(const bool done);
    void validate_start_stop();
    void update_start_stop(uint32_t proto);
    void generate_packet();

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};
};
};  // namespace ui::external_app::ookbrute

#endif /*__UI_OOKBRUTE_H__*/
