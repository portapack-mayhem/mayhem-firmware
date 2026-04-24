/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __UI_BATTINFO_H__
#define __UI_BATTINFO_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include "i2cdevmanager.hpp"

namespace ui {
class BattinfoView : public View {
   public:
    ~BattinfoView();
    BattinfoView(NavigationView& nav);
    BattinfoView(const BattinfoView&) = delete;
    BattinfoView(BattinfoView&&) = delete;
    BattinfoView& operator=(const BattinfoView&) = delete;
    BattinfoView& operator=(BattinfoView&&) = delete;

    void focus() override;
    std::string title() const override { return "Battery"; };

   private:
    void update_result();
    void on_timer();
    NavigationView& nav_;
    uint16_t timer_period = 60;
    uint16_t timer_counter = 0;
    uint8_t percent = 0;
    uint16_t voltage = 0;
    int32_t current = 0;

    Labels labels{
        {{UI_POS_X(2), UI_POS_Y(1)}, "Percent:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(2)}, "Voltage:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(3)}, "Method:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(4)}, "Capacity:", Theme::getInstance()->fg_light->foreground},
    };

    Labels labels_opt{
        {{UI_POS_X(2), UI_POS_Y(5)}, "Current:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(6)}, "Charge:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(7)}, "TTF/E:", Theme::getInstance()->fg_light->foreground},
        // {{UI_POS_X(2), UI_POS_Y(8)}, "Cycles:", Theme::getInstance()->fg_light->foreground},
        {{UI_POS_X(2), UI_POS_Y(10)}, "Change settings:", Theme::getInstance()->fg_light->foreground},
    };

    Text text_percent{
        {UI_POS_X(13), UI_POS_Y(1), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};
    Text text_voltage{
        {UI_POS_X(13), UI_POS_Y(2), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};
    Text text_method{
        {UI_POS_X(13), UI_POS_Y(3), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};
    Text text_capacity{
        {UI_POS_X(13), UI_POS_Y(4), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};
    Text text_current{
        {UI_POS_X(13), UI_POS_Y(5), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};
    Text text_charge{
        {UI_POS_X(13), UI_POS_Y(6), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};
    Text text_ttef{
        {UI_POS_X(13), UI_POS_Y(7), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};

    /* Text text_cycles{
        {UI_POS_X(13), UI_POS_Y(8), UI_POS_WIDTH(11), UI_POS_HEIGHT(1)},
        "-"};

    Text text_warn{
        {UI_POS_X(1), UI_POS_Y(9), screen_width, UI_POS_HEIGHT(2)},
        ""}; */

    Button button_settings{
        {UI_POS_X(2), UI_POS_Y(11) + 5, UI_POS_WIDTH(10), UI_POS_HEIGHT(2)},
        "Settings"};

    Button button_exit{
        {UI_POS_X_CENTER(12), UI_POS_Y_BOTTOM(4), UI_POS_WIDTH(12), UI_POS_HEIGHT(2)},
        "Back"};
    static msg_t static_fn(void* arg);
    Thread* thread{nullptr};
};

} /* namespace ui */

#endif /*__UI_BATTINFO__*/
