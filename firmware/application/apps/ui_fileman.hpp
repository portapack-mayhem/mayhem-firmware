/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2017 Furrtek
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

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "file.hpp"
#include "ui_navigation.hpp"
#include "ui_textentry.hpp"

namespace ui {

struct fileman_entry {
    std::string path{};
    uint32_t size{};
    bool is_directory{};
};

enum class EmptyReason : uint8_t {
    NotEmpty,
    NoFiles,
    NoSDC
};

enum class ClipboardMode : uint8_t {
    None,
    Cut,
    Copy
};

class FileManBaseView : public View {
   public:
    FileManBaseView(
        NavigationView& nav,
        std::string filter);

    virtual ~FileManBaseView() {}

    void focus() override;
    std::string title() const override { return "Fileman"; };
    void push_dir(const std::filesystem::path& path);

   protected:
    uint32_t prev_highlight = 0;
    uint8_t pagination = 0;
    uint8_t nb_pages = 1;
    static constexpr size_t max_filename_length = 20;
    static constexpr size_t max_items_loaded = 75;  // too memory hungry, so won't sort it
    static constexpr size_t items_per_page = 20;

    struct file_assoc_t {
        std::filesystem::path extension;
        const Bitmap* icon;
        ui::Color color;
    };

    const std::vector<file_assoc_t> file_types = {
        {u".TXT", &bitmap_icon_file_text, ui::Color::white()},
        {u".PNG", &bitmap_icon_file_image, ui::Color::green()},
        {u".BMP", &bitmap_icon_file_image, ui::Color::green()},
        {u".C8", &bitmap_icon_file_iq, ui::Color::dark_cyan()},
        {u".C16", &bitmap_icon_file_iq, ui::Color::dark_cyan()},
        {u".WAV", &bitmap_icon_file_wav, ui::Color::dark_magenta()},
        {u".PPL", &bitmap_icon_file_iq, ui::Color::white()},                  // Playlist/Replay
        {u".REM", &bitmap_icon_remote, ui::Color::orange()},                  // Remote
        {u"", &bitmap_icon_file, Theme::getInstance()->fg_light->foreground}  // NB: Must be last.
    };

    std::filesystem::path get_selected_full_path() const;
    const fileman_entry& get_selected_entry() const;

    int file_count_filtered(const std::filesystem::path& directory);
    void pop_dir();
    void refresh_list();
    void reload_current(bool reset_pagination = false);
    void load_directory_contents(const std::filesystem::path& dir_path);
    void load_directory_contents_unordered(const std::filesystem::path& dir_path, size_t file_cnt);
    const file_assoc_t& get_assoc(const std::filesystem::path& ext) const;

    NavigationView& nav_;

    EmptyReason empty_{EmptyReason::NotEmpty};
    std::function<void(KeyEvent)> on_select_entry{nullptr};
    std::function<void(bool)> on_refresh_widgets{nullptr};

    const std::string str_back{"<--"};
    const std::string str_next{"-->"};
    const std::string str_full{"Can't load more.."};
    const std::filesystem::path parent_dir_path{u".."};
    std::filesystem::path current_path{u""};
    std::filesystem::path extension_filter{u""};

    std::list<fileman_entry> entry_list{};
    std::vector<uint32_t> saved_index_stack{};

    bool show_hidden_files{false};

    Labels labels{
        {{0, 0}, "Path:", Theme::getInstance()->fg_light->foreground}};

    Text text_current{
        {6 * 8, 0 * 8, 24 * 8, 16},
        "",
    };

    MenuView menu_view{
        {0, 2 * 8, 240, 26 * 8},
        true};

    Button button_exit{
        {22 * 8, 34 * 8, 8 * 8, 32},
        "Exit"};
};

class FileLoadView : public FileManBaseView {
   public:
    std::function<void(std::filesystem::path)> on_changed{};

    FileLoadView(NavigationView& nav, std::string filter);
    virtual ~FileLoadView() {}

   private:
    void refresh_widgets(const bool v);
};

/*
// It would be nice to be able to launch FileLoadView
// but it will OOM if launched from within FileManager.
class FileSaveView : public View {
public:
        FileSaveView(
                NavigationView& nav,
                const std::filesystem::path& path,
                const std::filesystem::path& file);

        std::function<void(std::filesystem::path)> on_save { };

private:
        static constexpr size_t max_filename_length = 64;

        void refresh_widgets();

        NavigationView& nav_;
        std::filesystem::path path_;
        std::filesystem::path file_;
        std::string buffer_ { };

        Labels labels {
                { { 0 * 8, 1 * 16 }, "Path:", Theme::getInstance()->fg_light->foreground },
                { { 0 * 8, 6 * 16 }, "Filename:",Theme::getInstance()->fg_light->foreground },
        };

        Text text_path {
                { 0 * 8, 2 * 16, 30 * 8, 16 },
                "",
        };

        Button button_edit_path {
                { 18 * 8, 3 * 16, 11 * 8, 32 },
                "Edit Path"
        };

        Text text_name {
                { 0 * 8, 7 * 16, 30 * 8, 16 },
                "",
        };

        Button button_edit_name {
                { 18 * 8, 8 * 16, 11 * 8, 32 },
                "Edit Name"
        };

        Button button_save {
                { 10 * 8, 16 * 16, 9 * 8, 32 },
                "Save"
        };

        Button button_cancel {
                { 20 * 8, 16 * 16, 9 * 8, 32 },
                "Cancel"
        };
};
*/

class FileManagerView : public FileManBaseView {
   public:
    FileManagerView(NavigationView& nav);
    virtual ~FileManagerView() {}

   private:
    // Passed by ref to other views needing lifetime extension.
    std::string name_buffer{};
    std::filesystem::path clipboard_path{};
    ClipboardMode clipboard_mode{ClipboardMode::None};

    void refresh_widgets(const bool v);
    void on_rename(std::string_view hint);
    void on_delete();
    void on_clean();
    void on_paste();
    void on_new_dir();
    void on_new_file();

    bool handle_file_open();

    // True if the selected entry is a real file item.
    bool selected_is_valid() const;

    Text text_date{
        {0 * 8, 26 * 8, 28 * 8, 16},
        ""};

    NewButton button_rename{
        {0 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_rename,
        Theme::getInstance()->fg_blue->foreground};

    NewButton button_delete{
        {9 * 8, 34 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_trash,
        Theme::getInstance()->fg_red->foreground};

    NewButton button_clean{
        {13 * 8, 34 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_clean,
        Theme::getInstance()->fg_red->foreground};

    NewButton button_cut{
        {9 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_cut,
        Theme::getInstance()->fg_dark->foreground};

    NewButton button_copy{
        {13 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_copy,
        Theme::getInstance()->fg_dark->foreground};

    NewButton button_paste{
        {17 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_paste,
        Theme::getInstance()->fg_dark->foreground};

    NewButton button_new_dir{
        {22 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_new_dir,
        Theme::getInstance()->fg_green->foreground};

    NewButton button_new_file{
        {26 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_new_file,
        Theme::getInstance()->fg_green->foreground};

    NewButton button_open_notepad{
        {0 * 8, 34 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_notepad,
        Theme::getInstance()->fg_orange->foreground};

    NewButton button_rename_timestamp{

        {4 * 8, 29 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_options_datetime,
        Theme::getInstance()->fg_blue->foreground,
        /*vcenter*/ true};

    NewButton button_open_iq_trim{

        {4 * 8, 34 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_trim,
        Theme::getInstance()->fg_orange->foreground};

    NewButton button_show_hidden_files{
        {17 * 8, 34 * 8, 4 * 8, 32},
        {},
        &bitmap_icon_hide,
        Theme::getInstance()->fg_dark->foreground};
};

} /* namespace ui */
