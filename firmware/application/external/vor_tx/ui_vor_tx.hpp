/*
 * Copyright (C) 2026 PortaPack Mayhem
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
 * along with this program; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __UI_VOR_TX_H__
#define __UI_VOR_TX_H__

#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "portapack.hpp"
#include "radio_state.hpp"
#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_transmitter.hpp"

namespace ui::external_app::vor_tx {

class VorTxView : public View {
   public:
    VorTxView(NavigationView& nav);
    ~VorTxView();

    void focus() override;

    std::string title() const override { return "VOR TX"; }

   private:
    void start_tx();
    void stop_tx();
    void update_config();

    NavigationView& nav_;
    bool transmitting_{false};
    bool tx_acknowledged_{false};

    uint32_t radial_{0};
    bool ident_{true};
    std::string ident_text_{"VOR"};

    TxRadioState radio_state_{
        113'500'000 /* frequency */,
        1750000 /* bandwidth */,
        1536000 /* sampling rate */
    };

    app_settings::SettingsManager settings_{
        "tx_vor",
        app_settings::Mode::TX,
        {
            {"radial"sv, &radial_},
            {"ident"sv, &ident_},
            {"ident_text"sv, &ident_text_},
        }};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(1)}, "Radial:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(5)}, "Ident:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(8)}, "Status:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(10)}, "Lab use: set any freq &", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(0), UI_POS_Y(11)}, "TX into a dummy load.", Theme::getInstance()->fg_light->foreground}};

    NumberField field_radial{
        {UI_POS_X(8), UI_POS_Y(1)},
        3,
        {0, 359},
        1,
        '0'};

    Text text_radial_unit{
        {UI_POS_X(12), UI_POS_Y(1), UI_POS_WIDTH(4), UI_POS_HEIGHT(1)},
        "deg"};

    Checkbox check_ident{
        {UI_POS_X(0), UI_POS_Y(3)},
        13,
        "CW identifier",
        true};

    Button button_ident_text{
        {UI_POS_X(8), UI_POS_Y(5), UI_POS_WIDTH(10), UI_POS_HEIGHT(2)},
        "VOR"};

    Text text_status{
        {UI_POS_X(8), UI_POS_Y(8), UI_POS_WIDTH_REMAINING(8), UI_POS_HEIGHT(1)},
        "Idle"};

    TransmitterView tx_view{
        (int16_t)UI_POS_Y_BOTTOM(4),
        8333,
        1750000};
};

}  // namespace ui::external_app::vor_tx

#endif  // __UI_VOR_TX_H__
