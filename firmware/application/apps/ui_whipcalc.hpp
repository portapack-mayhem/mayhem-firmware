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

#ifndef __UI_WHIPCALC_H__
#define __UI_WHIPCALC_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_freq_field.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include <vector>

namespace ui {
class WhipCalcView : public View {
   public:
    WhipCalcView(NavigationView& nav);
    void focus() override;
    std::string title() const override { return "Ant. length"; };

   private:
    const double speed_of_light_mps = 299792458.0;      // m/s
    const double speed_of_light_fps = 983571087.90472;  // feet/s
    const std::string frac_str[4] = {"", " 1/4", " 1/2", " 3/4"};

    struct antenna_entry {
        std::string label{};
        std::vector<uint16_t> elements{};
    };

    NavigationView& nav_;
    std::vector<antenna_entry> antenna_db{};
    void update_result();
    void load_antenna_db();
    void add_default_antenna();

    Labels labels{
        {{2 * 8, 1 * 16}, "Frequency:", Theme::getInstance()->fg_light->foreground},
        {{7 * 8, 2 * 16}, "Wave:", Theme::getInstance()->fg_light->foreground},
        {{5 * 8, 3 * 16}, "Metric:", Theme::getInstance()->fg_light->foreground},
        {{3 * 8, 4 * 16}, "Imperial:", Theme::getInstance()->fg_light->foreground}};

    TxFrequencyField field_frequency{
        {13 * 8, 1 * 16},
        nav_};

    OptionsField options_type{
        {13 * 8, 2 * 16},
        7,
        {{"Full", 8},
         {"Half", 4},
         {"Quarter", 2},
         {"3/4", 6},
         {"1/8", 1},
         {"3/8", 3},
         {"5/8", 5},
         {"7/8", 7}}};

    Text text_result_metric{
        {13 * 8, 3 * 16, 10 * 16, 16},
        "-"};
    Text text_result_imperial{
        {13 * 8, 4 * 16, 10 * 16, 16},
        "-"};
    Console console{
        {0, 6 * 16, 240, 160}};

    Button button_exit{
        {72, 17 * 16, 96, 32},
        "Back"};
};

} /* namespace ui */

#endif /*__UI_WHIPCALC__*/
