/*
 * Copyright (C) 2024 HTotoo
 *
 * This file is part of PortaPack.
 *
 */

#ifndef __UI_flippertx_H__
#define __UI_flippertx_H__

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

namespace ui::external_app::flippertx {

#define OOK_SAMPLERATE 2280000U

class FlipperTxView : public View {
   public:
    FlipperTxView(NavigationView& nav);
    ~FlipperTxView();

    void focus() override;

    std::string title() const override {
        return "FlipperTx";
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
        "tx_flippertx", app_settings::Mode::TX};

    Button button_startstop{
        {0, 3 * 16, 96, 24},
        LanguageHelper::currentMessages[LANG_START]};

    bool is_running{false};

    void start();
    void stop();

    void on_tx_progress(const bool done);
    void generate_packet();

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.done);
        }};
};
};  // namespace ui::external_app::flippertx

#endif /*__UI_flippertx_H__*/
