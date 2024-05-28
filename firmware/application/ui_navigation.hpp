/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 * Copyright (C) 2024 u-foka
 * Copyleft (ɔ) 2024 zxkmm under GPL license
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

#ifndef __UI_NAVIGATION_H__
#define __UI_NAVIGATION_H__

#include <vector>
#include <map>
#include <utility>

#include "ui.hpp"
#include "theme.hpp"

#include "ui_widget.hpp"
#include "ui_focus.hpp"
#include "ui_menu.hpp"
#include "ui_btngrid.hpp"

#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "ui_audio.hpp"
#include "ui_sd_card_status_view.hpp"
#include "ui_dfu_menu.hpp"

#include "bitmap.hpp"
#include "ff.h"
#include "diskio.h"
#include "lfsr_random.hpp"
#include "sd_card.hpp"
#include "external_app.hpp"
#include "view_factory.hpp"

// for incrementing fake date when RTC battery is dead
#define DATE_FILEFLAG u"/SETTINGS/DATE_FILEFLAG"

using namespace sd_card;

namespace ui {

enum modal_t {
    INFO = 0,
    YESNO,
    ABORT
};

class CstrCmp {
   public:
    bool operator()(const char* a, const char* b) const;
};

// Should only be used as part of the appList in NavigationView, the viewFactory will never be destroyed.
class AppInfo {
   public:
    const char* id;  // MUST be unique! Used by serial command to start the app so it also has to make sense
    const char* displayName;
    app_location_t menuLocation;
    Color iconColor;
    const Bitmap* icon;
    ViewFactoryBase* viewFactory;  // Never destroyed, and I believe it's ok ;) Having a unique_ptr here breaks the initializer list of appList
};

struct AppInfoConsole {
    const char* appCallName;
    const char* appFriendlyName;
    const app_location_t appLocation;
};

class NavigationView : public View {
   public:
    std::function<void(const View&)> on_view_changed{};

    NavigationView() = default;

    NavigationView(const NavigationView&) = delete;
    NavigationView(NavigationView&&) = delete;
    NavigationView& operator=(const NavigationView&) = delete;
    NavigationView& operator=(NavigationView&&) = delete;

    bool is_top() const;
    bool is_valid() const;

    template <class T, class... Args>
    T* push(Args&&... args) {
        return reinterpret_cast<T*>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
    }

    template <class T, class... Args>
    T* replace(Args&&... args) {
        pop();
        return reinterpret_cast<T*>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
    }

    void push(View* v);
    View* push_view(std::unique_ptr<View> new_view);
    void replace(View* v);
    void pop(bool trigger_update = true);
    void home(bool trigger_update);

    void display_modal(const std::string& title, const std::string& message);
    void display_modal(
        const std::string& title,
        const std::string& message,
        modal_t type,
        std::function<void(bool)> on_choice = nullptr,
        bool compact = false);

    void focus() override;

    /* Sets the 'on_pop' handler for the current view.
     * Returns true if the handler was bound successfully. */
    bool set_on_pop(std::function<void()> on_pop);

    // App list is used to preserve order, so the menu items in the menu grid can stay in place
    // App map is used to look up apps by id used by serial app start
    using AppMap = std::map<const char*, const AppInfo&, CstrCmp>;
    using AppList = std::vector<AppInfo>;
    static const AppMap appMap;
    static const AppList appList;

    bool StartAppByName(const char* name);  // Starts a View  (app) by name stored in appListFC. This is to start apps from console
    void handle_autostart();

   private:
    struct ViewState {
        std::unique_ptr<View> view;
        std::function<void()> on_pop;
    };

    std::vector<ViewState> view_stack{};

    Widget* view() const;

    void free_view();
    void update_view();
};

/* Holds widgets and grows dynamically toward the left.
 * 16px tall fixed and right-aligns all children in the
 * order in which they were added. */
// TODO: Could make this a generic "StackPanel" control.
class StatusTray : public View {
   public:
    StatusTray(Point pos);

    StatusTray(const StatusTray&) = delete;
    StatusTray& operator=(const StatusTray&) = delete;

    void add_button(ImageButton* child);
    void add(Widget* child);
    void update_layout();
    void clear();
    void paint(Painter& painter) override;
    uint8_t width() { return width_; };

   private:
    static constexpr uint8_t height = 16;
    // This control grow to the left, so keep
    // track of the right edge.
    const Point pos_{};
    uint8_t width_{};
};

class SystemStatusView : public View {
   public:
    std::function<void(void)> on_back{};

    SystemStatusView(NavigationView& nav);

    void set_back_enabled(bool new_value);
    void set_back_hidden(bool new_value);
    void set_title_image_enabled(bool new_value);
    void set_title(const std::string new_value);

   private:
    static constexpr auto default_title = "";
    bool batt_info_up = false;  // to prevent show multiple batt info dialog
    NavigationView& nav_;

    Rectangle backdrop{
        {0 * 8, 0 * 16, ui::screen_width, 16},
        Theme::getInstance()->bg_dark->background};

    ImageButton button_back{
        {0, 0 * 16, 12 * 8, 16},  // Back button also covers the title for easier touch.
        &bitmap_icon_previous,
        Theme::getInstance()->bg_dark->foreground,
        Theme::getInstance()->bg_dark->background};

    Text title{
        {20, 0, 14 * 8, 1 * 16},
        default_title,
    };

    ImageButton button_title{
        {2, 0, 80, 16},
        &bitmap_titlebar_image,
        Theme::getInstance()->bg_dark->foreground,
        Theme::getInstance()->bg_dark->background};

    StatusTray status_icons{{screen_width, 0}};

    ImageToggle toggle_speaker{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_speaker_mute,
        &bitmap_icon_speaker,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background,
        *Theme::getInstance()->status_active,
        Theme::getInstance()->bg_dark->background};

    ImageToggle toggle_mute{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_speaker_and_headphones_mute,
        &bitmap_icon_speaker_and_headphones,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background,
        *Theme::getInstance()->status_active,
        Theme::getInstance()->bg_dark->background};

    ImageButton button_converter{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_upconvert,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background};

    ImageToggle toggle_stealth{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_stealth,
        &bitmap_icon_stealth,
        *Theme::getInstance()->status_active,
        Theme::getInstance()->bg_dark->background,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background};

    ImageButton button_camera{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_camera,
        Theme::getInstance()->bg_dark->foreground,
        Theme::getInstance()->bg_dark->background};

    ImageButton button_sleep{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_sleep,
        Theme::getInstance()->bg_dark->foreground,
        Theme::getInstance()->bg_dark->background};

    ImageButton button_bias_tee{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_biast_off,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background};

    ImageButton button_clock_status{
        {0, 0 * 16, 8, 1 * 16},
        &bitmap_icon_clk_int,
        Theme::getInstance()->fg_light->foreground,
        Theme::getInstance()->bg_dark->background};

    ImageButton button_fake_brightness{
        {0, 0, 2 * 8, 1 * 16},
        &bitmap_icon_brightness,
        *Theme::getInstance()->status_active,
        Theme::getInstance()->bg_dark->background};

    SDCardStatusView sd_card_status_view{
        {0, 0 * 16, 2 * 8, 1 * 16}};

    BatteryTextField battery_text{{0, 0, 2 * 8, 1 * 16}, 102};
    BatteryIcon battery_icon{{0, 0, 10, 1 * 16}, 102};

    void on_converter();
    void on_bias_tee();
    void on_camera();
    void on_title();
    void refresh();
    void on_clk();
    void rtc_battery_workaround();
    void on_battery_data(const BatteryStateMessage* msg);
    void on_battery_details();

    MessageHandlerRegistration message_handler_refresh{
        Message::ID::StatusRefresh,
        [this](const Message* const p) {
            (void)p;
            this->refresh();
        }};

    MessageHandlerRegistration message_handler_battery{
        Message::ID::BatteryStateData,
        [this](const Message* const p) {
            const auto message = static_cast<const BatteryStateMessage*>(p);
            this->on_battery_data(message);
        }};
};

class InformationView : public View {
   public:
    InformationView(NavigationView& nav);
    void refresh();
    bool firmware_checksum_error();

   private:
    // static constexpr auto version_string = "v1.4.4"; // This is commented out as we are now setting the version via ENV (VERSION_STRING=v1.0.0)
    NavigationView& nav_;

    Rectangle backdrop{
        {0, 0 * 16, 240, 16},
        {33, 33, 33}};

    Text version{
        {2, 0, 11 * 8, 16},
        VERSION_STRING};

    LiveDateTime ltime{
        {86, 0, 19 * 8, 16}};
};

class BMPView : public View {
   public:
    BMPView(NavigationView& nav);
    void paint(Painter&) override;
    void focus() override;

   private:
    Button button_done{
        {240, 0, 1, 1},
        ""};
};

class ReceiversMenuView : public BtnGridView {
   public:
    ReceiversMenuView(NavigationView& nav);
    std::string title() const override { return "Receive"; };

   private:
    NavigationView& nav_;
    void on_populate() override;
};

class TransmittersMenuView : public BtnGridView {
   public:
    TransmittersMenuView(NavigationView& nav);
    std::string title() const override { return "Transmit"; };

   private:
    NavigationView& nav_;
    void on_populate() override;
};

class UtilitiesMenuView : public BtnGridView {
   public:
    UtilitiesMenuView(NavigationView& nav);
    std::string title() const override { return "Utilities"; };

   private:
    NavigationView& nav_;
    void on_populate() override;
};

class SystemMenuView : public BtnGridView {
   public:
    SystemMenuView(NavigationView& nav);

   private:
    NavigationView& nav_;
    void on_populate() override;
    void hackrf_mode(NavigationView& nav);
};

class SystemView : public View {
   public:
    SystemView(
        Context& context,
        const Rect parent_rect);

    Context& context() const override;
    void toggle_overlay();
    void paint_overlay();
    void set_app_fullscreen(bool fullscreen);

    NavigationView* get_navigation_view();
    SystemStatusView* get_status_view();

   private:
    uint8_t overlay_active{0};

    SystemStatusView status_view{navigation_view};
    InformationView info_view{navigation_view};
    DfuMenu overlay{navigation_view};
    DfuMenu2 overlay2{navigation_view};
    NavigationView navigation_view{};
    Context& context_;
};

/*class NotImplementedView : public View {
public:
        NotImplementedView(NavigationView& nav);

        void focus() override;

private:
        Text text_title {
                { 5 * 8, 7 * 16, 19 * 8, 16 },
                "Not Yet Implemented"
        };

        Button button_done {
                { 10 * 8, 13 * 16, 10 * 8, 24 },
                "Bummer",
        };
};*/

class ModalMessageView : public View {
   public:
    ModalMessageView(
        NavigationView& nav,
        const std::string& title,
        const std::string& message,
        modal_t type,
        std::function<void(bool)> on_choice,
        bool compact = false);

    void paint(Painter& painter) override;
    void focus() override;

    std::string title() const override { return title_; };

   private:
    const std::string title_;
    const std::string message_;
    const modal_t type_;
    const std::function<void(bool)> on_choice_;
    const bool compact;

    Button button_ok{
        {10 * 8, 14 * 16, 10 * 8, 48},
        "OK",
    };

    Button button_yes{
        {5 * 8, 14 * 16, 8 * 8, 48},
        "YES",
    };

    Button button_no{
        {17 * 8, 14 * 16, 8 * 8, 48},
        "NO",
    };
};

} /* namespace ui */

#endif /*__UI_NAVIGATION_H__*/
