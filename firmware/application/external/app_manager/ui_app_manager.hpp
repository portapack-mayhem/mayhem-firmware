/*
 * copyleft 2024 zxkmm AKA zix aka sommermorgentraum
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

#ifndef __UI_APP_MANAGER_H__
#define __UI_APP_MANAGER_H__

#include "ui_navigation.hpp"

namespace ui::external_app::app_manager {

class AppManagerView : public View {
   public:
    AppManagerView(NavigationView& nav);
    std::string title() const override { return "AppMan"; };

   private:
    NavigationView& nav_;
    std::string autostart_app{""};
    SettingsStore nav_setting{
        "nav"sv,
        {{"autostart_app"sv, &autostart_app}}};

    void focus() override;
    void refresh_list();
    uint16_t app_list_index{0};

    Labels labels{
        {{UI_POS_X(0), UI_POS_Y(0)}, "App list:", Theme::getInstance()->fg_light->foreground}};

    MenuView menu_view{};

    Text text_app_info{
        {0, UI_POS_Y_BOTTOM(6), screen_width, UI_POS_HEIGHT(1)},
        "Highlight an app"};

    Button button_hide_unhide{
        {0, UI_POS_Y_BOTTOM(5), screen_width / 2 - 1, UI_POS_HEIGHT(2)},
        "Hide/Show"};

    Button button_clean_hide{
        {screen_width / 2 + 2, UI_POS_Y_BOTTOM(5), screen_width / 2 - 2, UI_POS_HEIGHT(2)},
        "Clean Hidden"};

    Button button_set_cancel_autostart{
        {0, screen_height - 32 - 16, screen_width / 2 - 1, UI_POS_HEIGHT(2)},
        "Set Autostart"};

    Button button_clean_autostart{
        {screen_width / 2 + 2, screen_height - 32 - 16, screen_width / 2 - 2, UI_POS_HEIGHT(2)},
        "Del Autostart"};

    std::string get_app_info(uint16_t index, bool is_display_name);
    void get_blacklist(std::vector<std::string>& blacklist);
    void write_blacklist(const std::vector<std::string>& blacklist);
    bool is_blacklisted(const std::string& app_display_name);
    void hide_app();
    void unhide_app();
    void hide_unhide_app();
    void clean_blacklist();
    bool is_autostart_app(const std::string& display_name);
    void set_auto_start();
    void unset_auto_start();
    void set_unset_autostart_app();
    bool is_autostart_app(const char* id_aka_app_call_name);
};

}  // namespace ui::external_app::app_manager

#endif  // __UI_APP_MANAGER_H__