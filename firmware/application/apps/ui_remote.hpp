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
#include "ui_spectrum.hpp"
#include "ui_transmitter.hpp"

#include "app_settings.hpp"
#include "baseband_api.hpp"
#include "bitmap.hpp"
#include "file.hpp"
#include "metadata_file.hpp"
#include "optional.hpp"
#include "radio_state.hpp"
#include "replay_thread.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
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
    // NB: Icons need to be 16x16 or they won't fit corrently.
    static constexpr std::array<const Bitmap*, 25> bitmaps_{
        nullptr,
        &bitmap_icon_fox,
        &bitmap_icon_adsb,
        &bitmap_icon_ais,
        &bitmap_icon_aprs,
        &bitmap_icon_btle,
        &bitmap_icon_burger,
        &bitmap_icon_camera,
        &bitmap_icon_cwgen,
        &bitmap_icon_dmr,
        &bitmap_icon_file_image,
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

// TODO: Use RGB colors instead?
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
    static constexpr std::array<Color, 21> colors_{
        Color::black(),         // 0
        Color::white(),         // 1
        Color::darker_grey(),   // 2
        Color::dark_grey(),     // 3
        Color::grey(),          // 4
        Color::light_grey(),    // 5
        Color::red(),           // 6
        Color::orange(),        // 7
        Color::yellow(),        // 8
        Color::green(),         // 9
        Color::blue(),          // 10
        Color::cyan(),          // 11
        Color::magenta(),       // 12
        Color::dark_red(),      // 13
        Color::dark_orange(),   // 14
        Color::dark_yellow(),   // 15
        Color::dark_green(),    // 16
        Color::dark_blue(),     // 17
        Color::dark_cyan(),     // 18
        Color::dark_magenta(),  // 19
        Color::purple()};       // 20
};

/* Data model for a remote entry. */
struct RemoteEntryModel {
    std::filesystem::path path{};
    std::string name{};
    uint8_t icon = 0;
    uint8_t bg_color = 0;
    uint8_t fg_color = 0;
    capture_metadata metadata{};
    // TODO: start/end position for trimming.

    std::string to_string() const;
    static Optional<RemoteEntryModel> parse(std::string_view line);
};

/* Data model for a remote. */
struct RemoteModel {
    std::string name{};
    std::vector<RemoteEntryModel> entries{};

    bool delete_entry(const RemoteEntryModel* entry);
    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path);
};

/* Button for the remote UI. */
class RemoteButton : public NewButton {
   public:
    std::function<void(RemoteButton&)> on_select2{};
    std::function<void(RemoteButton&)> on_long_select{};

    RemoteButton(Rect parent_rect, RemoteEntryModel* entry);
    RemoteButton(const RemoteButton&) = delete;
    RemoteButton& operator=(const RemoteButton&) = delete;

    void on_focus() override;
    void on_blur() override;
    bool on_key(KeyEvent key) override;
    void paint(Painter& painter) override;

    RemoteEntryModel* entry();
    void set_entry(RemoteEntryModel* entry);

   protected:
    Style paint_style() override;

   private:
    // Hide because it's not used.
    using NewButton::on_select;
    RemoteEntryModel* entry_;
};

/* Settings container for remote. */
struct RemoteSettings {
    std::string remote_path{};
};

/* View for editing a remote entry button. */
class RemoteEntryEditView : public View {
   public:
    std::function<void(RemoteEntryModel&)> on_delete{};

    RemoteEntryEditView(NavigationView& nav, RemoteEntryModel& entry);

    std::string title() const override { return "Edit Button"; };
    void focus() override;

   private:
    RemoteEntryModel& entry_;

    void refresh_ui();
    void load_path(std::filesystem::path&& path);

    Labels labels{
        {{2 * 8, 1 * 16}, "Name:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 2 * 16}, "Path:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 3 * 16}, "Freq:", Theme::getInstance()->fg_light->foreground},
        {{17 * 8, 3 * 16}, "MHz", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 4 * 16}, "Rate:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 5 * 16}, "Icon:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 6 * 16}, "FG Color:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 7 * 16}, "BG Color:", Theme::getInstance()->fg_light->foreground},
        {{8 * 8, 9 * 16}, "Button preview", Theme::getInstance()->fg_light->foreground},
    };

    TextField field_name{{8 * 8, 1 * 16, 20 * 8, 1 * 16}, {}};

    TextField field_path{{8 * 8, 2 * 16, 20 * 8, 1 * 16}, {}};

    FrequencyField field_freq{{8 * 8, 3 * 16}};

    Text text_rate{{8 * 8, 4 * 16, 20 * 8, 1 * 16}, {}};

    NumberField field_icon_index{
        {8 * 8, 5 * 16},
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
        {10 * 8, 11 * 16 - 8, 10 * 8, 50},
        &entry_};

    NewButton button_delete{
        {2 * 8, 16 * 16, 4 * 8, 2 * 16},
        {},
        &bitmap_icon_trash,
        Color::red()};

    Button button_done{{11 * 8, 16 * 16, 8 * 8, 2 * 16}, "Done"};
};

/* App that allows for buttons to be bound to captures for playback. */
class RemoteView : public View {
   public:
    RemoteView(NavigationView& nav);
    RemoteView(NavigationView& nav, std::filesystem::path path);
    ~RemoteView();

    RemoteView(const RemoteView&) = delete;
    RemoteView& operator=(const RemoteView&) = delete;

    std::string title() const override { return "Remote"; };
    void focus() override;

   private:
    /* Creates the dynamic buttons. */
    void create_buttons();
    /* Resets all the pointers to null entries. */
    void reset_buttons();
    void refresh_ui();

    void add_button();
    void edit_button(RemoteButton& btn);
    void send_button(RemoteButton& btn);
    void stop();
    void new_remote();
    void open_remote();
    void init_remote();
    bool load_remote(std::filesystem::path&& path);
    void save_remote(bool show_errors = true);
    void rename_remote(const std::string& new_name);
    void handle_replay_thread_done(uint32_t return_code);
    void set_needs_save(bool v = true) { needs_save_ = v; }
    void set_remote_path(std::filesystem::path&& path);
    bool is_sending() const { return replay_thread_ != nullptr; }
    void show_error(const std::string& msg) const;

    static constexpr Dim button_rows = 4;
    static constexpr Dim button_cols = 3;
    static constexpr uint8_t max_buttons = button_rows * button_cols;
    static constexpr Dim button_area_height = 200;
    static constexpr Dim button_width = screen_width / button_cols;
    static constexpr Dim button_height = button_area_height / button_rows;

    // This value is mysterious... why?
    static constexpr uint32_t baseband_bandwidth = 2'500'000;

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

    bool needs_save_ = false;
    std::string temp_buffer_{};
    std::filesystem::path remote_path_{};
    RemoteButton* current_btn_{};

    const Point buttons_top_{0, 20};
    std::vector<std::unique_ptr<RemoteButton>> buttons_{};

    std::unique_ptr<ReplayThread> replay_thread_{};
    bool ready_signal_{};  // Used to signal ReplayThread ready.

    TextField field_title{
        {0 * 8, 0 * 16 + 2, 30 * 8, 1 * 16},
        {}};

    TransmitterView2 tx_view{
        {0 * 8, 17 * 16},
        /*short_ui*/ true};

    Checkbox check_loop{
        {10 * 8, 17 * 16},
        4,
        "Loop",
        /*small*/ true};

    TextField field_filename{
        {0 * 8, 18 * 16, 17 * 8, 1 * 16},
        {}};

    NewButton button_add{
        {17 * 8 + 4, 17 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_add,
        Color::orange(),
        /*vcenter*/ true};

    NewButton button_new{
        {22 * 8, 17 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_new_file,
        Color::dark_blue(),
        /*vcenter*/ true};

    NewButton button_open{
        {26 * 8, 17 * 16, 4 * 8, 2 * 16},
        "",
        &bitmap_icon_load,
        Color::dark_blue(),
        /*vcenter*/ true};

    spectrum::WaterfallView waterfall{};

    MessageHandlerRegistration message_handler_replay_thread_error{
        Message::ID::ReplayThreadDone,
        [this](const Message* p) {
            auto message = *reinterpret_cast<const ReplayThreadDoneMessage*>(p);
            handle_replay_thread_done(message.return_code);
        }};

    MessageHandlerRegistration message_handler_fifo_signal{
        Message::ID::RequestSignal,
        [this](const Message* p) {
            auto message = static_cast<const RequestSignalMessage*>(p);
            if (message->signal == RequestSignalMessage::Signal::FillRequest) {
                ready_signal_ = true;
            }
        }};
};

} /* namespace ui */
