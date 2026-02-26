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

namespace ui::external_app::opera_cake {

class OperaCakeView : public View {
   public:
    OperaCakeView(NavigationView& nav);
    std::string title() const override { return "OperaCake"; };
    void focus() override;

   private:
    NavigationView& nav_;

    // PCA9557 I2C address for the first Opera Cake board (address pins = 0)
    static constexpr uint8_t OPERACAKE_I2C_ADDRESS = 0x18;

    // Saved settings — declared before SettingsStore so they are populated on load
    uint8_t setting_port_a{0};  // 0=A1, 1=A2, 2=A3, 3=A4
    uint8_t setting_port_b{0};  // 0=B1, 1=B2, 2=B3, 3=B4

    SettingsStore settings_{
        "opera_cake"sv,
        {{"port_a"sv, &setting_port_a},
         {"port_b"sv, &setting_port_b}}};

    void detect_board();
    void apply_settings();

    Labels labels{
        {{0 * 8, 0 * 16}, "Opera Cake - Manual Mode", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 2 * 16}, "Board:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 4 * 16}, "Port A0:", Theme::getInstance()->fg_light->foreground},
        {{0 * 8, 5 * 16}, "Port B0:", Theme::getInstance()->fg_light->foreground}};

    Text text_status{
        {7 * 8, 2 * 16, 23 * 8, 16},
        "Scanning..."};

    // Port A0 connection selector: A1-A4
    OptionsField options_port_a{
        {10 * 8, 4 * 16},
        2,
        {{"A1", 0}, {"A2", 1}, {"A3", 2}, {"A4", 3}}};

    // Port B0 connection selector: B1-B4
    OptionsField options_port_b{
        {10 * 8, 5 * 16},
        2,
        {{"B1", 0}, {"B2", 1}, {"B3", 2}, {"B4", 3}}};

    Button button_apply{
        {4 * 8, 7 * 16, 22 * 8, 32},
        "Apply"};

    Button button_rescan{
        {4 * 8, 10 * 16, 22 * 8, 32},
        "Re-scan board"};

    Text text_result{
        {0 * 8, 12 * 16, 30 * 8, 16},
        ""};
};

}  // namespace ui::external_app::opera_cake

#endif  // __UI_OPERA_CAKE_H__
