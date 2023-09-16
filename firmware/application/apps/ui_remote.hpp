/*
 * Copyright (C) 2023 Kyle Reed
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
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"

#include "app_settings.hpp"
#include "bitmap.hpp"
#include "file.hpp"
#include "metadata_file.hpp"
#include "radio_state.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ui {

/* Maps icon index to bitmap. */
class RemoteIcons {
   public:
    static constexpr const Bitmap* get(uint8_t index) {
        if (index >= std::size(bitmaps_))
            return bitmaps_[0];

        return bitmaps_[index];
    }

    static constexpr size_t size() {
        return std::size(bitmaps_);
    }

   private:
    static constexpr std::array<const Bitmap*, 24> bitmaps_{
        &bitmap_icon_adsb,
        &bitmap_icon_ais,
        &bitmap_icon_aprs,
        &bitmap_icon_btle,
        &bitmap_icon_burger,
        &bitmap_icon_camera,
        &bitmap_icon_cwgen,
        &bitmap_icon_dmr,
        &bitmap_icon_file_image,
        &bitmap_icon_fox,
        &bitmap_icon_lge,
        &bitmap_icon_looking,
        &bitmap_icon_memory,
        &bitmap_icon_morse,
        &bitmap_icon_nrf,
        &bitmap_icon_notepad,
        &bitmap_icon_rds,
        &bitmap_icon_remote,
        &bitmap_icon_setup,
        &bitmap_icon_sleep,
        &bitmap_icon_sonde,
        &bitmap_icon_stealth,
        &bitmap_icon_tetra,
        &bitmap_icon_temperature};
};

/* Maps color index to color. */
class RemoteColors {
   public:
    static constexpr Color get(uint8_t index) {
        if (index >= std::size(colors_))
            return colors_[0];

        return colors_[index];
    }

    static constexpr size_t size() {
        return std::size(colors_);
    }

   private:
    static constexpr std::array<Color, 18> colors_{
        Color::black(),
        Color::white(),
        Color::red(),
        Color::orange(),
        Color::yellow(),
        Color::green(),
        Color::blue(),
        Color::cyan(),
        Color::magenta(),
        Color::grey(),
        Color::dark_red(),
        Color::dark_orange(),
        Color::dark_yellow(),
        Color::dark_green(),
        Color::dark_blue(),
        Color::dark_cyan(),
        Color::dark_magenta(),
        Color::dark_grey()};
};

/* Data model for a remote entry. */
struct RemoteEntryModel {
    std::filesystem::path path{};
    std::string name{};
    uint8_t icon = 0;
    uint8_t bg_color = 0;
    uint8_t fg_color = 0;
    capture_metadata metadata{};
    File::Size file_size = 0;
    // TODO: start/end position for trimming.
};

/* Data model for a remote. */
struct RemoteModel {
    std::string name{};
    std::vector<RemoteEntryModel> entries{};

    bool delete_entry(const RemoteEntryModel* entry) {
        // NB: expecting 'entry' to be a pointer to an entry in vector.
        // I hate implementing operator==, so using this hack instead.
        auto it = std::find_if(
            entries.begin(), entries.end(),
            [entry](auto& item) { return entry == &item; });
        if (it == entries.end())
            return false;

        entries.erase(it);
        return true;
    }
};

/* Button for the remote UI. */
class RemoteButton : public NewButton {
   public:
    std::function<void(RemoteButton&)> on_select2{};
    std::function<void(RemoteButton&)> on_long_select{};

    RemoteButton(Rect parent_rect, RemoteEntryModel& entry);

    void on_focus() override;
    void on_blur() override;
    bool on_key(KeyEvent key) override;

    RemoteEntryModel& entry();

   protected:
    Style paint_style() override;

   private:
    // Hide because it's not used.
    using NewButton::on_select;
    RemoteEntryModel& entry_;
};

/* Settings container for remote. */
struct RemoteSettings {
    std::string remote_path{};
};

class RemoteEntryEditView : public View {
   public:
    std::function<void(RemoteEntryModel&)> on_delete{};

    RemoteEntryEditView(NavigationView& nav, RemoteEntryModel& entry);

    std::string title() const override { return "Edit Item"; };
    void focus() override;

   private:
    RemoteEntryModel& entry_;
    std::string temp_buffer_{};
    static constexpr uint8_t text_edit_max = 30;

    Labels labels{
        {{1 * 8, 1 * 16}, "Name:", Color::light_grey()},
        {{1 * 8, 2 * 16}, "Path:", Color::light_grey()},
        {{1 * 8, 3 * 16}, "Freq:", Color::light_grey()},
        {{1 * 8, 4 * 16}, "Rate:", Color::light_grey()},
        {{1 * 8, 5 * 16}, "Icon:", Color::light_grey()},
        {{1 * 8, 6 * 16}, "FG Color:", Color::light_grey()},
        {{1 * 8, 7 * 16}, "BG Color:", Color::light_grey()},
        {{8 * 8, 9 * 16}, "Button preview", Color::light_grey()},
    };

    TextField field_name{{7 * 8, 1 * 16, 20 * 8, 1 * 16}, {}};

    TextField field_path{{7 * 8, 2 * 16, 20 * 8, 1 * 16}, {}};

    FrequencyField field_freq{{7 * 8, 3 * 16}};

    Text text_rate{{7 * 8, 4 * 16, 20 * 8, 1 * 16}, {}};

    NumberField field_icon_index{
        {7 * 8, 5 * 16},
        2,
        {0, RemoteIcons::size() - 1},
        /*step*/ 1,
        /*fill*/ ' ',
        /*loop*/ true};

    NumberField field_fg_color_index{
        {11 * 8, 6 * 16},
        2,
        {0, RemoteColors::size() - 1},
        /*step*/ 1,
        /*fill*/ ' ',
        /*loop*/ true};

    NumberField field_bg_color_index{
        {11 * 8, 7 * 16},
        2,
        {0, RemoteColors::size() - 1},
        /*step*/ 1,
        /*fill*/ ' ',
        /*loop*/ true};

    RemoteButton button_preview{
        {10 * 8, 11 * 16, 10 * 8, 50},
        entry_};

    Button button_delete{{5 * 8, 16 * 16, 10 * 8, 2 * 16}, "Delete"};
    Button button_done{{16 * 8, 16 * 16, 8 * 8, 2 * 16}, "Done"};
};

class RemoteView : public View {
   public:
    RemoteView(NavigationView& nav);
    // RemoteView(NavigationView& nav, const std::filesystem::path& path);
    ~RemoteView();

    std::string title() const override { return "Remote"; };
    void focus() override;

   private:
    void refresh_ui();
    void load_test();

    static constexpr Dim button_rows = 4;
    static constexpr Dim button_cols = 3;
    static constexpr uint8_t max_buttons = button_rows * button_cols;
    static constexpr Dim button_width = 240 / button_cols;
    static constexpr Dim button_height = 200 / button_rows;

    NavigationView& nav_;
    RxRadioState radio_state_{};

    // Settings
    RemoteSettings settings_{};
    app_settings::SettingsManager app_settings_{
        "tx_remote"sv,
        app_settings::Mode::TX,
        {
            {"remote_path"sv, &settings_.remote_path},
        }};

    RemoteModel model_{};
    std::vector<std::unique_ptr<RemoteButton>> buttons_{};

    NewButton button_edit{
        {7 * 8, 4 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_notepad,
        Color::orange()};

    NewButton button_add{
        {11 * 8, 4 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_new_file,
        Color::orange()};

    NewButton button_delete{
        {15 * 8, 4 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_delete,
        Color::orange()};

    NewButton button_open{
        {20 * 8, 4 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_load,
        Color::dark_blue()};

    NewButton button_save{
        {24 * 8, 4 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_save,
        Color::dark_blue()};

    // Waterfall?
};

} /* namespace ui */
