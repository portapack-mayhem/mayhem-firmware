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
#include "ui_language.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "message.hpp"
#include "transmitter_model.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "portapack.hpp"

namespace ui::external_app::coasterp {

class CoasterPagerView : public View {
   public:
    CoasterPagerView(NavigationView& nav);
    ~CoasterPagerView();

    void focus() override;

    std::string title() const override { return "BurgerPgrTX"; };

   private:
    enum tx_modes {
        IDLE = 0,
        SINGLE,
        SCAN
    };

    tx_modes tx_mode = IDLE;

    TxRadioState radio_state_{
        433920000, /* frequency */
        1750000 /* bandwidth */,
        2280000 /* sampling rate */
    };
    app_settings::SettingsManager settings_{
        "tx_burger", app_settings::Mode::TX};

    void start_tx();
    void generate_frame();
    void on_tx_progress(const uint32_t progress, const bool done);

    Labels labels{
        {{1 * 8, 3 * 8}, "Syscall pager TX beta", Theme::getInstance()->fg_light->foreground},
        {{1 * 8, 8 * 8}, LanguageHelper::currentMessages[LANG_DATADP], Theme::getInstance()->fg_light->foreground}};

    SymField sym_data{
        {7 * 8, 8 * 8},
        16,
        SymField::Type::Hex};

    Checkbox checkbox_scan{
        {10 * 8, 14 * 8},
        4,
        LanguageHelper::currentMessages[LANG_SCAN]};

    /*
    ProgressBar progressbar {
        { 5 * 8, 12 * 16, 20 * 8, 16 }};
    */

    Text text_message{
        {5 * 8, 13 * 16, 20 * 8, 16},
        ""};

    TransmitterView tx_view{
        16 * 16,
        10000,
        12};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

} /* namespace ui::external_app::coasterp */
