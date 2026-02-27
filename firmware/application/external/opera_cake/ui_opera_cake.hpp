/*
 * copyleft spammingdramaqueen
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

#ifndef __UI_OPERA_CAKE_H__
#define __UI_OPERA_CAKE_H__

#include "ui_navigation.hpp"
#include "ui_widget.hpp"
#include "app_settings.hpp"
#include "opera_cake_app_boot.hpp"
#include "message.hpp"

namespace ui::external_app::opera_cake {

class OperaCakeView : public View {
   public:
    OperaCakeView(NavigationView& nav);
    std::string title() const override { return "OperaCake"; };
    void focus() override;

   private:
    NavigationView& nav_;

    // --- Persisted settings (declared before SettingsStore) ---
    uint8_t setting_mode{0};    // 0 = Manual, 1 = Frequency
    uint8_t setting_port_a{0};  // Manual: 0=A1 … 3=A4
    uint8_t setting_port_b{0};  // Manual: 0=B1 … 3=B4
    // Frequency ranges per port (MHz). A4 also acts as fallback.
    uint32_t setting_min_a1{1};
    uint32_t setting_max_a1{30};
    uint32_t setting_min_a2{30};
    uint32_t setting_max_a2{300};
    uint32_t setting_min_a3{300};
    uint32_t setting_max_a3{1000};
    uint32_t setting_min_a4{1000};
    uint32_t setting_max_a4{6000};

    SettingsStore settings_{
        "opera_cake"sv,
        {
            {"mode"sv, &setting_mode},
            {"port_a"sv, &setting_port_a},
            {"port_b"sv, &setting_port_b},
            {"min_a1"sv, &setting_min_a1},
            {"max_a1"sv, &setting_max_a1},
            {"min_a2"sv, &setting_min_a2},
            {"max_a2"sv, &setting_max_a2},
            {"min_a3"sv, &setting_min_a3},
            {"max_a3"sv, &setting_max_a3},
            {"min_a4"sv, &setting_min_a4},
            {"max_a4"sv, &setting_max_a4},
        }};

    bool board_detected_{false};

    // --- Private methods ---
    void detect_board();
    void build_ranges(::opera_cake::FreqRanges& ranges);
    void apply_manual();
    void apply_frequency();

    // --- Static labels ---
    // Screen pixel layout (8px/char, 16px/row):
    //  Row 0  y=  0  Board: <status>
    //  Row 1  y= 16  Mode: <Manual|Frequency>
    //  Row 2  y= 32  Port  From    To        (column headers)
    //  Row 3  y= 48  A1   [min]- [max] MHz
    //  Row 4  y= 64  A2   [min]- [max] MHz
    //  Row 5  y= 80  A3   [min]- [max] MHz
    //  Row 6  y= 96  A4   [min]- [max] MHz  (also freq-mode fallback)
    //  Row 7  y=112  A0: <portA>  B0: <portB>   (manual override)
    //  Row 9  y=144  [Apply]
    //  Row 10 y=160  [Re-scan board]
    //  Row 11 y=176  <result text>
    Labels labels{
        {{0 * 8, 0 * 16}, "Board:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 1 * 16}, "Mode:", Theme::getInstance()->fg_light->foreground},
        // column headers for frequency ranges
        {{0 * 8, 2 * 16}, "Port", Theme::getInstance()->fg_medium->foreground},
        {{3 * 8, 2 * 16}, "From", Theme::getInstance()->fg_medium->foreground},
        {{8 * 8, 2 * 16}, "To", Theme::getInstance()->fg_medium->foreground},
        // row A1
        {{0 * 8, 3 * 16}, "A1", Theme::getInstance()->fg_light->foreground},
        {{7 * 8, 3 * 16}, "-", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, 3 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
        // row A2
        {{0 * 8, 4 * 16}, "A2", Theme::getInstance()->fg_light->foreground},
        {{7 * 8, 4 * 16}, "-", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, 4 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
        // row A3
        {{0 * 8, 5 * 16}, "A3", Theme::getInstance()->fg_light->foreground},
        {{7 * 8, 5 * 16}, "-", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, 5 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
        // row A4
        {{0 * 8, 6 * 16}, "A4", Theme::getInstance()->fg_light->foreground},
        {{7 * 8, 6 * 16}, "-", Theme::getInstance()->fg_light->foreground},
        {{12 * 8, 6 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
        // manual port selectors row
        {{0 * 8, 7 * 16}, "A0:", Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 7 * 16}, "B0:", Theme::getInstance()->fg_light->foreground},
    };

    Text text_status{
        {7 * 8, 0 * 16, 23 * 8, 16},
        "Scanning..."};

    // Mode selector: Manual (0) or Frequency (1)
    OptionsField options_mode{
        {6 * 8, 1 * 16},
        9,
        {{"Manual   ", 0}, {"Frequency", 1}}};

    // Frequency range fields — min_ax at col 3, max_ax at col 8 (all in MHz)
    NumberField field_min_a1{{3 * 8, 3 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_max_a1{{8 * 8, 3 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_min_a2{{3 * 8, 4 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_max_a2{{8 * 8, 4 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_min_a3{{3 * 8, 5 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_max_a3{{8 * 8, 5 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_min_a4{{3 * 8, 6 * 16}, 4, {1, 9999}, 1, ' '};
    NumberField field_max_a4{{8 * 8, 6 * 16}, 4, {1, 9999}, 1, ' '};

    // Manual port selectors
    OptionsField options_port_a{
        {4 * 8, 7 * 16},
        2,
        {{"A1", 0}, {"A2", 1}, {"A3", 2}, {"A4", 3}}};

    OptionsField options_port_b{
        {12 * 8, 7 * 16},
        2,
        {{"B1", 0}, {"B2", 1}, {"B3", 2}, {"B4", 3}}};

    Button button_apply{
        {4 * 8, 9 * 16, 22 * 8, 32},
        "Apply"};

    Button button_rescan{
        {4 * 8, 12 * 16, 22 * 8, 32},
        "Re-scan board"};

    Text text_result{
        {0 * 8, 14 * 16, 30 * 8, 16},
        ""};
};

}  // namespace ui::external_app::opera_cake

#endif  // __UI_OPERA_CAKE_H__
