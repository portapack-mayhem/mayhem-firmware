/*
 * Copyright (C) 2025 Sarah Rose
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

#ifndef __UI_P25_TX
#define __UI_P25_TX

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_transmitter.hpp"
#include "ui_freq_field.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"

namespace ui::external_app::p25_tx {

class P25TxView : public View {
   public:
    explicit P25TxView(NavigationView& nav);
    ~P25TxView();

    void focus() override;
    std::string title() const override { return "P25 TX"; }

   private:
    void start_tx();
    void stop_tx();
    void on_tx_progress(const uint32_t progress, const bool done);
    NavigationView& nav_;

    TxRadioState radio_state_{
        155'000'000,
        12500,
        2'400'000};

    app_settings::SettingsManager settings_{
        "tx_p25_tx",
        app_settings::Mode::TX};

    Labels labels_{{
        {{UI_POS_X(0), UI_POS_Y(0)}, "NAC:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(0)}, "SYS:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(1)}, "WACN:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(2)}, "RFSS:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(2)}, "SITE:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(3)}, "TG:", ui::Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(9), UI_POS_Y(3)}, "VCH:", ui::Theme::getInstance()->fg_light->foreground},
    }};

    SymField field_nac{{UI_POS_X(4), UI_POS_Y(0)}, 3, SymField::Type::Hex};
    SymField field_sysid{{UI_POS_X(13), UI_POS_Y(0)}, 3, SymField::Type::Hex};
    SymField field_wacn{{UI_POS_X(5), UI_POS_Y(1)}, 5, SymField::Type::Hex};
    SymField field_rfssid{{UI_POS_X(5), UI_POS_Y(2)}, 2, SymField::Type::Hex};
    SymField field_siteid{{UI_POS_X(14), UI_POS_Y(2)}, 2, SymField::Type::Hex};
    SymField field_tg{{UI_POS_X(3), UI_POS_Y(3)}, 4, SymField::Type::Hex};
    SymField field_vch{{UI_POS_X(13), UI_POS_Y(3)}, 3, SymField::Type::Hex};

    Text text_status{{0, UI_POS_Y(4), UI_POS_MAXWIDTH, UI_POS_DEFAULT_HEIGHT}, "Ready"};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(5),
        12500,
        0};

    bool transmitting{false};
    int tsbk_idx_{0};

    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(message.progress, message.done);
        }};
};

}  // namespace ui::external_app::p25_tx

#endif /* __UI_P25_TX */
