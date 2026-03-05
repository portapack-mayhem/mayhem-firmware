#ifndef __UI_SAME_TX_H__
#define __UI_SAME_TX_H__

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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"
#include "app_settings.hpp"
#include "radio_state.hpp"
#include "message.hpp"
#include "modems.hpp"

namespace ui::external_app::same_tx {

class SameTxView : public View {
   public:
    SameTxView(NavigationView& nav);
    ~SameTxView();
    SameTxView(const SameTxView&) = delete;
    SameTxView& operator=(const SameTxView&) = delete;
    void focus() override;
    std::string title() const override { return "SAME TX"; }

   private:
    NavigationView& nav_;
    bool tx_active_{false};
    void start_tx();
    void stop_tx();
    void on_tx_progress(const uint32_t progress, const bool done);
    void update_msg_preview();

    TxRadioState radio_state_{162550000, 1750000, AFSK_TX_SAMPLERATE};
    app_settings::SettingsManager settings_{"tx_same", app_settings::Mode::TX};

    Labels labels{{
        {{0 * 8, 0 * 16}, "Org:", ui::Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 0 * 16}, "Evt:", ui::Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 1 * 16}, "FIPS:", ui::Theme::getInstance()->fg_light->foreground},
        {{16 * 8, 1 * 16}, "Dur:", ui::Theme::getInstance()->fg_light->foreground},
    }};
    OptionsField field_org{
        {4 * 8, 0 * 16},
        3,
        {{"WXR", 0}, {"EAS", 1}, {"CIV", 2}, {"PEP", 3}}};
    NumberField field_event_idx{{12 * 8, 0 * 16}, 2, {0, 25}, 1, '0', true};
    Text text_evt{{15 * 8, 0 * 16, 3 * 8, 16}, "RWT"};
    NumberField field_fips_state{{5 * 8, 1 * 16}, 2, {0, 99}, 1, '0'};
    NumberField field_fips_county{{7 * 8 + 4, 1 * 16}, 3, {0, 999}, 1, '0'};
    NumberField field_dur_h{{20 * 8, 1 * 16}, 2, {0, 99}, 1, '0'};
    NumberField field_dur_m{{22 * 8, 1 * 16}, 2, {0, 59}, 1, '0'};
    Text text_msg{{0, 2 * 16, 240, 16}, ""};
    Text text_status{{0, 3 * 16, 240, 16}, "Ready"};
    TransmitterView tx_view{(int16_t)UI_POS_Y_BOTTOM(4), 1000, 0};
    MessageHandlerRegistration message_handler_tx_progress{
        Message::ID::TXProgress,
        [this](const Message* const p) {
            const auto msg = *reinterpret_cast<const TXProgressMessage*>(p);
            this->on_tx_progress(msg.progress, msg.done);
        }};
};

}  // namespace ui::external_app::same_tx
#endif
