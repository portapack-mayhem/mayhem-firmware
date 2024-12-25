#ifndef __UI_APP_MANAGER_H__
#define __UI_APP_MANAGER_H__

#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "audio.hpp"
#include "ch.h"

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
        {{0 * 8, 0 * 16}, "App list:", Theme::getInstance()->fg_light->foreground}};

    MenuView menu_view{};

    Text text_app_info{
        {0, 27 * 8, screen_width, 16},
        "Highlight an app"};

    Button button_hide_unhide{
        {0, 29 * 8, screen_width / 2 - 1, 32},
        "Hide/Show"};

    Button button_clean_hide{
        {screen_width / 2 + 2, 29 * 8, screen_width / 2 - 2, 32},
        "Clean Hidden"};

    Button button_set_cancel_autostart{
        {0, screen_height - 32 - 16, screen_width / 2 - 1, 32},
        "Set Autostart"};

    Button button_clean_autostart{
        {screen_width / 2 + 2, screen_height - 32 - 16, screen_width / 2 - 2, 32},
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
    bool is_autostart_app(const char* id_aka_friendly_name);
};

}  // namespace ui::external_app::app_manager

#endif  // __UI_APP_MANAGER_H__