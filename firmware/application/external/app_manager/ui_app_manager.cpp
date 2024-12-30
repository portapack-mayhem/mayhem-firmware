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

#include "ui_app_manager.hpp"
#include "ui_navigation.hpp"
#include "ui_external_items_menu_loader.hpp"

#include "file.hpp"
namespace fs = std::filesystem;

#include "string_format.hpp"

#include "file_reader.hpp"

using namespace portapack;

namespace ui::external_app::app_manager {

/* AppManagerView **************************************/
/* | inner apps | external apps |
 * ------------------------------
 * | id         | appCallName   |
 * | displayName|appFriendlyName|
 */
AppManagerView::AppManagerView(NavigationView& nav)
    : nav_{nav} {
    add_children({&labels,
                  &menu_view,
                  &text_app_info,
                  &button_hide_unhide,
                  &button_clean_hide,
                  &button_set_cancel_autostart,
                  &button_clean_autostart});

    menu_view.set_parent_rect({0, 2 * 8, screen_width, 24 * 8});

    menu_view.on_highlight = [this]() {
        if (menu_view.highlighted_index() >= app_list_index) {
            text_app_info.set("");
            return;
        }

        std::string info;
        auto app_name = get_app_info(menu_view.highlighted_index(), true);
        auto app_id = get_app_info(menu_view.highlighted_index(), false);

        info += "Hidden:";

        if (is_blacklisted(app_name)) {
            info += "Yes ";
        } else {
            info += "No ";
        }

        info += "Autostart:";
        if (is_autostart_app(app_id)) {
            info += "Yes";
        } else {
            info += "No";
        }

        if (info.empty()) {
            info = "Highlight an app";
        }

        text_app_info.set(info);
    };

    button_hide_unhide.on_select = [this](Button&) {
        hide_unhide_app();
        refresh_list();
    };

    button_clean_hide.on_select = [this](Button&) {
        clean_blacklist();
        refresh_list();
    };

    button_set_cancel_autostart.on_select = [this](Button&) {
        set_unset_autostart_app();
        refresh_list();
    };

    button_clean_autostart.on_select = [this](Button&) {
        unset_auto_start();
        refresh_list();
    };

    refresh_list();
}

void AppManagerView::refresh_list() {
    auto previous_cursor_index = menu_view.highlighted_index();
    app_list_index = 0;
    menu_view.clear();

    size_t index = 0;

    auto padding = [](const std::string& str) {
        return str.length() < 25 ? std::string(25 - str.length(), ' ') + '' : "";
        // that weird char is a icon in portapack's font base that looks like a switch, which i use to indicate the auto start
    };

    for (auto& app : NavigationView::appList) {
        if (app.id == nullptr) continue;

        app_list_index++;

        menu_view.add_item({app.displayName + std::string(is_autostart_app(app.id) ? padding(app.displayName) : ""),
                            app.iconColor,
                            app.icon,
                            [this, app_id = std::string(app.id)](KeyEvent) {
                                button_hide_unhide.focus();
                            }});
    }

    ExternalItemsMenuLoader::load_all_external_items_callback([this, &index, &padding](ui::AppInfoConsole& app) {
        if (app.appCallName == nullptr) return;

        app_list_index++;

        menu_view.add_item({app.appFriendlyName + std::string(is_autostart_app(app.appCallName) ? padding(app.appFriendlyName) : ""),
                            ui::Color::light_grey(),
                            &bitmap_icon_sdcard,
                            [this, app_id = std::string(app.appCallName)](KeyEvent) {
                                button_hide_unhide.focus();
                            }});
    });

    menu_view.set_highlighted(previous_cursor_index);
}

void AppManagerView::focus() {
    menu_view.focus();
}

void AppManagerView::hide_app() {
    std::vector<std::string> blacklist;
    get_blacklist(blacklist);

    blacklist.push_back(get_app_info(menu_view.highlighted_index(), true));
    write_blacklist(blacklist);
}

void AppManagerView::unhide_app() {
    std::vector<std::string> blacklist;
    get_blacklist(blacklist);
    blacklist.erase(std::remove(blacklist.begin(), blacklist.end(), get_app_info(menu_view.highlighted_index(), true)), blacklist.end());
    write_blacklist(blacklist);
}

void AppManagerView::hide_unhide_app() {
    if (is_blacklisted(get_app_info(menu_view.highlighted_index(), true)))
        unhide_app();
    else
        hide_app();
}

void AppManagerView::get_blacklist(std::vector<std::string>& blacklist) {
    File f;

    auto error = f.open(u"SETTINGS/blacklist");
    if (error)
        return;

    auto reader = FileLineReader(f);
    for (const auto& line : reader) {
        if (line.length() == 0 || line[0] == '#')
            continue;

        blacklist.push_back(trim(line));
    }
}

void AppManagerView::write_blacklist(const std::vector<std::string>& blacklist) {
    File f;
    auto error = f.create(u"SETTINGS/blacklist");
    if (error)
        return;

    for (const auto& app_name : blacklist) {
        auto line = app_name + "\n";
        f.write(line.c_str(), line.length());
    }
}

void AppManagerView::clean_blacklist() {
    std::vector<std::string> blacklist = {};
    write_blacklist(blacklist);
}

bool AppManagerView::is_blacklisted(const std::string& app_name) {
    std::vector<std::string> blacklist;
    get_blacklist(blacklist);
    return std::find(blacklist.begin(), blacklist.end(), app_name) != blacklist.end();
}

void AppManagerView::set_auto_start() {
    auto app_index = menu_view.highlighted_index();
    if (app_index >= app_list_index) return;

    auto id_aka_app_call_name = get_app_info(app_index, false);

    autostart_app = id_aka_app_call_name;

    refresh_list();
}

void AppManagerView::unset_auto_start() {
    autostart_app = "";
    refresh_list();
}

void AppManagerView::set_unset_autostart_app() {
    auto app_index = menu_view.highlighted_index();
    if (app_index >= app_list_index) return;

    auto id_aka_friendly_name = get_app_info(app_index, false);

    if (is_autostart_app(id_aka_friendly_name))
        unset_auto_start();
    else
        set_auto_start();
}

bool AppManagerView::is_autostart_app(const char* id_aka_friendly_name) {
    return autostart_app == std::string(id_aka_friendly_name);
}

bool AppManagerView::is_autostart_app(const std::string& id_aka_friendly_name) {
    return autostart_app == id_aka_friendly_name;
}

std::string AppManagerView::get_app_info(uint16_t index, bool is_display_name) {
    size_t current_index = 0;
    std::string result;

    for (auto& app : NavigationView::appList) {
        if (app.id == nullptr) continue;
        if (current_index == index) {
            return is_display_name ? std::string(app.displayName) : std::string(app.id);
        }
        current_index++;
    }

    ExternalItemsMenuLoader::load_all_external_items_callback([&current_index, &index, &result, &is_display_name](ui::AppInfoConsole& app) {
        if (app.appCallName == nullptr) return;
        if (current_index == index) {
            result = is_display_name ? app.appFriendlyName : app.appCallName;
        }
        current_index++;
    });

    return result;
}

}  // namespace ui::external_app::app_manager