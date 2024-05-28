/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __POCSAG_APP_H__
#define __POCSAG_APP_H__

#include "ui_widget.hpp"
#include "ui_freq_field.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"

#include "app_settings.hpp"
#include "log_file.hpp"
#include "pocsag.hpp"
#include "pocsag_packet.hpp"
#include "radio_state.hpp"

#include <functional>

class POCSAGLogger {
   public:
    Optional<File::Error> append(const std::filesystem::path& filename) {
        return log_file.append(filename);
    }

    void log_raw_data(const pocsag::POCSAGPacket& packet, const uint32_t frequency);
    void log_decoded(Timestamp timestamp, const std::string& text);

   private:
    LogFile log_file{};
};

namespace ui {

class BaudIndicator : public Widget {
   public:
    BaudIndicator(Point position)
        : Widget{{position, {5, height}}} {}

    void paint(Painter& painter) override;
    void set_rate(uint16_t rate) {
        if (rate != rate_) {
            rate_ = rate;
            set_dirty();
        }
    }

   private:
    static constexpr uint8_t height = 16;
    uint16_t rate_ = 0;
};

class BitsIndicator : public Widget {
   public:
    BitsIndicator(Point position)
        : Widget{{position, {2, height}}} {}

    void paint(Painter& painter) override;
    void set_bits(uint32_t bits) {
        if (bits != bits_) {
            bits_ = bits;
            set_dirty();
        }
    }

   private:
    static constexpr uint8_t height = 16;
    uint32_t bits_ = 0;
};

class FrameIndicator : public Widget {
   public:
    FrameIndicator(Point position)
        : Widget{{position, {4, height}}} {}

    void paint(Painter& painter) override;
    void set_frames(uint8_t frame_count) {
        if (frame_count != frame_count_) {
            frame_count_ = frame_count;
            set_dirty();
        }
    }

    void set_sync(bool has_sync) {
        if (has_sync != has_sync_) {
            has_sync_ = has_sync;
            set_dirty();
        }
    }

   private:
    static constexpr uint8_t height = 16;
    uint8_t frame_count_ = 0;
    bool has_sync_ = false;
};

enum POCSAGFilter : uint8_t {
    FILTER_NONE,
    FILTER_DROP,
    FILTER_KEEP
};

struct POCSAGSettings {
    bool enable_small_font = false;
    bool enable_logging = false;
    bool enable_raw_log = false;
    bool hide_bad_data = false;
    bool hide_addr_only = false;
    uint8_t filter_mode = false;
    uint32_t filter_address = 0;
};

class POCSAGSettingsView : public View {
   public:
    POCSAGSettingsView(NavigationView& nav, POCSAGSettings& settings);

    std::string title() const override { return "POCSAG Config"; };
    void focus() override { button_save.focus(); }

   private:
    POCSAGSettings& settings_;

    Labels labels{
        {{2 * 8, 12 * 16}, "Filter Mode:", Theme::getInstance()->fg_light->foreground},
        {{2 * 8, 13 * 16}, "Filter Addr:", Theme::getInstance()->fg_light->foreground},
    };

    Checkbox check_log{
        {2 * 8, 2 * 16},
        10,
        "Enable Log"};

    Checkbox check_log_raw{
        {2 * 8, 4 * 16},
        12,
        "Log Raw Data"};

    Checkbox check_small_font{
        {2 * 8, 6 * 16},
        4,
        "Use Small Font"};

    Checkbox check_hide_bad{
        {2 * 8, 8 * 16},
        22,
        "Hide Bad Data"};

    Checkbox check_hide_addr_only{
        {2 * 8, 10 * 16},
        22,
        "Hide Addr Only"};

    OptionsField opt_filter_mode{
        {15 * 8, 12 * 16},
        4,
        {{"None", FILTER_NONE},
         {"Drop", FILTER_DROP},
         {"Keep", FILTER_KEEP}}};

    SymField field_filter_address{
        {15 * 8, 13 * 16},
        7,
        SymField::Type::Dec,
        true /*explicit_edit*/};

    Button button_save{
        {11 * 8, 16 * 16, 10 * 8, 2 * 16},
        "Save"};
};

class POCSAGAppView : public View {
   public:
    POCSAGAppView(NavigationView& nav);
    ~POCSAGAppView();

    std::string title() const override { return "POCSAG RX"; };
    void focus() override;

   private:
    bool logging() const { return settings_.enable_logging; };
    bool logging_raw() const { return settings_.enable_raw_log; };
    bool hide_bad_data() const { return settings_.hide_bad_data; };
    bool hide_addr_only() const { return settings_.hide_addr_only; };

    NavigationView& nav_;
    RxRadioState radio_state_{
        466175000 /* frequency*/,
        max283x::filter::bandwidth_minimum /* bandwidth */,
        3072000 /* sampling rate */
    };

    // Settings
    POCSAGSettings settings_{};
    app_settings::SettingsManager app_settings_{
        "rx_pocsag"sv,
        app_settings::Mode::RX,
        {
            {"small_font"sv, &settings_.enable_small_font},
            {"enable_logging"sv, &settings_.enable_logging},
            {"enable_raw_log"sv, &settings_.enable_raw_log},
            {"filter_mode"sv, &settings_.filter_mode},
            {"filter_address"sv, &settings_.filter_address},
            {"hide_bad_data"sv, &settings_.hide_bad_data},
            {"hide_addr_only"sv, &settings_.hide_addr_only},
        }};

    void refresh_ui();
    bool ignore_address(uint32_t address) const;
    void handle_decoded(Timestamp timestamp, const std::string& prefix);
    void on_packet(const POCSAGPacketMessage* message);
    void on_stats(const POCSAGStatsMessage* stats);

    uint32_t last_address = 0;
    pocsag::EccContainer ecc{};
    pocsag::POCSAGState pocsag_state{&ecc};
    POCSAGLogger logger{};
    uint16_t packet_count = 0;

    RxFrequencyField field_frequency{
        {0 * 8, 0 * 8},
        nav_};

    RFAmpField field_rf_amp{
        {11 * 8, 0 * 16}};
    LNAGainField field_lna{
        {13 * 8, 0 * 16}};
    VGAGainField field_vga{
        {16 * 8, 0 * 16}};

    RSSI rssi{
        {19 * 8 - 4, 3, 6 * 8, 4}};
    Audio audio{
        {19 * 8 - 4, 8, 6 * 8, 4}};

    NumberField field_squelch{
        {25 * 8, 0 * 16},
        2,
        {0, 99},
        1,
        ' ',
        true /*wrap*/};
    AudioVolumeField field_volume{
        {28 * 8, 0 * 16}};

    Image image_status{
        {0 * 8 + 4, 1 * 16 + 2, 16, 16},
        &bitmap_icon_pocsag,
        Theme::getInstance()->bg_darkest->foreground,
        Theme::getInstance()->bg_darkest->background};

    Text text_packet_count{
        {3 * 8, 1 * 16 + 2, 5 * 8, 16},
        "0"};

    BitsIndicator widget_bits{
        {8 * 8 + 1, 1 * 16 + 2}};

    FrameIndicator widget_frames{
        {8 * 8 + 4, 1 * 16 + 2}};

    BaudIndicator widget_baud{
        {8 * 9 + 1, 1 * 16 + 2}};

    Button button_filter_last{
        {10 * 8, 1 * 16, 12 * 8, 20},
        "Filter Last"};

    Button button_config{
        {22 * 8, 1 * 16, 8 * 8, 20},
        "Config"};

    // 54 == status bar (16) + top controls (2 * 16 + 6).
    Console console{
        {0, 2 * 16 + 6, screen_width, screen_height - 54}};

    MessageHandlerRegistration message_handler_packet{
        Message::ID::POCSAGPacket,
        [this](Message* const p) {
            const auto message = static_cast<const POCSAGPacketMessage*>(p);
            this->on_packet(message);
        }};

    MessageHandlerRegistration message_handler_stats{
        Message::ID::POCSAGStats,
        [this](Message* const p) {
            const auto stats = static_cast<const POCSAGStatsMessage*>(p);
            this->on_stats(stats);
        }};
};

} /* namespace ui */

#endif /*__POCSAG_APP_H__*/
