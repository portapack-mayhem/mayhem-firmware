/*
 * copyleft Elliot Alderson from F society
 * copyleft Darlene Alderson from F society
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

#ifndef __UI_PLAYLIST_EDITOR_H__
#define __UI_PLAYLIST_EDITOR_H__

#include "ui_navigation.hpp"
namespace fs = std::filesystem;

namespace ui::external_app::playlist_editor {

class PlaylistEditorView : public View {
   public:
    PlaylistEditorView(NavigationView& nav);
    std::string title() const override { return "PPL Edit"; };

   private:
    NavigationView& nav_;

    void focus() override;

    std::vector<std::string> playlist = {};
    fs::path current_ppl_path = "";

    Labels labels{
        {{0 * 8, 0 * 16}, "Entries:", Theme::getInstance()->fg_light->foreground}};

    MenuView menu_view{};

    Text text_hint{
        {0, 27 * 8, screen_width, 16},
        "Highlight an app"};

    Text text_ppl_name{
    {0, 27 * 8, screen_width, 16},
    "Highlight an app"};

    Button button_open_playlist{
        {0, 29 * 8, screen_width / 2 - 1, 32},
        "Open PPL"};

    Button button_edit{
        {screen_width / 2 + 2, 29 * 8, screen_width / 2 - 2, 32},
        "Edit Item"};

    Button button_insert{
        {0, screen_height - 32 - 16, screen_width / 2 - 1, 32},
        "Ins. After"};

    Button button_save_playlist{
        {screen_width / 2 + 2, screen_height - 32 - 16, screen_width / 2 - 2, 32},
        "Save PPL"};

    void open_file();
    void on_file_changed(const fs::path& path);
    void on_edit_current_index();
    void refresh_interface();
    void on_edit_item();
    void on_insert_item();
    void refresh_menu_view();
    void save_ppl();
};

class PlaylistItemEditView : public View {
   public:
    std::function<void(std::string)> on_save{};
    std::function<void()> on_delete{};

    PlaylistItemEditView(NavigationView& nav, std::string item);

    std::string title() const override { return "Edit Item"; };
    void focus() override;

   private:
    NavigationView& nav_;
    std::string original_item_ = "";
    std::string path_ = "";
    uint32_t delay_{0}; 

    void refresh_ui();
    void parse_item(std::string item);
    std::string build_item() const;

    Labels labels{
        {{0 * 8, 1 * 16}, "Path:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 5 * 16}, "Delay(ms):", Theme::getInstance()->fg_light->foreground}
    };

    TextField field_path{
        {0 , 2 * 16, screen_width, 16},
        "empty"
    };

    NumberField field_delay{
        {11 * 8, 5 * 16},
        5,
        {0, 99999},
        1,
        ' '
    };

    Button button_browse{
        {2 * 8, 8 * 16, 8 * 8, 3 * 16},
        "Browse"
    };

    Button button_input_delay{
        {5 * 8, 8 * 16, 10 * 8, 3 * 16},
        "Input Delay"
    };

    Button button_delete{
        {1 , 16 * 16, screen_width/2-4, 2 * 16},
        "Delete"
    };

    Button button_save{
        {1 + screen_width / 2 + 1, 16 * 16, screen_width/2-4 , 2 * 16},
        "Save"
    };
};

}  // namespace ui::external_app::playlist_editor

#endif  // __UI_PLAYLIST_EDITOR_H__