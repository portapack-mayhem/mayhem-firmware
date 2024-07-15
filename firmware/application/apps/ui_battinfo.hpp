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

#ifndef __UI_BATTINFO_H__
#define __UI_BATTINFO_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"

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
        {{2 * 8, 1 * 16}, "Percent:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 2 * 16}, "Voltage:", Theme::getInstance()->fg_light->foreground}};

    Labels labels_opt{
        {{2 * 8, 3 * 16}, "Current:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 4 * 16}, "Charge:", Theme::getInstance()->fg_light->foreground}};

    Text text_percent{
        {13 * 8, 1 * 16, 10 * 16, 16},
        "-"};
    Text text_voltage{
        {13 * 8, 2 * 16, 10 * 16, 16},
        "-"};
    Text text_current{
        {13 * 8, 3 * 16, 10 * 16, 16},
        "-"};
    Text text_charge{
        {13 * 8, 4 * 16, 10 * 16, 16},
        "-"};

    Button button_exit{
        {72, 17 * 16, 96, 32},
        "Back"};
    static msg_t static_fn(void* arg);
    static bool needRun;
    Thread* thread{nullptr};
};

} /* namespace ui */

#endif /*__UI_BATTINFO__*/
