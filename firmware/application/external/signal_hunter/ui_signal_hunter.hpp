/*
 * Copyright (C) 2026 Matej Sochan
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

#ifndef __UI_SIGNAL_HUNTER_H__
#define __UI_SIGNAL_HUNTER_H__

#include "app_settings.hpp"
#include "rf_path.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_tabview.hpp"
#include "ui_freq_field.hpp"
#include "capture_thread.hpp"
#include "message.hpp"
#include <vector>
#include <memory>

namespace ui::external_app::signal_hunter {

class SignalHunterAppView;

// --- TAB 1: Main View ---
class HunterMainView : public View {
   public:
    HunterMainView(Rect parent_rect, SignalHunterAppView& parent);
    void on_show() override;
    void focus() override;
    void update_status(const std::string& status, const Style* style);
    void update_hits(uint32_t hits);
    void update_frequency(rf::Frequency freq);
    void set_recording_state(bool recording);
    void set_start_button_text(const std::string& text) { button_start_stop.set_text(text); }

   private:
    SignalHunterAppView& parent_app;
    rf::Frequency current_freq_{433920000};
    Text text_current_freq{{UI_POS_X_CENTER(14), UI_POS_Y(3), 112, 16}, ""};
    Text text_status{{UI_POS_X_CENTER(12), UI_POS_Y(6), 96, 16}, "IDLE"};
    Button button_start_stop{{UI_POS_X_CENTER(10), UI_POS_Y(8), 80, 32}, "START"};
    Text text_hits{{UI_POS_X_CENTER(10), UI_POS_Y(11), 80, 16}, "Hits: 0"};
};

// --- TAB 2: Freqs View ---
class HunterFreqsView : public View {
   public:
    HunterFreqsView(Rect parent_rect, SignalHunterAppView& parent);
    void focus() override;
    void update_list_count();

   private:
    SignalHunterAppView& parent_app;
    Button button_load_file{{UI_POS_X(2), UI_POS_Y(1), 80, 28}, "LOAD FILE"};
    Button button_clear{{UI_POS_X_RIGHT(12), UI_POS_Y(1), 80, 28}, "CLEAR"};

    Labels labels{
        {{UI_POS_X(2), UI_POS_Y(4)}, "Dwell Time (ms):", Color::light_grey()}};
    NumberField field_dwell{{UI_POS_X_RIGHT(8), UI_POS_Y(4)}, 4, {10, 9999}, 10, ' '};

    Text text_loaded_info{{UI_POS_X(2), UI_POS_Y(6), 208, 16}, "Loaded: 0 freqs"};
};

// --- TAB 3: Config View ---
class HunterConfigView : public View {
   public:
    HunterConfigView(Rect parent_rect, SignalHunterAppView& parent);
    void update_mode_display();
    void focus() override;
    void on_show() override;

   private:
    SignalHunterAppView& parent_app;

    Button button_mode{
        {UI_POS_X(2), UI_POS_Y(2), 208, 28},
        "MODE: SINGLE"};

    RxFrequencyField field_single_freq;  // UI_POS_Y(4)

    Labels labels{
        {{UI_POS_X(2), UI_POS_Y(6)}, "Energy Threshold:", Color::light_grey()},
        {{UI_POS_X(2), UI_POS_Y(8)}, "Hang-Time (ms):", Color::light_grey()}};

    NumberField field_threshold{{UI_POS_X_RIGHT(8), UI_POS_Y(6)}, 5, {100, 99999}, 100, ' '};
    NumberField field_hang_time{{UI_POS_X_RIGHT(8), UI_POS_Y(8)}, 4, {10, 5000}, 10, ' '};

    Text text_info_config{
        {UI_POS_X(2), UI_POS_Y(10), 208, 16},
        "Restart HUNT after change"};
};

// --- MAIN APP VIEW ---
class SignalHunterAppView final : public ui::View {
   public:
    SignalHunterAppView(ui::NavigationView& nav);
    ~SignalHunterAppView();
    void focus() override;
    std::string title() const override { return "SignalHunter"; }
    ui::NavigationView& get_nav() { return nav_; }

    // Application state shared across all views
    std::vector<rf::Frequency> frequency_list;
    uint32_t current_freq_index{0};
    uint32_t energy_threshold{5000};
    uint32_t hangtime_ms{500};
    uint32_t hop_dwell_ms{200};
    std::string freqman_file{"TARGETS"};
    bool is_hunting{false};
    uint32_t trigger_hits{0};
    rf::Frequency single_frequency{433920000};
    bool freq_hop_mode{false};

    void send_hunter_config(bool start);
    HunterMainView* get_main_view() { return view_main.get(); }
    HunterConfigView* get_config_view() { return view_config.get(); }

   private:
    ui::NavigationView& nav_;

    std::unique_ptr<app_settings::SettingsManager> settings_{};
    std::unique_ptr<HunterMainView> view_main{};
    std::unique_ptr<HunterFreqsView> view_freqs{};
    std::unique_ptr<HunterConfigView> view_config{};
    std::unique_ptr<TabView> tab_view{};

    std::string current_capture_filename{};
    std::unique_ptr<CaptureThread> capture_thread{};

    ui::LNAGainField field_lna{{UI_POS_X(0), UI_POS_Y(0)}};
    ui::VGAGainField field_vga{{UI_POS_X(7), UI_POS_Y(0)}};
    ui::RFAmpField field_rf_amp{{UI_POS_X(14), UI_POS_Y(0)}};
    ui::RSSI rssi{{UI_POS_X(0), UI_POS_Y(1), UI_POS_MAXWIDTH, 4}};

    static constexpr Dim tab_bar_h = 20;

    Rect view_rect{
        0, UI_POS_Y(2) + 4, UI_POS_MAXWIDTH,
        screen_height - (UI_POS_Y(2) + 4) - UI_POS_HEIGHT(1)};

    Rect content_rect{
        0, UI_POS_Y(2) + 4 + tab_bar_h, UI_POS_MAXWIDTH,
        screen_height - (UI_POS_Y(2) + 4 + tab_bar_h) - UI_POS_HEIGHT(1)};

    void on_hunter_trigger(const HunterTriggerMessage* message);
    void on_hunter_stop(const HunterStopMessage* message);
    void start_recording();
    void stop_recording();

    // timer variables
    uint32_t hop_timer_ms{0};
    void on_frame_sync();

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->on_frame_sync();
        }};

    MessageHandlerRegistration message_handler_trigger{
        Message::ID::HunterTrigger,
        [this](const Message* const p) {
            this->on_hunter_trigger(static_cast<const HunterTriggerMessage*>(p));
        }};

    MessageHandlerRegistration message_handler_stop{
        Message::ID::HunterStop,
        [this](const Message* const p) {
            this->on_hunter_stop(static_cast<const HunterStopMessage*>(p));
        }};
};

}  // namespace ui::external_app::signal_hunter

#endif /*__UI_SIGNAL_HUNTER_H__*/
