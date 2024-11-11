/*
 * Copyright (C) 2024 HTotoo
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

class OokBruteView : public View {
   public:
    OokBruteView(NavigationView& nav);
    ~OokBruteView();

    void focus() override;

    std::string title() const override {
        return "OokBrute";
    };

   private:
    NavigationView& nav_;
    TxRadioState radio_state_{
        433920000 /* frequency */,
        1750000 /* bandwidth */,
        OOK_SAMPLERATE /* sampling rate */
    };

    TxFrequencyField field_frequency{
        {0 * 8, 0 * 16},
        nav_};
    TransmitterView2 tx_view{
        {11 * 8, 0 * 16},
        /*short_ui*/ true};
    app_settings::SettingsManager settings_{
        "tx_ookbrute", app_settings::Mode::TX};

    Button button_startstop{
        {0, 3 * 16, 96, 24},
        LanguageHelper::currentMessages[LANG_START]};

    NumberField field_start{
        {0 * 8, 1 * 16},
        5,
        {0, 2500},
        1,
        ' ',
        true};

    NumberField field_stop{
        {9 * 8, 1 * 16},
        5,
        {0, 2500},
        1,
        ' ',
        true};

    OptionsField options_atkmode{
        {0 * 8, 2 * 16},
        10,
        {{"Came12", 0},
         {"Came24", 1},
         {"Nice12", 2},
         {"Nice24", 3},
         {"Holtek12", 4}}};

    bool is_running{false};

    uint32_t counter = 0;  // for packet change

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

#endif /*__UI_BLESPAM_H__*/
